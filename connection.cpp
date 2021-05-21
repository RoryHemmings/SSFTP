#include <sys/stat.h>
#include <pwd.h>
#include <shadow.h>

#include <fstream>

#include "connection.h"

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
void Connection::checkPassword()
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
    {
        sock->send(SFTP::createFailureResponse(out, SFTP::INVALID_USER), out);
        return;
    }

    spwd = getspnam(username.c_str());
    if (spwd == NULL && errno == EACCES)
    {
        LOGGER::LogError("No Access to Shadow file");
        sock->send(SFTP::createFailureResponse(out, SFTP::MISC_ERROR), out);
        return;
    }

    if (spwd != NULL)           // If there is a shadow password record
        pwd->pw_passwd = spwd->sp_pwdp;     // Use the shadow password

    encrypted = crypt(password.c_str(), pwd->pw_passwd);
    if (encrypted == NULL)
    {
        LOGGER::LogError("crpyt() failed");
        sock->send(SFTP::createFailureResponse(out, SFTP::MISC_ERROR), out);
        return;
    }

    bool authOk = strcmp(encrypted, pwd->pw_passwd) == 0;
    if (!authOk)
    {
        sock->send(SFTP::createFailureResponse(out, SFTP::INVALID_PASSWORD), out);
        return;
    }

    // Login Successful
    user = { username, pwd->pw_dir, pwd->pw_dir };
    LOGGER::DebugLog(this->Name() + " Successfully Logged in user: " + username);

    sock->send(SFTP::createSuccessResponse(out), out);
}

void Connection::printWorkingDirectory()
{
    if (!this->isLoggedIn())
    {
        sock->send(SFTP::createFailureResponse(out, SFTP::NOT_LOGGED_IN), out);
        return;
    }

    sock->send(SFTP::crPwd(out, user.currentDir), out);
}

void Connection::listDirectory()
{
    std::string path = std::string(in+1);

    if (!this->isLoggedIn())
    {
        sock->send(SFTP::createFailureResponse(out, SFTP::NOT_LOGGED_IN), out);
        return;
    }

    std::string finalPath = generateNewPath(user.currentDir, path);

    std::string::size_type homeDirLen = user.homeDir.size();
    if (finalPath.substr(0, homeDirLen) != user.homeDir)
    {
        sock->send(SFTP::createFailureResponse(out, SFTP::ACCESS_DENIED), out);
        return;
    }

    std::string ret = exec("ls -la " + finalPath);
    if (ret.size() == 0)
    {
        sock->send(SFTP::createFailureResponse(out, SFTP::COMMAND_EXECUTION_FAILED, "cannot find " + finalPath), out);
        return;
    }

    size_t maxlen = BUFLEN - 2; // max number of data bytes allowed in the buffer (1 header byte and 1 null termination)
    uint32_t numPackets = floor(ret.size() / maxlen) + 1; // total number of packets

    sock->send(SFTP::crLsPrimary(out, numPackets), out);

    int i = 0;
    while (i < ret.size())
    {
        clearBuffers(BUFLEN, in, out);

        // Waits for go-ahead to continue sending
        sock->recv(in, BUFLEN);
        if (in[0] != SFTP::SUCCESS)
        {
            sock->send(SFTP::createFailureResponse(out, SFTP::INVALID_RESPONSE), out);
            break;
        }

        sock->send(SFTP::crLs(out, ret.substr(i, maxlen)), out);
        i += maxlen;
    }
}

void Connection::changeUserDirectory()
{
    if (!this->isLoggedIn())
    {
        sock->send(SFTP::createFailureResponse(out, SFTP::NOT_LOGGED_IN), out);
        return;
    }

    std::string path(in+1); // null termination for command acts as null termination for string
    std::string newPath = generateNewPath(user.currentDir, path);

    // Check if path is valid
    struct stat buffer;
    if (stat(newPath.c_str(), &buffer) != 0)
    {
        sock->send(SFTP::createFailureResponse(out, SFTP::INVALID_PATH), out);
        return;
    }

    std::string finalPath = newPath;

    // Check that path is within access bubble
    std::string::size_type homeDirLen = user.homeDir.size();
    if (newPath.substr(0, homeDirLen) != user.homeDir)
    {
        sock->send(SFTP::createFailureResponse(out, SFTP::ACCESS_DENIED), out);
        return;
    }

    user.currentDir = finalPath;
    sock->send(SFTP::crCd(out, finalPath), out);
}

