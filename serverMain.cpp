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

#include "sftp.h"
#include "logger.h"
#include "socket.h"
#include "utils.h"

size_t handleCommand(Socket* client, char* in, char* out)
{
  // COMPLETE TEST, DISCARD WHAT IS IN THIS FUNCTION RIGHT NOW
  out = "+User-id valid";
  return strlen(out) + 1;
}

void listen(ServerSocket& serverSocket)
{ 
    Socket* client;
    char in[BUFLEN];
    char out[BUFLEN];

    while (true)
    {
        // Listens for activity on sockets
        // will automatically handle new clients before returning
        client = serverSocket.recv(in);
        LOGGER::Log(client->Name() + " ", LOGGER::COLOR::CYAN, false);
        LOGGER::Log(in);

        client->send(handleCommand(client, in, out), out);

        clearBuffers(BUFLEN, in, out);
    }
}

void onConnection(Socket* client)
{
    LOGGER::Log("Client " + client->Name() + " Established Connection", LOGGER::COLOR::GREEN);

    int len;
    char out[1024];

    len = SFTP::createResponse(out, SFTP::SUCCESS, "established connection");
    client->send(len, out);
}

int main(int argc, char** argv)
{
    ServerSocket serverSocket("127.0.0.1", PORT, &onConnection);
    listen(serverSocket);

    serverSocket.close();

    return 0;
}
