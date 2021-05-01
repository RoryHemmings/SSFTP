#include "connection.h"

//size_t printWorkingDirectory(Socket* client)
//{
    //User* user = getUserByClient(client);
    //if (user == NULL)
        //return SFTP::createFailureResponse(out, SFTP::NOT_LOGGED_IN);

    //return SFTP::crPwd(out, user->currentDir);
//}

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

//// Returns of message length
//size_t handleCommand(Socket* client)
//{
    //// prevents async from going out of scope and blocking
    //static std::future<void> temp;

    //switch (SFTP::resolveCommand(in[0]))
    //{
    //case SFTP::USER: // sync
        //// Returns length of output buffer
        //return checkPassword(client); 
    //case SFTP::PRWD: // sync
        //// Returns length of output buffer
        //return printWorkingDirectory(client);
    //case SFTP::LIST: // async
        //temp = std::async(std::launch::async, &listDirectory, client, std::string(in+1));
        //return 0;
    //case SFTP::CDIR: // sync
        //return changeUserDirectory(client);
    //case SFTP::MDIR: // sync
        //return createDirectory(client, std::string(in+1));
    //case SFTP::GRAB: // async
        //temp = std::async(std::launch::async, &grabFile, client, std::string(in+1));
        //return 0;
    //case SFTP::PUTF: // async
        //temp = std::async(std::launch::async, &receiveFile);
        //return 0;
    //}

    //return SFTP::createFailureResponse(out, SFTP::INVALID_COMMAND);
//}

//size_t handleCommand(Socket* client)
//{
    //// prevents async from going out of scope and blocking
    //static std::future<void> temp;

    //switch (SFTP::resolveCommand(in[0]))
    //{
    //case SFTP::USER: // sync
        //// Returns length of output buffer
        //return checkPassword(client); 
    //case SFTP::PRWD: // sync
        //// Returns length of output buffer
        //return printWorkingDirectory(client);
    //case SFTP::LIST: // async
        //temp = std::async(std::launch::async, &listDirectory, client, std::string(in+1));
        //return 0;
    //case SFTP::CDIR: // sync
        //return changeUserDirectory(client);
    //case SFTP::MDIR: // sync
        //return createDirectory(client, std::string(in+1));
    //case SFTP::GRAB: // async
        //temp = std::async(std::launch::async, &grabFile, client, std::string(in+1));
        //return 0;
    //case SFTP::PUTF: // async
        //temp = std::async(std::launch::async, &receiveFile);
        //return 0;
    //}

    //return SFTP::createFailureResponse(out, SFTP::INVALID_COMMAND);
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

        LOGGER::HexDump("recv", in, 100);

        mtx.lock(); 
        clearBuffer(BUFLEN, out);
        sock->sendLine("This is epic"); 
        mtx.unlock();

        // TODO and worry about the program closing
        // handleCommand();
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

