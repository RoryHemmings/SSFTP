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

#include "sftp.h"
#include "logger.h"
#include "socket.h"
#include "utils.h"

/* Even though global variables are generally considered bad
 * practice in many situations, it is by far the most simple
 * and efficient way of doing things for this situation
 *
 * trust me I have tried several different workarounds
 */
char in[BUFLEN];
char out[BUFLEN];

struct User
{
    std::string username;
    std::string homeDir;
    std::string currentDir;
};

// All currently logged in users
std::map<Socket*, User> users;

size_t checkPassword(Socket* client)
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
    users[client] = { username, pwd->pw_dir, pwd->pw_dir };
    LOGGER::DebugLog(client->Name() + " Successfully Logged in user: " + username);

    return SFTP::createSuccessResponse(out);
}

std::string exec(const std::string& command)
{
    char buffer[128];
    std::string ret = "";
    FILE* pipe;

    pipe = popen(command.c_str(), "r");
    if (!pipe)
        return "";
    try
    {
        while (fgets(buffer, sizeof buffer, pipe) != NULL)
        {
            ret += buffer;
        }
    }
    catch (...)
    {
        pclose(pipe);
        return "";
    }

    pclose(pipe);
    return ret;
}

User* getUserByClient(Socket* client)
{
    auto iter = users.find(client);
    if (iter == users.end())
        return NULL;

    return &(iter->second);
}

bool checkStatus()
{
    if (in[0] == SFTP::SUCCESS)
        return true;

    return false;
}

size_t printWorkingDirectory(Socket* client)
{
    User* user = getUserByClient(client);
    if (user == NULL)
        return SFTP::createFailureResponse(out, SFTP::NOT_LOGGED_IN);

    return SFTP::crPwd(out, user->currentDir);
}

void listDirectory(Socket* client)
{
    // Temporary buffers so that the async buffer doesn't share with the sync one
    char tempIn[BUFLEN];
    char tempOut[BUFLEN];

    User* user = getUserByClient(client);
    if (user == NULL)
    {
        client->send(SFTP::createFailureResponse(tempOut, SFTP::NOT_LOGGED_IN), tempOut);
        return;
    }

    std::string ret = exec("ls -la " + user->currentDir);
    if (ret.size() == 0)
    {
        client->send(SFTP::createFailureResponse(tempOut, SFTP::COMMAND_EXECUTION_FAILED), tempOut);
        return;
    }

    size_t maxlen = BUFLEN - 2; // max number of data bytes allowed in the buffer (1 header byte and 1 null termination)
    uint32_t numPackets = floor(ret.size() / maxlen) + 1; // total number of packets 

    client->send(SFTP::crLsPrimary(tempOut, numPackets), tempOut);

    int i = 0;
    while (i < ret.size())
    {
        client->send(SFTP::crLs(tempOut, ret.substr(i, maxlen)), tempOut);
        i += maxlen;

        clearBuffer(BUFLEN, tempIn);
    }
}

bool isSlash(char c)
{
    return c == '/';
}

/*
 *  Recursively removes dot pairs
 */
void removeDotPairs(std::string& s)
{
    std::vector<int> slashIndicies;
    for (std::string::size_type i = 0; i < s.size(); ++i)
    {
        if (s[i] == '/')
            slashIndicies.push_back(i);
    }
    
    for (auto iter = slashIndicies.begin(); iter != slashIndicies.end(); ++iter)
    {
        if (s[*(iter)+1] == '.') 
        {
            if (s[*(iter)+2] == '.') 
            {
                if (s[*(iter)+3] == '/') // whole path is wrapped in /, so this shouldn't violate bounds
                {
                    // succesfully found valid /..
                    // delete from last slash index to this slash index plus 3
                    
                    s.erase(*(iter - 1), ((*iter) + 3) - (*(iter - 1)));

                    /* Since slashIndicies is invalid, now I can
                     * just recursively call the function to start over
                     * and Eventually remove all pairs
                     */
                    removeDotPairs(s);
                    break;
                }
            }
        }
    }
}

std::string generateNewPath(const std::string& path, const std::string& next)
{
    std::string fullPath = "/" + path + "/" + next + "/";
    std::string ret = "";
    
    // Remove double (or more) slashes
    // this way it will work regardless 
    // of when people put slashes in their paths
    auto i = fullPath.begin();
    while (i != fullPath.end())
    {
        ret += *i;
        if ((*i) == '/')
            i = std::find_if_not(++i, fullPath.end(), isSlash); // Skip all preceding slashes until regular char is found
        else
            ++i;
    }

    // Apply instances of /..
    removeDotPairs(ret);

    // Remove ending slash
    if (ret[ret.size() - 1] == '/')
        ret.erase(ret.size() - 1);

    return ret;
}

