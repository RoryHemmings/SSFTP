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
#include <cmath>
#include <fstream>

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

enum L_COMMAND
{
    L_LS,
    L_PWD,
    L_CDIR,
    L_GRAB,
    L_MKDIR,
    L_EXIT,
    INVALID
};

enum L_ERROR
{
    L_INVALID_COMMAND = -1,
    L_FAILED_TO_OPEN_FILE = -2
};

/* Positive codes are remote and negative errors are local */
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
    case SFTP::INVALID_RESPONSE:
        LOGGER::LogError("Server responsed with invalid response");
        break;
    case SFTP::NOT_LOGGED_IN:
        LOGGER::LogError("Not Logged In");
        exit(code);
    case SFTP::COMMAND_EXECUTION_FAILED:
        LOGGER::LogError("Failed to execute command");
        break;
    case SFTP::INVALID_PATH:
        LOGGER::LogError("Invalid Path");
        break;
    case SFTP::FAILED_TO_OPEN_FILE:
        LOGGER::LogError("Couldn't open file");
        break;
    case L_INVALID_COMMAND:
        LOGGER::LogError("Invalid Command");
        break;
    case L_FAILED_TO_OPEN_FILE:
        LOGGER::LogError("Failed to open file locally");
        break;
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
        { "grab", L_GRAB },
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

/* Returns true if success and false if failure */
bool checkStatus()
{
    if (in[0] == SFTP::SUCCESS)
    {
        return true;
    }
    else if (in[0] == SFTP::FAILURE)
    {
        error(in[1]);
        return false;
    }
    else
    {
        error(SFTP::INVALID_RESPONSE);
        return false;
    }
}

void parsePwd(ClientSocket& sock)
{
    sock.recv(in);
    if (checkStatus())
    {
        // in+1 because in+0 is response code
        std::string p(in+1);
        LOGGER::Log(p);
    }
    else 
    {
        // Error was already handled by checkStatus()
        return;
    }
}

void parseLs(ClientSocket& sock)
{
    std::string output = "";
    uint32_t index = 0, end;

    // Get primary information
    sock.recv(in);

    if (checkStatus()) // Handles errors
    {
        // Get the uint32_t value: end
        memcpy(&end, in+1, 4);

        clearBuffers(BUFLEN, in, out);
    }

    do
    {
        // Get secondary information
        sock.recv(in);

        if (checkStatus()) // Handles errors
        {
            // Append output to output string
            output += std::string(in+1);
            clearBuffers(BUFLEN, in, out);
        }
        else 
        {
            // Error was already handled by checkStatus()
            return;
        }

        ++index;
    }
    while (index < end);

    LOGGER::Log(output, LOGGER::WHITE, false);
}

void parseCd(ClientSocket& sock, std::string& currentDir)
{
    sock.recv(in);
    if (checkStatus())
    {
        std::string newPath(in+1);
        currentDir = newPath;
    }
    else 
    {
        // Error was already handled by checkStatus()
        return;
    }
}

void parseGrab(ClientSocket& sock)
{
    uint32_t index = 0, end;
    std::string path;
    
    // Get primary information
    sock.recv(in);

    if (checkStatus()) // Handles errors
    {
        char* outPath;

        // Get the uint32_t value: end
        memcpy(&end, in+1, 4);
        path = std::string(in+5);

        clearBuffers(BUFLEN, in, out);
    }

    std::ofstream outfile;
    outfile.open(path, std::ios::binary);

    if (!outfile.is_open())
    {
        error(L_FAILED_TO_OPEN_FILE);
        return;
    } 

    do
    {
        // Get secondary information
        sock.recv(in);

        if (checkStatus()) // Handles errors
        {
            // Append output to output string
            uint16_t length;
            memcpy(&length, in+1, 2);

            LOGGER::HexDump("in", in+3, length);
            outfile.write(in+3, length);

            clearBuffers(BUFLEN, in, out);
        }
        else 
        {
            // Error was already handled by checkStatus()
            return;
        }

        ++index;
    }
    while (index < end);

    outfile.close();
}

int main(int argc, char** argv)
{
    std::string username;
    std::string currentDir;

    ClientSocket sock("127.0.0.1", PORT);

    sock.recv(in);
    if (in[0] == SFTP::SUCCESS)
        LOGGER::DebugLog("Connection Established");
    else
        error(in[1]);  // Server sent invalid response

    username = authenticateConnection(sock);

    std::string input;
    size_t len = 0;
    do
    {
        clearBuffers(BUFLEN, in, out);

        LOGGER::Log(currentDir + "$ ", LOGGER::COLOR::RED, false);
        getline(std::cin, input);

        std::vector<std::string> cmd = split(input);

        switch (resolveCommand(cmd[0]))
        {
        case L_PWD:
            sock.send(SFTP::ccPwd(out), out);
            parsePwd(sock);
            break;
        case L_LS:
            sock.send(SFTP::ccLs(out), out);
            parseLs(sock);
            break;
        case L_CDIR:
            if (cmd.size() != 2)
            {
                LOGGER::Log("Usage: cd <path>");                
                break;
            }
            sock.send(SFTP::ccCd(out, cmd[1]), out);
            parseCd(sock, currentDir);
            break;
        case L_GRAB:
            if (cmd.size() != 2)
            {
                LOGGER::Log("Usage: grab <filename>");
                break;
            }
            sock.send(SFTP::ccGrab(out, cmd[1]), out);
            parseGrab(sock);
            break;
        case L_MKDIR:
            break;
        case L_EXIT:
            LOGGER::Log("bye");
            exit(0);
        case INVALID:
            error(L_INVALID_COMMAND);
            break;
        }
    } while(true);

    sock.close();

    return 0;
}

