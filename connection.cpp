#include <pwd.h>
#include <shadow.h>

#include "connection.h"

//void listDirectory(Socket* client, const std::string& path)
//{
    //// Temporary buffers so that the async buffer doesn't share with the sync one
    //char tempIn[BUFLEN];
    //char tempOut[BUFLEN];

    //User* user = getUserByClient(client);
    //if (user == NULL)
    //{
        //client->send(SFTP::createFailureResponse(tempOut, SFTP::NOT_LOGGED_IN), tempOut);
        //return;
    //}

    //std::string finalPath = generateNewPath(user->currentDir, path);

    //std::string::size_type homeDirLen = user->homeDir.size();
    //if (finalPath.substr(0, homeDirLen) != user->homeDir)
    //{
        //client->send(SFTP::createFailureResponse(tempOut, SFTP::ACCESS_DENIED), tempOut);
        //return;
    //}

    //std::string ret = exec("ls -la " + finalPath);
    //if (ret.size() == 0)
    //{
        //client->send(SFTP::createFailureResponse(tempOut, SFTP::COMMAND_EXECUTION_FAILED), tempOut);
        //return;
    //}

    //size_t maxlen = BUFLEN - 2; // max number of data bytes allowed in the buffer (1 header byte and 1 null termination)
    //uint32_t numPackets = floor(ret.size() / maxlen) + 1; // total number of packets 

    //client->send(SFTP::crLsPrimary(tempOut, numPackets), tempOut);

    //int i = 0;
    //while (i < ret.size())
    //{
        //client->send(SFTP::crLs(tempOut, ret.substr(i, maxlen)), tempOut);
        //i += maxlen;

        //clearBuffer(BUFLEN, tempIn);
    //}
//}

//size_t changeUserDirectory(Socket* client)
//{
    //User* user = getUserByClient(client);
    //if (user == NULL)
        //return SFTP::createFailureResponse(out, SFTP::NOT_LOGGED_IN);

    //std::string path(in+1); // null termination for command acts as null termination for string
    //std::string newPath = generateNewPath(user->currentDir, path); 

    //// Check if path is valid
    //struct stat buffer;
    //if (stat(newPath.c_str(), &buffer) != 0)
        //return SFTP::createFailureResponse(out, SFTP::INVALID_PATH); 

    //std::string finalPath = newPath;

    //// Check that path is within access bubble
    //std::string::size_type homeDirLen = user->homeDir.size();
    //if (newPath.substr(0, homeDirLen) != user->homeDir)
        //return SFTP::createFailureResponse(out, SFTP::ACCESS_DENIED);

    //user->currentDir = finalPath;
    //return SFTP::crCd(out, finalPath);
//}

//size_t createDirectory(Socket* client, const std::string& name)
//{
    //User* user = getUserByClient(client);
    //if (user == NULL)
        //return SFTP::createFailureResponse(out, SFTP::NOT_LOGGED_IN);

    //std::string path = generateNewPath(user->currentDir, name);

    //// Check that path is within access bubble
    //std::string::size_type homeDirLen = user->homeDir.size();
    //if (path.substr(0, homeDirLen) != user->homeDir)
        //return SFTP::createFailureResponse(out, SFTP::ACCESS_DENIED);
    
    //std::string ret = exec("mkdir -p " + path);
    
    //return SFTP::crMkDir(out, ret);
//}

//// Takes string for file path to prevent buffer issues
//void grabFile(Socket* client, const std::string& filePath)
//{
    //// Temporary buffers so that the async buffer doesn't share with the sync one
    //char tempIn[BUFLEN];
    //char tempOut[BUFLEN];

    //User* user = getUserByClient(client);
    //if (user == NULL)
    //{
        //client->send(SFTP::createFailureResponse(tempOut, SFTP::NOT_LOGGED_IN), tempOut);
        //return;
    //}

    //std::string path;
    //path = generateNewPath(user->currentDir, filePath);
    
    //std::ifstream file;
    //file.open(path, std::ios::binary);

    //file.seekg(0, file.end);
    //int fileSize = file.tellg();
    //file.seekg(0, file.beg);

    //uint32_t totalPackets = floor(fileSize / (BUFLEN - 4)) + 1;

    //client->send(SFTP::crGrabPrimary(tempOut, totalPackets, filePath), tempOut);

    //if (file.is_open())
    //{
        //while (!file.eof())
        //{
            //size_t i = 3; // Starts at 3 to leave space for status byte and length 
            //clearBuffer(BUFLEN, tempOut);
            //while (i < BUFLEN - 1) // Leave space for null termination
            //{
                //char c = (char) file.get();
                //if (file.eof())
                    //break;

                //tempOut[i] = c;
                //++i;
            //}
            
            //uint16_t length = i - 3;

            //[> crGrab works differently from all other response facotries
             //* Instead of overwriting buffer, it only changes the status byte
             //* and the length

             //* This is done to prevent copying of the entire buffer which would be inefficient
             //*/
            //client->send(SFTP::crGrab(tempOut, length), tempOut);
        //}
        //file.close();
    //}
    //else
    //{
        //client->send(SFTP::createFailureResponse(tempOut, SFTP::FAILED_TO_OPEN_FILE), tempOut);
        //return;
    //}
