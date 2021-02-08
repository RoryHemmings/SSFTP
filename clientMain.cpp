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

// To parse args
#include <unistd.h>

#include "sftp.h"
#include "logger.h"
#include "socket.h"

void stop(ClientSocket& sock)
{
    sock.close();
    exit(0);
}

void error(ClientSocket& sock, int16_t code)
{
    switch (code)
    {
    case -1:
        LOGGER::LogError("Connection Authentication Failed");
        stop(sock);
        break;
    case -2:
        LOGGER::LogError("Error 2");
        exit(code);
        break;
    }
}

/*  Takes server response and output buffer (will be sent to server)
 *  returns length of the buffer
 */
int16_t parseCommand(ClientSocket& sock, const char* in, char* out)
{
    std::string input;

    LOGGER::Log(">>> ", LOGGER::COLOR::RED, false);
    getline(std::cin, input);

    if (input == "exit")
    {
        stop(sock);
    }
    else
    {
        strcpy(out, input.c_str());
        // TODO fix the null byte problem potentially
        return input.size();
    }
}

int main(int argc, char** argv)
{
    ClientSocket clientSocket("127.0.0.1", PORT);

    int16_t len;
    char in[BUFLEN];
    char out[BUFLEN];

    clientSocket.recv(in);
    if (in[0] == '+')
    {
        LOGGER::DebugLog("Connection Established");
    }
    else
    {
        error(clientSocket, -1);
    }

    do
    {
        len = parseCommand(clientSocket, in, out);
        if (len < 1)
        {
            // Return value serves as error code if its negative
            error(clientSocket, len);
        }

        clientSocket.send(len, out);

        clientSocket.recv(in);
        LOGGER::Log(in, LOGGER::COLOR::CYAN);

        for (int16_t i = 0; i < BUFLEN; ++i)
        {
            in[i] = '\0';
            out[i] = '\0';
        }

    } while(true);

    return 0;
}

