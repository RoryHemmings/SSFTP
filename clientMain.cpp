/*
 *
 * DSFTP Client
 * Version 0.1
 *
 * Author Rory Hemmings
 *
 */

#include <iostream>
#include <string>

#include "logger.h"
#include "socket.h"

int main(int argc, char** argv)
{
  ClientSocket clientSocket("192.168.1.81", 3000);

  LOGGER::Log(">>> ", LOGGER::COLOR::RED, false);
  std::string input;
  while (getline(std::cin, input))
  {
    if (input.compare("exit") == 0)
    {
      return 1;
    }

    clientSocket.sendLine(input);

    LOGGER::Log(">>> ", LOGGER::COLOR::RED, false);
  }

  return 0;
}

