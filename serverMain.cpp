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

#include "socket.h"

#define BUF_LEN 1024

void listen(ServerSocket& serverSocket)
{
  Socket* client;
  char buffer[1024];

  while (true)
  {
    // Listens for activity on sockets
    // will automatically handle new clients before returning
    client = serverSocket.recv(buffer); 
    LOGGER::Log(client->Name() + " ", LOGGER::COLOR::CYAN, false);
    LOGGER::Log(buffer);
    client->sendLine(buffer);

    for (int16_t i = 0; i < BUF_LEN; ++i)
    {
      buffer[i] = '\0';
    }
  }
}

int main(int argc, char** argv)
{
  ServerSocket serverSocket("127.0.0.1", 3000);
  listen(serverSocket);

  serverSocket.close();

  return 0;
}