/* Bugs:
 * When you try to make a new directory over one that
 * already exists, it breaks
 * check to make sure that it doesn't already exist before writing
 */
void Connection::createDirectory()
{
    if (!this->isLoggedIn())
        sock->send(SFTP::createFailureResponse(out, SFTP::NOT_LOGGED_IN), out);

    std::string name = std::string(in+1);
    std::string path = generateNewPath(user.currentDir, name);

    // Check that path is within access bubble
    std::string::size_type homeDirLen = user.homeDir.size();
    if (path.substr(0, homeDirLen) != user.homeDir)
        sock->send(SFTP::createFailureResponse(out, SFTP::ACCESS_DENIED), out);

    std::string ret = exec("mkdir -p " + path);

    sock->send(SFTP::crMkDir(out, ret), out);
}

// Takes string for file path to prevent buffer issues
void Connection::grabFile()
{
    if (!this->isLoggedIn())
    {
        sock->send(SFTP::createFailureResponse(out, SFTP::NOT_LOGGED_IN), out);
        return;
    }

    std::string path;
    std::string relativePath(in+1);
    path = generateNewPath(user.currentDir, relativePath); // takes relative path and makes it absolute

    std::ifstream file;
    file.open(path, std::ios::binary);

    if (!file.is_open())
    {
        sock->send(SFTP::createFailureResponse(out, SFTP::FAILED_TO_OPEN_FILE), out);
        return;
    }

    file.seekg(0, file.end);
    int fileSize = file.tellg();
    file.seekg(0, file.beg);

    uint32_t totalPackets = floor(fileSize / (BUFLEN - 4)) + 1;

    sock->send(SFTP::crGrabPrimary(out, totalPackets, relativePath), out);

    while (!file.eof())
    {
        clearBuffers(BUFLEN, in, out);

        // Waits for go-ahead to continue sending
        sock->recv(in);
        if (in[0] != SFTP::SUCCESS)
        {
            file.close();
            return;
        }

        size_t i = 3; // Starts at 3 to leave space for status byte and length
        while (i < BUFLEN - 1) // Leave space for null termination
        {
            char c = (char) file.get();
            if (file.eof())
                break;

            out[i] = c;
            ++i;
        }

        uint16_t length = i - 3;

        /* crGrab() works differently from all other response facotries
         * Instead of overwriting buffer, it only changes the status byte
         * and the length

         * This is done to prevent copying of the entire buffer which would be inefficient.
         * Therefore, I have to modify the buffer before passing it to the function
         */
        sock->send(SFTP::crGrab(out, length), out);
    }
    file.close();
}

// Returns of message length
void Connection::handleCommand()
{
    switch (SFTP::resolveCommand(in[0]))
    {
    case SFTP::USER:
        checkPassword();
        return;
    case SFTP::PRWD:
        printWorkingDirectory();
        return;
    case SFTP::LIST:    // Async
        listDirectory();
        return;
    case SFTP::CDIR:
        changeUserDirectory();
        return;
    case SFTP::MDIR:
        createDirectory();
        return;
    case SFTP::GRAB:    // Aysnc
        grabFile();
        return;
    case SFTP::PUTF:    // Async
        // temp = std::async(std::launch::async, &receiveFile);
        return;
    }

    sock->send(SFTP::createFailureResponse(out, SFTP::INVALID_COMMAND), out);
}

void Connection::listen()
{
    int code;

    clearBuffers(BUFLEN, in, out);
    sock->send(SFTP::createSuccessResponse(out), out);

    while (running)
    {
        // Recieve input
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

        // Handle input
        mtx.lock();
        clearBuffer(BUFLEN, out);
        handleCommand();
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


// TODO
// ls return failure string response (give more detailed error messages upon failure)
// make grab correctly handle errors when invalid path is specified
// add a -out option for grap and putf (sort out path issues when grabbing long paths)
//
// fix permissions - change owner programatically (chown)
