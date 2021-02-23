/*
 *
 * DSFTP Server
 * Author Rory Hemmings
 *
 */

#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <map>

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

// All currently logged in users
std::map<Socket*, std::string> users;

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
    users[client] = username;
    LOGGER::DebugLog(client->Name() + " Successfully Logged in user: " + username);

    return SFTP::createSuccessResponse(out);
}


// Returns of message length
size_t handleCommand(Socket* client)
{
    switch (SFTP::resolveCommand(in[0]))
    {
        case SFTP::USER:
            // TODO actually check and add user to vector of ids (sockfds)
            return checkPassword(client); // Will modify the out buffer and return length
    }
    
    return SFTP::createFailureResponse(out, SFTP::INVALID_COMMAND);
}

void listen(ServerSocket& serverSocket)
{
    Socket* client;

    while (true)
    {
        // Listens for activity on sockets
        // will automatically handle new clients before returning
        client = serverSocket.recv(in);
        client->send(handleCommand(client), out);

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

