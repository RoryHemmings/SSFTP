/*
 *
 * DSFTP Client
 * Author Rory Hemmings
 *
 */

#include <iostream>
#include <string>
#include <algorithm>
#include <cstdint>
#include <vector>

// To parse args
#include <unistd.h>

#include "sftp.h"
#include "logger.h"
#include "socket.h"
#include "utils.h"

/* Even though global variables are generally considered bad 
 * practice in many situations, it is by far the most simple 
 * and efficient way of doing things for this situation
 *
 * trust me I have tried several different workarounds
 */
char in[BUFLEN];
char out[BUFLEN];

void error(int8_t code)
{
    switch (code)
    {
        case SFTP::MISC_ERROR:
            LOGGER::LogError("Error Occured (error code 1)");
            exit(code);
        case SFTP::INVALID_COMMAND:
            LOGGER::LogError("Invalid SFTP Command (error code 2)");
            exit(code);
        case SFTP::INVALID_USER:
            LOGGER::LogError("Invalid username");
            break;
        case SFTP::INVALID_PASSWORD:
            LOGGER::LogError("Incorrect password");
            break;
    }
}

/*  Takes server response and output buffer (will be sent to server)
 *  returns length of the buffer
 */
int16_t parseCommand()
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
        // TODO fix the null byte problem potentially
        return input.size();
    }
}

std::string authenticateConnection(ClientSocket& sock)
{
    clearBuffers(BUFLEN, in, out);

    std::string username;
    std::string password;

    LOGGER::Log("Enter username for ", LOGGER::COLOR::MAGENTA, false);
    LOGGER::Log(username, LOGGER::COLOR::CYAN, false);
    LOGGER::Log(": ", LOGGER::COLOR::MAGENTA, false);
    getline(std::cin, username);

    LOGGER::Log("Enter password for ", LOGGER::COLOR::MAGENTA, false);
    LOGGER::Log(username, LOGGER::COLOR::CYAN, false);
    LOGGER::Log(": ", LOGGER::COLOR::MAGENTA, false);
    getline(std::cin, password);

    // hexDump("UserBuffer", out, 100);
    sock.send(SFTP::ccUser(out, username, password), out);
    sock.recv(in);

    if (in[0] == SFTP::FAILURE)
    {
        error(in[1]);
        authenticateConnection(sock);
    }

    clearBuffers(BUFLEN, in, out);

    return username;
}

int main(int argc, char** argv)
{
    std::string username;

    ClientSocket sock("127.0.0.1", PORT);

    sock.recv(in);
    if (in[0] == SFTP::SUCCESS)
        LOGGER::DebugLog("Connection Established");
    else
        error(in[1]);  // Server sent invalid response

    clearBuffers(BUFLEN, in, out);

    username = authenticateConnection(sock);
    std::cout << "Finished: " << username << std::endl;

    /*
    do
    {
        len = parseCommand(in, out);
        if (len < 1)
        {
            // Return value serves as error code if its negative
            error(len);
        }

        sock.send(len, out);

        sock.recv(in);
        LOGGER::Log(in, LOGGER::COLOR::CYAN);

        clearBuffers(BUFLEN, in, out);

    } while(true);
    */

    sock.close();

    return 0;
}

