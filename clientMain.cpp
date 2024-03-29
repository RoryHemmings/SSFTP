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
    L_RM,
    L_TOGGLE,
    L_CLEAR,
    L_HELP,
    L_EXIT,
    INVALID
};

enum L_ERROR
{
    L_INVALID_COMMAND = -1,
    L_FAILED_TO_OPEN_FILE = -2,
    L_INVALID_PATH = -3,
    L_FILE_SEND_FAILED = -4,
    L_FILE_EXISTS = -5
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
    case SFTP::ACCESS_DENIED:
        LOGGER::LogError("Access Denied");
        break;
    case SFTP::SERVER_FULL:
        LOGGER::LogError("The server is currently full");
        exit(code);
    case L_INVALID_COMMAND:
        LOGGER::LogError("Invalid Command");
        break;
    case L_FAILED_TO_OPEN_FILE:
        LOGGER::LogError("Failed to open file locally (check the filename and path)");
        break;
    case L_INVALID_PATH:
        LOGGER::LogError("Invalid Path");
        break;
    case L_FILE_SEND_FAILED:
        LOGGER::LogError("Failed to send file");
        break;
    case L_FILE_EXISTS:
        LOGGER::LogError("File Already Exists");
        break;
    default:
        LOGGER::LogError("Unkown Error");
        LOGGER::LogError("Code: " + std::to_string(code));
    }

    std::string details(in+2);
    if (details.size() > 0)
        LOGGER::Log(details);
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
        { "rm", L_RM },
        { "toggle", L_TOGGLE },
        { "clear", L_CLEAR },
        { "help", L_HELP },
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

std::string authenticateConnection(ClientSocket& sock, std::string& remoteHomeDir)
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
    // getline(std::cin, password);
    password = getpass("", true);

    // hexDump("UserBuffer", out, 100);
    sock.send(SFTP::ccUser(out, username, password), out);
    sock.recv(in);

    if (!checkStatus())
    {
        username = authenticateConnection(sock, remoteHomeDir); // Run recursively until success
    }

    remoteHomeDir = std::string(in+1);
    return username;
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
    if (!checkStatus()) // Handles errors
        return;
    
    // Get the uint32_t value: end
    memcpy(&end, in+1, 4);
    clearBuffers(BUFLEN, in, out);

    do
    {
        // Give go-ahead to continue receiving
        sock.send(SFTP::createSuccessResponse(out), out);

        // Get secondary information
        sock.recv(in);
        if (!checkStatus())
            return;

        // Append output to output string
        output += std::string(in+1);
        clearBuffers(BUFLEN, in, out);

        ++index;
    }
    while (index < end);

    LOGGER::Log(output, LOGGER::WHITE, false);
}

void parseCd(ClientSocket& sock, std::string& remoteDir)
{
    sock.recv(in);
    if (!checkStatus()) // Handles errors
        return;

    std::string newPath(in+1);
    remoteDir = newPath;
    
}

void parseGrab(ClientSocket& sock, const std::string& outputDir, const std::string& fn="")
{
    uint32_t index = 0, end;
    std::string filename = fn;
    
    // Get primary information
    sock.recv(in);

    if (!checkStatus()) // Handles errors
        return;

    // Get the uint32_t value: end
    memcpy(&end, in+1, 4);

    if (filename.size() == 0)
        filename = std::string(in+5);

    std::string outPath = generateNewPath(outputDir, filename);

    if (fileExists(outPath))
    {
        error(L_FILE_EXISTS);
        return;
    }
    
    LOGGER::Log("Grabbing into: " + outPath);

    std::ofstream outfile;
    outfile.open(outPath, std::ios::binary);

    if (!outfile.is_open())
    {
        error(L_FAILED_TO_OPEN_FILE);

        clearBuffer(BUFLEN, out);
        sock.send(SFTP::createFailureResponse(out, SFTP::FAILED_TO_OPEN_FILE), out);
        return;
    } 

    LOGGER::Log("[", LOGGER::COLOR::WHITE, false);
    do
    {
        clearBuffers(BUFLEN, in, out);

        // Give go-ahead to continue conversation
        sock.send(SFTP::createSuccessResponse(out), out);

        // Get secondary information
        sock.recv(in);

        if (!checkStatus()) // Handles errors
            return;
        
        // Append output to output string
        uint16_t length;
        memcpy(&length, in+1, 2);

        LOGGER::Log("#", LOGGER::COLOR::WHITE, false);
        outfile.write(in+3, length);

        ++index;
    }
    while (index < end);

    LOGGER::Log("]");
    outfile.close();
}

void parseMkDir(ClientSocket& sock)
{
    sock.recv(in); 
    if (!checkStatus())
        return;

    LOGGER::Log(std::string(in+1), LOGGER::COLOR::WHITE, false);
}

void parseRm(ClientSocket& sock)
{

}