size_t changeUserDirectory(Socket* client)
{
    User* user = getUserByClient(client);
    if (user == NULL)
        return SFTP::createFailureResponse(out, SFTP::NOT_LOGGED_IN);

    std::string path(in+1); // null termination for command acts as null termination for string
    std::string newPath = generateNewPath(user->currentDir, path); 

    // Check if path is valid
    struct stat buffer;
    if (stat(newPath.c_str(), &buffer) != 0)
        return SFTP::createFailureResponse(out, SFTP::INVALID_PATH); 

    std::string finalPath = newPath;

    // Check that path is within access bubble
    std::string::size_type homeDirLen = user->homeDir.size();
    if (newPath.substr(0, homeDirLen) != user->homeDir)
        finalPath = user->homeDir;

    user->currentDir = finalPath;
    return SFTP::crCd(out, finalPath);
}

// Takes string for file path to prevent buffer issues
void grabFile(Socket* client, const std::string& filePath)
{
    // Temporary buffers so that the async buffer doesn't share with the sync one
    char tempIn[BUFLEN];
    char tempOut[BUFLEN];

    User* user = getUserByClient(client);
    if (user == NULL)
    {
        client->send(SFTP::createFailureResponse(tempOut, SFTP::NOT_LOGGED_IN), tempOut);
        return;
    }

    std::string path;
    path = generateNewPath(user->currentDir, filePath);
    
    std::ifstream file;
    file.open(path, std::ios::binary);

    file.seekg(0, file.end);
    int fileSize = file.tellg();
    file.seekg(0, file.beg);

    uint32_t totalPackets = floor(fileSize / (BUFLEN - 4)) + 1;

    client->send(SFTP::crGrabPrimary(tempOut, totalPackets, filePath), tempOut);

    if (file.is_open())
    {
        while (!file.eof())
        {
            size_t i = 3; // Starts at 3 to leave space for status byte and length 
            clearBuffer(BUFLEN, tempOut);
            while (i < BUFLEN - 1) // Leave space for null termination
            {
                char c = (char) file.get();
                if (file.eof())
                    break;

                tempOut[i] = c;
                ++i;
            }
            
            uint16_t length = i - 3;
            client->send(SFTP::crGrab(tempOut, length), tempOut);
        }
        file.close();
    }
    else
    {
        client->send(SFTP::createFailureResponse(tempOut, SFTP::FAILED_TO_OPEN_FILE), tempOut);
        return;
    }
}

// Returns of message length
size_t handleCommand(Socket* client)
{
    // prevents async from going out of scope and blocking
    static std::future<void> temp;

    switch (SFTP::resolveCommand(in[0]))
    {
    case SFTP::USER: // sync
        // Returns length of output buffer
        return checkPassword(client); 
    case SFTP::PRWD: // sync
        // Returns length of output buffer
        return printWorkingDirectory(client);
    case SFTP::LIST: // async
        temp = std::async(std::launch::async, &listDirectory, client);
        return 0;
    case SFTP::CDIR: // sync
        return changeUserDirectory(client);
    case SFTP::GRAB: // async
        temp = std::async(std::launch::async, &grabFile, client, std::string(in+1));
        return 0;
    }

    return SFTP::createFailureResponse(out, SFTP::INVALID_COMMAND);
}

void listen(ServerSocket& serverSocket)
{
    Socket* client;
    size_t len = 0;

    while (true)
    {
        // Listens for activity on sockets
        // will automatically handle new clients before returning
        client = serverSocket.recv(in);

        /* To handle communications that take more than one 
         * stage, I will use asynchronous functions whenever a
         * mutlti stage communication takes place (ie. file transfer
         * or other long command)
         *
         * if handleCommand() returns 0, then it should
         * just skip over the send command
         */

        len = handleCommand(client);

        /* if request was synchronous */
        if (len > 0)
            client->send(len, out);

       clearBuffers(BUFLEN, in, out);
    }
}

void onConnect(Socket* client)
{
    clearBuffer(BUFLEN, out);
    LOGGER::Log("Client " + client->Name() + " Established Connection", LOGGER::COLOR::GREEN);
    client->send(SFTP::createSuccessResponse(out), out);
}

void onDisconnect(Socket* client)
{
    auto user = users.find(client);
    if (user != users.end())
        users.erase(user);

    LOGGER::Log("Client disconnected: " + client->Name(), LOGGER::COLOR::YELLOW);
}


int main(int argc, char** argv)
{
    ServerSocket serverSocket("127.0.0.1", PORT, &onConnect, &onDisconnect);
    listen(serverSocket);

    serverSocket.close();
    return 0;
}

