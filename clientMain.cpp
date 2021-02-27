/*
 *
 * SSFTP Client
 * Author Rory Hemmings
 *
 */

#include <iostream>
#include <string>
#include <algorithm>
#include <cstdint>
#include <locale>
#include <map>
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

// TODO resolve command
// Same names but with L prefix

enum L_COMMAND
{
    L_LS,
    L_PWD,
    L_CDIR,
    L_MKDIR,
    L_EXIT,
    INVALID
};

enum L_ERROR
{
    L_INVALID_COMMAND = -1
};

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
    case L_INVALID_COMMAND:
        LOGGER::LogError("Invalid Command");
        break;
    case SFTP::INVALID_RESPONSE:
        LOGGER::LogError("Server responsed with invalid response");
        break;
    case SFTP::NOT_LOGGED_IN:
        LOGGER::LogError("Not Logged In");
        exit(code);
    default:
        LOGGER::LogError("Unkown Error");
        LOGGER::LogError("Code: " + std::to_string(code));
    }
}

L_COMMAND resolveCommand(const std::string& cmd)
{
    static const std::map<std::string, L_COMMAND> cmds
    {
        { "pwd", L_PWD },
        { "ls", L_LS },
        { "cd", L_CDIR },
        { "mkdir", L_MKDIR },
        { "exit", L_EXIT }
    };

    std::string command = cmd;
    std::transform(command.begin(), command.end(), command.begin(), tolower);

    auto it = cmds.find(command);
    if (it != cmds.end())
        return it->second;

    // No command found
    return INVALID;
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
        username = authenticateConnection(sock); // Run recursively until success
    }

    clearBuffers(BUFLEN, in, out);

    return username;
}

int16_t parsePwd()
{
    if (in[0] == SFTP::SUCCESS)
    {
        std::string p(in+1);
        LOGGER::Log(p);
    }
    else if (in[0] = SFTP::FAILURE)
        error(in[1]);
    else
        error(SFTP::INVALID_RESPONSE);

    return 0;
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

    username = authenticateConnection(sock);

    std::string input;
    size_t len = 0;
    int16_t err;
    do
    {
        clearBuffers(BUFLEN, in, out);

        LOGGER::Log(">>> ", LOGGER::COLOR::RED, false);
        getline(std::cin, input);

        std::vector<std::string> cmd = split(input);

        switch (resolveCommand(cmd[0]))
        {
        case L_PWD:
            len = SFTP::ccPwd(out);
            sock.send(len, out);
            sock.recv(in);
            err = parsePwd();
        case L_LS:
            break;
        case L_CDIR:
            break;
        case L_MKDIR:
            break;
        case L_EXIT:
            LOGGER::Log("bye");
            exit(0);
        case INVALID:
            err = L_INVALID_COMMAND;
            break;
        }

        if (err < 0)
        {
            error(err);
        }

    } while(true);

    sock.close();

    return 0;
}