//}

//void receiveFile()
//{
    //// TODO figure this out
//}

Connection::Connection(Socket* sock)
    : sock(sock)
    , in(NULL)
    , out(NULL)
    , running(false)
    , t()
{
    in = new char[BUFLEN];
    out = new char[BUFLEN];
}

Connection::~Connection()
{
    mtx.lock();

    close();
    delete sock;
    delete[] in;
    delete[] out;

    mtx.unlock();
}

// Returns message length
size_t Connection::checkPassword()
{
    char *encrypted, *p;
    struct passwd *pwd;
    struct spwd *spwd;
    uint16_t usernameLength, passwordLength;

    // Get username and password lengths (2 bytes long)
    memcpy(&usernameLength, in+1, 2);
    memcpy(&passwordLength, in+3, 2);

    std::string username(in+5, usernameLength);
    std::string password(in+5 + usernameLength, passwordLength);

    pwd = getpwnam(username.c_str());
    if (pwd == NULL)
        return SFTP::createFailureResponse(out, SFTP::INVALID_USER);

    spwd = getspnam(username.c_str());
    if (spwd == NULL && errno == EACCES)
    {
        LOGGER::LogError("No Access to Shadow file");
        return SFTP::createFailureResponse(out, SFTP::MISC_ERROR);
    }

    if (spwd != NULL)           // If there is a shadow password record
        pwd->pw_passwd = spwd->sp_pwdp;     // Use the shadow password

    encrypted = crypt(password.c_str(), pwd->pw_passwd);
    if (encrypted == NULL)
    {
        LOGGER::LogError("crpyt() failed");
        return SFTP::createFailureResponse(out, SFTP::MISC_ERROR);
    }

    bool authOk = strcmp(encrypted, pwd->pw_passwd) == 0;
    if (!authOk)
        return SFTP::createFailureResponse(out, SFTP::INVALID_PASSWORD);

    // Login Successful
    user = { username, pwd->pw_dir, pwd->pw_dir };
    LOGGER::DebugLog(this->Name() + " Successfully Logged in user: " + username);

    return SFTP::createSuccessResponse(out);
}

size_t Connection::printWorkingDirectory()
{
    if (!this->isLoggedIn())
        return SFTP::createFailureResponse(out, SFTP::NOT_LOGGED_IN);

    return SFTP::crPwd(out, user.currentDir);
}

// Returns of message length
size_t Connection::handleCommand()
{
    switch (SFTP::resolveCommand(in[0]))
    {
    case SFTP::USER:
        // Returns length of output buffer
        return checkPassword(); 
    case SFTP::PRWD:
        // Returns length of output buffer
        return printWorkingDirectory();
    case SFTP::LIST:
        // temp = std::async(std::launch::async, &listDirectory, sock, std::string(in+1));
        return 0;
    case SFTP::CDIR:
        // return changeUserDirectory(sock);
    case SFTP::MDIR:
        // return createDirectory(sock, std::string(in+1));
    case SFTP::GRAB:
        // temp = std::async(std::launch::async, &grabFile, sock, std::string(in+1));
        return 0;
    case SFTP::PUTF:
        // temp = std::async(std::launch::async, &receiveFile);
        return 0;
    }

    return SFTP::createFailureResponse(out, SFTP::INVALID_COMMAND);
}

void Connection::listen()
{
    int code;

    clearBuffers(BUFLEN, in, out);
    sock->send(SFTP::createSuccessResponse(out), out);

    while (running)
    {
        mtx.lock();
        clearBuffer(BUFLEN, in);
        code = sock->recv(in);
        mtx.unlock();

        // Client disconnected
        if (code == 0)
        {
            // This connection object will be deleted next time a client joins
            setActive(false);

            LOGGER::Log("Client: " + sock->Name() + " disconnected", LOGGER::COLOR::RED);
            return;
        }

        // LOGGER::HexDump("recv", in, 100);

        mtx.lock(); 
        clearBuffer(BUFLEN, out);
        sock->send(handleCommand(), out);
        mtx.unlock();
    }
}

void Connection::start()
{
    setActive(true);
    t = std::make_unique<std::thread>(&Connection::listen, this);
}

/* Note to self:
 *
 * Never try to lock mutexs that are
 * already locked. It will cause week
 * long headaches
 */
void Connection::close()
{
    setActive(false);

    t->join();
    sock->close();
}