void sendFile(ClientSocket& sock, const std::string& localDir, const std::string& filePath, const std::string& outputPath)
{
    std::string path;
    path = generateNewPath(localDir, filePath);

    std::ifstream file;
    file.open(path, std::ios::binary);
    if (!file.is_open())
    {
        error(L_FAILED_TO_OPEN_FILE);
        return;
    }

    // Get file size
    file.seekg(0, file.end);
    int fileSize = file.tellg();
    file.seekg(0, file.beg);

    uint32_t totalPackets = floor(fileSize / (BUFLEN - 4)) + 1;

    sock.send(SFTP::ccPutPrimary(out, totalPackets, outputPath), out);  
     
    while (!file.eof())
    {
        clearBuffers(BUFLEN, in, out);

        sock.recv(in);
        if (!checkStatus())
            return;       

        size_t i = 3; // Starts at 3 to leave space for status byte and length
        while (i < BUFLEN - 1)
        {
            char c = (char) file.get();
            if (file.eof())
                break;

            out[i] = c;
            ++i;
        }

        uint16_t length = i - 3;
        sock.send(SFTP::ccPut(out, length), out); // Modifies buffer as opposed to copying it
    }
    LOGGER::Log("File " + path + " sent to " + outputPath);
}

void localPwd(const std::string& localDir)
{
    LOGGER::Log(localDir);
}

void localLs(const std::string& localDir, const std::string& path)
{
    std::string finalPath = generateNewPath(localDir, path);

    LOGGER::Log(exec("ls -la " + finalPath), LOGGER::COLOR::WHITE, false);
}

void localMkDir(const std::string& localDir, const std::string& path)
{
    std::string finalPath = generateNewPath(localDir, path);

    LOGGER::Log(exec("mkdir " + finalPath), LOGGER::COLOR::WHITE, false);
}

void localRm(const std::string& path)
{

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

void helpMenu()
{
    LOGGER::Log("toggle                 switches between local and remote mode");
    LOGGER::Log("pwd                    prints working directory of current mode");
    LOGGER::Log("ls                     shows files in directory of current mode");
    LOGGER::Log("cd <path>              changes working directory of current mode");
    LOGGER::Log("mkdir <name>           creates new directory with name 'name' in current mode");
    LOGGER::Log("");
    LOGGER::Log("grab <path> <out:opt>  downloads file from path on remote machine to current directory on local machine");
    LOGGER::Log("                       only available in remote mode");
    LOGGER::Log("put <path> <out>       uploads file from path on local machine to current directory on remote machine");
    LOGGER::Log("                       only available in local mode");
    LOGGER::Log("");
    LOGGER::Log("clear                  clears the console");
    LOGGER::Log("help                   displays this message");
    LOGGER::Log("exit                   close connection and exit program");
}

int main(int argc, char** argv)
{
    std::string username;
    std::string remoteDir;
    std::string localDir = getExecPath();

    std::string addr = "127.0.0.1";

    int mode = REMOTE;

    ClientSocket sock(addr, PORT);

    sock.recv(in);
    if (in[0] == SFTP::SUCCESS)
        LOGGER::DebugLog("Connection Established");
    else
        error(in[1]);  // Server sent invalid response

    username = authenticateConnection(sock, remoteDir);

    std::string input;
    size_t len = 0;
    
    do
    {
        clearBuffers(BUFLEN, in, out);

        if (mode == LOCAL)
            LOGGER::Log("[" + addr + "] (LOCAL) " + localDir + "$ ", LOGGER::COLOR::GREEN, false);
        else
            LOGGER::Log("[" + addr + "] (REMOTE) " + remoteDir + "$ ", LOGGER::COLOR::RED, false);

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
            if (cmd.size() > 2)
            {
                LOGGER::Log("Usage: ls <path>");
                break;
            }
            if (mode == REMOTE)
            {
                if (cmd.size() == 2)
                    sock.send(SFTP::ccLs(out, cmd[1]), out);
                else
                    sock.send(SFTP::ccLs(out, ""), out);

                parseLs(sock);
            }
            else
            {
                if (cmd.size() == 2)
                    localLs(localDir, cmd[1]);
                else
                    localLs(localDir, "");
            }

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
            if (cmd.size() != 2 && cmd.size() != 3)
            {
                LOGGER::Log("Usage: grab <filename> <outputFilename:optional>");
                break;
            }

            sock.send(SFTP::ccGrab(out, cmd[1]), out);
            if (cmd.size() == 3)
                parseGrab(sock, localDir, cmd[2]);
            else
                parseGrab(sock, localDir);

            break;
        case L_PUT:
            if (mode != LOCAL)
            {
                LOGGER::LogError("the put command is only valid in LOCAL mode");
                break;
            }
            if (cmd.size() != 3)
            {
                LOGGER::Log("Usage: put <filename> <output path>");
                break;
            }

            sendFile(sock, localDir, cmd[1], cmd[2]);
            break;
        case L_MKDIR:
            if (cmd.size() != 2)
            {
                LOGGER::Log("Usage: mkdir <path>");
                break;
            }
            if (mode == REMOTE)
            {
                sock.send(SFTP::ccMkDir(out, cmd[1]), out);
                parseMkDir(sock);
            }
            else
            {
                localMkDir(localDir, cmd[1]);
            }
            break;
        case L_RM:
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
        case L_CLEAR:
            system("clear");
            break;
        case L_HELP:
            helpMenu();
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

