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
    LOGGER::DebugLog("recieve");
    // client->sendLine("Response");
  }
}

int main(int argc, char** argv)
{
  ServerSocket serverSocket("127.0.0.1", 3000);
  listen(serverSocket);

  serverSocket.close();

  return 0;
}
