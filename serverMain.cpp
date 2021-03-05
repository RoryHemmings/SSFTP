/*
 *
 * SSFTP Server
 * Author Rory Hemmings
 *
 */

#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <future>
#include <map>
#include <cmath>

#include <unistd.h>
#include <limits.h>
#include <pwd.h>
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

    size_t maxlen = BUFLEN - 10; // max number of data bytes allowed in the buffer (9 header bytes and 1 null termination)
    uint32_t end = floor(ret.size() / maxlen); // Will be one less than total number, which means that it is the final index

    int i = 0;
    while (i < ret.size())
    {
        client->send(SFTP::crLs(tempOut, ret.substr(i, maxlen), floor(i / maxlen), end), tempOut);
        i += maxlen;

        clearBuffer(BUFLEN, tempIn);
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

