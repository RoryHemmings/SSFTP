/*
 *
 * Client
 * Version 0.3
 *
 * Author: Rory Hemmings
 *
 */

#include <iostream>
#include <string>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "utils.h"

#define PORT 3000

int main(int argc, char** argv) {
  std::cout << std::endl << colorText("Client POC v0.2") << std::endl;
  std::cout << "Author: Rory Hemmings\n" << std::string(15, '-') << std::endl;

  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  
  const char* message = "Hello from client";
  char buffer[1024] = {0};

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) 
  {
    std::cout << "Socket creation error" << std::endl;
    return 1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) 
  {
    std::cout << "Invalid adress/ Adress not supported" << std::endl;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    std::cout << "Connection Failed" << std::endl;
  }

  while (true)
  {
    // Get line of user input
    std::string cmd;
    std::cout << colorText(">>> ");
    std::getline(std::cin, cmd);

    // If command is "exit"
    if (cmd.compare("exit") == 0) break;

    send(sock, cmd.c_str(), cmd.size() + 1, 0);
    std::cout << colorText("Sent message") << ": " << cmd << std::endl;

    valread = read(sock, buffer, 1024);

    std::string response(buffer);
    std::cout << colorText("From server") << ": " << response << std::endl;
  }

  return 0;
}
