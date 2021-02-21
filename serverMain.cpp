/*
 *
 * DSFTP Server
 * Version 0.1
 *
 * Author Rory Hemmings
 *
 */

#include <iostream>
#include <string>
#include <cstdint>
#include <map>

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

// Returns of message length
size_t handleCommand()
{
    switch (SFTP::resolveCommand(in[0]))
    {
        case SFTP::USER:
            LOGGER::DebugLog("User");
            // TODO actually check and add user to vector of ids (sockfds)
            return SFTP::createSuccessResponse(out);
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

        LOGGER::Log(client->Name() + " ", LOGGER::COLOR::CYAN, false);
        LOGGER::Log(in);

        client->send(handleCommand(), out);
        clearBuffers(BUFLEN, in, out);
    }
}

void onConnection(Socket* client)
{
    clearBuffer(BUFLEN, out);
    LOGGER::Log("Client " + client->Name() + " Established Connection", LOGGER::COLOR::GREEN);
    client->send(SFTP::createSuccessResponse(out), out);
}

int main(int argc, char** argv)
{
    ServerSocket serverSocket("127.0.0.1", PORT, &onConnection);
    listen(serverSocket);

    serverSocket.close();
    return 0;
}

