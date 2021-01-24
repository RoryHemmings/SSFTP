/*
 *
 * Server
 * Version 0.3
 *
 * Author: Rory Hemmings
 *
 */

#include <iostream>
#include <string>
#include <cstring>

#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>

#include "utils.h"

#define PORT 3000

int main(int argc, char** argv)
{
  std::cout << std::endl << colorText("Server POC v0.2") << std::endl;
  std::cout << "Author: Rory Hemmings\n" << std::string(15, '-') << std::endl;

  int server_fd, client_socket, valread;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  char buffer[1024] = {0};

  // Create socket file descriptor
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == 0)
  {
    std::cout << "Socket failed" << std::endl;
    return 1;
  }

  // Attatch socket create settings for bind
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
  {
    std::cout << "setsockopt failed" << std::endl;
    return 1;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Attatch socket to port
  if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
  {
    std::cout << "Bind failed (something else could be using currently assigned port: " << PORT << ")" << std::endl;
    std::cout << errno << std::endl;
    return 1;
  }

  if (listen(server_fd, 3) < 0)
  {
    std::cout << "Listen failed" << std::endl;
    return 1;
  }

  // Pauses execution until client is accepted
  client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
  if (client_socket < 0)
  {
    std::cout << "Client Accept failure" << std::endl;
    return 1;
  }
  
  std::cout << colorText("Client Accepted") << std::endl;

  while (true)
  {
    valread = read(client_socket, buffer, 1024);
    std::string message = std::string(buffer);
    
    std::cout << colorText("Incoming message") << ": " << message << std::endl; 
    
    // If message is "exit"
    if (message.compare("exit") == 0) break;

    std::string response(message);

    // add 1 to size because of null termination
    send(client_socket, response.c_str(), response.size() + 1, 0);
    std::cout << colorText("Response") << ": " << response << " " << colorText("sent to client") << std::endl;
    std::cout << std::string(15, '-') << std::endl;
  }

  return 0;
}
