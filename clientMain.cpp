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
#include "utils.h"

void error(int16_t code)
{
    switch (code)
    {
    case -1:
        LOGGER::LogError("Connection Authentication Failed");
        exit(code);
    case -2:
        LOGGER::LogError("Invalid user-id");
        exit(code);
    }
}

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
        // TODO fix the null byte problem potentially
        return input.size();
    }
}

int16_t authenticateConnection(char* in, char* out, ClientSocket& sock, const char* username)
{
    sock.send(SFTP::createMessage(out, SFTP::USER, 1, username), out);
    sock.recv(in);

    // Response code
    char response = in[0];
    switch (response)
    {
    case '!':
        // Account doesn't require password
        LOGGER::DebugLog("Login Successful!");
        return 0;
    case '+':
        // Follow through to get password from user
        break;
    case '-':
        // Username is incorrect
        return -1;
    }

    clearBuffers(BUFLEN, in, out);

    std::string pswd;
    LOGGER::Log("Enter password for ", LOGGER::COLOR::MAGENTA, false);
    LOGGER::Log(username, LOGGER::COLOR::BLUE, false);
    LOGGER::Log(": ", LOGGER::COLOR::MAGENTA, false);
    getline(std::cin, pswd);

    SFTP::createMessage(out, SFTP::PASS, 1, pswd.c_str());
    LOGGER::DebugLog(out);
}

int main(int argc, char** argv)
{
    int16_t len;
    char in[BUFLEN];
    char out[BUFLEN];

    ClientSocket sock("127.0.0.1", PORT);

    sock.recv(in);
    if (in[0] == '+')
    {
        LOGGER::DebugLog("Connection Established");
    }
    else
    {
        // Server didn't send welcome message
        error(-1);
    }

    if (authenticateConnection(in, out, sock, "test_user") < 0)
    {
        // Invalid user-id
        error(-2);
    }

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

    return 0;
}

