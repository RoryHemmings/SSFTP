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

void listen(ServerSocket& serverSocket)
{
  Socket* client;
  char buffer[1024];

  while (true)
  {
    // Listens for activity on sockets
    // will automatically handle new clients before returning
    client = serverSocket.recv(buffer); 
    LOGGER::Log(client->Name() + " ", LOGGER::COLOR::GREEN, false);
    LOGGER::Log(buffer);
    client->sendLine(buffer);

    for (int16_t i = 0; i < BUFLEN; ++i)
    {
      buffer[i] = '\0';
    }
  }
}

void onConnection(Socket* client)
{
  LOGGER::Log("Client " + client->Name() + " Established Connection", LOGGER::COLOR::CYAN);

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
