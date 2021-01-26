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
#include <algorithm>
#include <cstdlib>
#include <cstdint>

#include "logger.h"
#include "socket.h"

#define BUF_LEN 1024

/*  Takes server response and output buffer (will be sent to server)
 *  returns length of the buffer
 */
int16_t parseCommand(const char* in, char* out)
{ 
  std::string input;

  LOGGER::Log(">>> ", LOGGER::COLOR::RED, false);
  getline(std::cin, input);

  if (input == "exit")
  {
    exit(0);
  }
  else
  {
    strcpy(out, input.c_str());
    return input.size();
  }
}

void error(int16_t code)
{
  switch (code)
  {
    case -1:
      LOGGER::LogError("Error 1");
      break;
    case -2:
      LOGGER::LogError("Error 2");
      exit(code);
      break;
  }
}

int main(int argc, char** argv)
{
  ClientSocket clientSocket("127.0.0.1", 3000);

  int16_t len;
  char in[BUF_LEN];
  char out[BUF_LEN];

  do
  {
    clientSocket.recv(in);
    LOGGER::Log(in, LOGGER::COLOR::CYAN);

    len = parseCommand(in, out);
    if (len < 1)
    {
      // Return value serves as error code if its negative
      error(len);
    }

    clientSocket.send(len, out);

    for (int16_t i = 0; i < BUF_LEN; ++i)
    {
      in[i] = '\0';
      out[i] = '\0';
    }
    
  } while(true);

  return 0;
}

