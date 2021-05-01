/*
 *
 * SSFTP Server
 * Author Rory Hemmings
 *
 */

#include <iostream>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <future>
#include <map>
#include <cmath>
#include <fstream>

#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <sys/stat.h>
#include <shadow.h>
#include <signal.h>

#include "sftp.h"
#include "logger.h"
#include "socket.h"
#include "connection.h"
#include "utils.h"

/* Even though global variables are generally considered bad
 * practice in many situations, it is by far the most simple
 * and efficient way of doing things for this situation
 */
// char in[BUFLEN];
// char out[BUFLEN];

//struct User
//{
    //std::string username;
    //std::string homeDir;
    //std::string currentDir;
//};

// All currently logged in users
// std::map<Socket*, User> users;

// Keeps track of the loose connections 
std::vector<Connection*> connections;
const unsigned int maxConnections = 1024;

//size_t checkPassword(Socket* client)
//{
    //char *encrypted, *p;
    //struct passwd *pwd;
    //struct spwd *spwd;
    //uint16_t usernameLength, passwordLength;

    //// Get username and password lengths (2 bytes long)
    //memcpy(&usernameLength, in+1, 2);
    //memcpy(&passwordLength, in+3, 2);

    //std::string username(in+5, usernameLength);
    //std::string password(in+5 + usernameLength, passwordLength);

    //pwd = getpwnam(username.c_str());
    //if (pwd == NULL)
        //return SFTP::createFailureResponse(out, SFTP::INVALID_USER);

    //spwd = getspnam(username.c_str());
    //if (spwd == NULL && errno == EACCES)
    //{
        //LOGGER::LogError("No Access to Shadow file");
        //return SFTP::createFailureResponse(out, SFTP::MISC_ERROR);
    //}

    //if (spwd != NULL)           // If there is a shadow password record
        //pwd->pw_passwd = spwd->sp_pwdp;     // Use the shadow password

    //encrypted = crypt(password.c_str(), pwd->pw_passwd);
    //if (encrypted == NULL)
    //{
        //LOGGER::LogError("crpyt() failed");
        //return SFTP::createFailureResponse(out, SFTP::MISC_ERROR);
    //}

    //bool authOk = strcmp(encrypted, pwd->pw_passwd) == 0;
    //if (!authOk)
        //return SFTP::createFailureResponse(out, SFTP::INVALID_PASSWORD);

    //// Login Successful
    //users[client] = { username, pwd->pw_dir, pwd->pw_dir };
    //LOGGER::DebugLog(client->Name() + " Successfully Logged in user: " + username);

    //return SFTP::createSuccessResponse(out);
//}

//User* getUserByClient(Socket* client)
//{
    //auto iter = users.find(client);
    //if (iter == users.end())
        //return NULL;

    //return &(iter->second);
//}

//bool checkStatus()
//{
    //if (in[0] == SFTP::SUCCESS)
        //return true;

    //return false;
//}

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

//void listen(ServerSocket& serverSocket)
//{
    //Socket* client;
    //size_t len = 0;

    //while (true)
    //{
        //// Listens for activity on sockets
        //// will automatically handle new clients before returning
        //client = serverSocket.recv(in);

        /* To handle communications that take more than one 
         * stage, I will use asynchronous functions whenever a
         * mutlti stage communication takes place (ie. file transfer
         * or other long command)
         *
         * if handleCommand() returns 0, then it should
         * just skip over the send command
         */

        //len = handleCommand(client);

        //[> if request was synchronous <]
        //if (len > 0)
            //client->send(len, out);

       //clearBuffers(BUFLEN, in, out);
    //}
//}

//void onConnect(Socket* client)
//{
    //clearBuffer(BUFLEN, out);
    //LOGGER::Log("Client " + client->Name() + " Established Connection", LOGGER::COLOR::GREEN);
    //client->send(SFTP::createSuccessResponse(out), out);
//}

//void onDisconnect(Socket* client)
//{
    //auto user = users.find(client);
    //if (user != users.end())
        //users.erase(user);

    //LOGGER::Log("Client disconnected: " + client->Name(), LOGGER::COLOR::YELLOW);
//}
//

void cleanConnections()
{
    std::vector<Connection*>::size_type i = 0;
    while (i < connections.size())
    {
        if (!(connections[i]->isActive()))
        {
            std::cout << "Deleting: " << connections[i]->Name() << std::endl;
            Connection* c = connections[i];
            connections.erase(connections.begin() + i);

            delete c;
            continue; // Keeps index valid
        }
        ++i;
    }
}

void listen(ServerSocket& server)
{
    char out[BUFLEN];

    LOGGER::DebugLog("Listening for clients...");
    while (true)
    {
        Socket* sock;
        sock = server.accept();
        if (sock < 0)
            LOGGER::LogError("Coundn't accept connection");
        
        if (connections.size() + 1 > maxConnections)
        {
            clearBuffer(BUFLEN, out);
            sock->send(SFTP::createFailureResponse(out, SFTP::SERVER_FULL), out);
            sock->close();

            delete sock;
            continue;
        }

        // Deletes any inactive connection objects
        cleanConnections();
        
        /* I allocate the Connection object to the heap
         * so that it doesn't go out of scope and get destroyed
         */
        Connection* connection = new Connection(sock);
        connections.push_back(connection);
        connection->start();
        
        LOGGER::Log("Client " + sock->Name() + " Established Connection", LOGGER::COLOR::GREEN);
        std::cout << "Number of Connections: " << connections.size() << std::endl;
    }

    // TODO maybe add cmd interface for server running on a seperate thread
}

void onExit(int signum)
{
    std::cout << std::endl;
    cleanConnections();

    std::cout << "\nbye" << std::endl;
    exit(signum);
}

int main(int argc, char** argv)
{
    signal(SIGINT, &onExit);

    ServerSocket serverSocket("127.0.0.1", PORT);
    listen(serverSocket);

    serverSocket.close();
    return 0;
}

