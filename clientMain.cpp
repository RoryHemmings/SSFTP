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
    case -2:
        LOGGER::LogError("Invalid user-id");
    }

    exit(code);
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

int16_t authenticateConnection(ClientSocket& sock, const std::string& username)
{
    std::string response;
    std::string pswd;

    sock.sendLine(SFTP::createCommand(SFTP::USER, 1, username));
    response = sock.recvLine();

    // Response code
    switch (response[0])
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

    LOGGER::Log("Enter password for ", LOGGER::COLOR::MAGENTA, false);
    LOGGER::Log(username, LOGGER::COLOR::BLUE, false);
    LOGGER::Log(": ", LOGGER::COLOR::MAGENTA, false);
    getline(std::cin, pswd);

    sock.sendLine(SFTP::createCommand(SFTP::PASS, 1, pswd));
    LOGGER::DebugLog(sock.recvLine());
}

int main(int argc, char** argv)
{
    std::string username = "sftp_user";
    std::string in;

    ClientSocket sock("127.0.0.1", PORT);

    in = sock.recvLine();
    if (in[0] == '+')
        LOGGER::DebugLog("Connection Established");
    else
        error(-1);  // Server sent invalid response

    if (authenticateConnection(sock, username) < 0)
        error(-2);  // Invalid user-id

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

