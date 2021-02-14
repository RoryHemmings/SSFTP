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

std::string handleCommand(const std::string& in)
{
    std::vector<std::string> cmd = split(in);

    switch (SFTP::resolveCommand(cmd[0]))
    {
        case SFTP::USER:
            LOGGER::DebugLog("User");
            break;
        case SFTP::PASS:
            LOGGER::DebugLog("Password");
            break;
    }

    return "+success";
}

void listen(ServerSocket& serverSocket)
{
    Socket* client;
    std::string in;

    while (true)
    {
        // Listens for activity on sockets
        // will automatically handle new clients before returning
        client = serverSocket.recvLine(in);

        LOGGER::Log(client->Name() + " ", LOGGER::COLOR::CYAN, false);
        LOGGER::Log(in);

        client->sendLine(handleCommand(in));
    }
}

void onConnection(Socket* client)
{
    LOGGER::Log("Client " + client->Name() + " Established Connection", LOGGER::COLOR::GREEN);
    client->sendLine(SFTP::createResponse(SFTP::SUCCESS, "established connection"));
}

int main(int argc, char** argv)
{
    ServerSocket serverSocket("127.0.0.1", PORT, &onConnection);
    listen(serverSocket);

    serverSocket.close();
    return 0;
}

