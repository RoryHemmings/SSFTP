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
#include <libgen.h>
#include <sys/stat.h>

#include "sftp.h"
#include "logger.h"
#include "socket.h"
#include "utils.h"

#define REMOTE 0
#define LOCAL 1

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
    L_PUT,
    L_MKDIR,
    L_TOGGLE,
    L_EXIT,
    INVALID
};

enum L_ERROR
{
    L_INVALID_COMMAND = -1,
    L_FAILED_TO_OPEN_FILE = -2,
    L_INVALID_PATH = -3
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
    case L_INVALID_PATH:
        LOGGER::LogError("Invalid Path");
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
        { "put", L_PUT },
        { "mkdir", L_MKDIR },
        { "toggle", L_TOGGLE },
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

void parseCd(ClientSocket& sock, std::string& remoteDir)
{
    sock.recv(in);
    if (checkStatus())
    {
        std::string newPath(in+1);
        remoteDir = newPath;
    }
    else 
    {
        // Error was already handled by checkStatus()
        return;
    }
}

void parseGrab(ClientSocket& sock, std::string outputDir)
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
    outfile.open(outputDir + "/" + path, std::ios::binary);

    if (!outfile.is_open())
    {
        error(L_FAILED_TO_OPEN_FILE);
        return;
    } 

    LOGGER::Log("[", LOGGER::COLOR::WHITE, false);

    do
    {
        // Get secondary information
        sock.recv(in);

        if (checkStatus()) // Handles errors
        {
            // Append output to output string
            uint16_t length;
            memcpy(&length, in+1, 2);

            LOGGER::Log("#", LOGGER::COLOR::WHITE, false);
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

    LOGGER::Log("]");

    outfile.close();
}

void sendFile(ClientSocket& sock)
{
    
}

void localPwd(const std::string& localDir)
{
    LOGGER::Log(localDir);
}

void localLs(const std::string& localDir)
{
    std::string ret;
    ret = exec("ls -la " + localDir);

    LOGGER::Log(ret);
}

void changeLocalDirectory(std::string& localDir, const std::string& path)
{
    std::string newPath = generateNewPath(localDir, path);
    
    // Check if path is valid
    struct stat buffer;
    if (stat(newPath.c_str(), &buffer) != 0)
    {
        error(L_INVALID_PATH);
        return;
    }

    localDir = newPath;
}

std::string getExecPath()
{
  char result[ 1024 ];
  const char* path;

  ssize_t count = readlink( "/proc/self/exe", result, 1024 );
  if (count != -1)
      path = dirname(result);

  return path;
}

int main(int argc, char** argv)
{
    std::string username;
    std::string remoteDir;
    std::string localDir = getExecPath();

    int mode = REMOTE;
    // TODO update all commands so that they work with their appropriate mode 

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

        if (mode == LOCAL)
            LOGGER::Log("(LOCAL) " + localDir + "$ ", LOGGER::COLOR::GREEN, false);
        else
            LOGGER::Log("(REMOTE) " + remoteDir + "$ ", LOGGER::COLOR::RED, false);

        getline(std::cin, input);

        std::vector<std::string> cmd = split(input);

        switch (resolveCommand(cmd[0]))
        {
        case L_PWD:
            if (mode == REMOTE)
            {
                sock.send(SFTP::ccPwd(out), out);
                parsePwd(sock);
            }
            else
                localPwd(localDir);

            break;
        case L_LS:
            if (mode == REMOTE)
            {
                sock.send(SFTP::ccLs(out), out);
                parseLs(sock);
            }
            else
                localLs(localDir);

            break;
        case L_CDIR:
            if (cmd.size() != 2)
            {
                LOGGER::Log("Usage: cd <path>");                
                break;
            }
            if (mode == REMOTE)
            {
                sock.send(SFTP::ccCd(out, cmd[1]), out);
                parseCd(sock, remoteDir);
            }
            else
                changeLocalDirectory(localDir, cmd[1]);

            break;
        case L_GRAB:
            if (mode != REMOTE)
            {
                LOGGER::LogError("the grab command is only valid in REMOTE mode");
                break;
            }
            if (cmd.size() != 2)
            {
                LOGGER::Log("Usage: grab <filename>");
                break;
            }
            sock.send(SFTP::ccGrab(out, cmd[1]), out);
            parseGrab(sock, localDir);
            break;
        case L_PUT:
            if (mode != LOCAL)
            {
                LOGGER::LogError("the put command is only valid in LOCAL mode");
                break;
            }
            sendFile(sock);
            break;
        case L_MKDIR:
            if (mode == REMOTE)
            {

            }
            else
            {

            }
            break;
        case L_TOGGLE:
            mode = (mode == LOCAL) ? REMOTE : LOCAL;
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

