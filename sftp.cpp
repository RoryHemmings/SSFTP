#include <algorithm>
#include <cstring>
#include "logger.h"
#include "utils.h"
#include "sftp.h"

using namespace std;
using namespace SFTP;

COMMAND SFTP::resolveCommand(const char cmd)
{
    static const map<uint8_t, COMMAND> commands
    {
        { 0x01, USER },
        { 0x02, PRWD },
        { 0x03, LIST },
        { 0x04, CDIR },
        { 0x05, REMV },
        { 0x06, MOVE },
        { 0x07, GRAB },
        { 0x08, PUTF },
        { 0x09, COPY },
        { 0x10, MDIR },
    };

    uint8_t val = (uint8_t)cmd;

    auto it = commands.find(val);
    if (it != commands.end())
        return it->second;

    // TODO figure out how to return safely
    LOGGER::LogError("Invalid Protocol Command");
    exit(1);
}

size_t createStringSandwich(char* out, uint8_t cmd, const std::string& arg)
{
    clearBuffer(BUFLEN, out);

    size_t len = 0;
    size_t numReservedBytes = 2; // Status byte and termination

    out[0] = cmd;
    len += 1;

    strncpy(out+len, arg.c_str(), BUFLEN - numReservedBytes);
    len += min(arg.size(), (size_t) BUFLEN - numReservedBytes);

    out[len] = '\0';
    len += 1;

    return len;
}

size_t SFTP::ccUser(char* out, const string& username, const string& password)
{
    size_t len = 0;
    uint16_t usernameLength, passwordLength;

    clearBuffer(BUFLEN, out);

    // Set command byte
    out[len] = SFTP::USER;
    len += 1;

    /*
     * Header
     */
    usernameLength = username.size();
    if (usernameLength > 512) // Username exceeds max length
        return -1;

    // Set 2 username length bytes
    memcpy(out+len, &usernameLength, 2);
    len += 2;

    passwordLength = password.size();
    if (passwordLength > 512) // Password exceeds max length
        return -2;

    // Set 2 password length bytes
    memcpy(out+len, &passwordLength, 2);
    len += 2;

    /*
     * Data
     */

    // Set username bytes
    memcpy(out+len, username.c_str(), usernameLength); // Intentionally leaves off null termination
    len += usernameLength;

    // Set password bytes
    memcpy(out+len, password.c_str(), passwordLength); // Intentionally leaves off null termination
    len += passwordLength;

    /*
     * Although this byte is already set
     * it is more readable if i set it here
     * just so that there isn't a random len
     * increase statement
     */
    out[len] = '\0';
    len += 1;

    if (len > BUFLEN) // Buffer overflow
        return -3;

    return len;
}

size_t SFTP::ccPwd(char* out)
{
    clearBuffer(BUFLEN, out);
    size_t len = 0;

    out[0] = PRWD;
    len += 1;

    out[len] = '\0';
    len += 1;

    return len;
}

size_t SFTP::ccLs(char* out)
{
    clearBuffer(BUFLEN, out);
    size_t len = 0;

    out[0] = LIST;
    len += 1;

    out[len] = '\0';
    len += 1;

    return len;
}

size_t SFTP::ccCd(char* out, const string& path)
{
    clearBuffer(BUFLEN, out);
    size_t len = 0;

    out[0] = CDIR;
    len += 1;

    strncpy(out+len, path.c_str(), BUFLEN);
    len += min(path.size(), (size_t) BUFLEN);

    /* ensure that null termination is added to
       prevent buffer overflows */
    out[len] = '\0';
    len += 1;
    
    return len;
}

size_t SFTP::ccGrab(char* out, const std::string& path)
{
    clearBuffer(BUFLEN, out);
    size_t len = 0;

    out[0] = GRAB;
    len += 1;

    size_t maxStrLen = BUFLEN - 2; // buffer length - status and termination
    strncpy(out+len, path.c_str(), maxStrLen);
    len += min(path.size(), maxStrLen);

    out[len] = '\0';
    len += 1;

    return len;
}

size_t SFTP::ccPutPrimary(char* out, uint32_t totalPackets, const std::string& path)
{
    clearBuffer(BUFLEN, out);

    size_t len = 0;
    size_t numReservedBytes = 2 + sizeof(totalPackets);

    out[0] = PUTF;
    len += 1;

    memcpy(out+len, &totalPackets, sizeof(totalPackets));
    len += sizeof(totalPackets);

    strncpy(out+len, path.c_str(), BUFLEN - numReservedBytes);
    len += min(path.size(), (size_t) BUFLEN - numReservedBytes);

    out[len] = '\0';
    len += 1;

    return len;
}

size_t SFTP::ccPut(char* out, uint16_t dataLength)
{
    
}

size_t SFTP::ccMkDir(char* out, const std::string& name)
{
    return createStringSandwich(out, MDIR, name);
}

size_t SFTP::ccRm(char* out, const std::string& path)
{
    // TODO Fix all of the previous request function inconsistencies
    // TODO make all of the commands that execute shell commands return the result of the shell command
}

size_t SFTP::crPwd(char* out, const string& currentDir)
{
    clearBuffer(BUFLEN, out);
    size_t len = 0;

    out[0] = SUCCESS;
    len += 1;

    strncpy(out+len, currentDir.c_str(), BUFLEN);
    len += min(currentDir.size(), (size_t) BUFLEN);

    /* Make sure that null termination is added
       in case that ls content len goes over BUFLEN */
    out[len] = '\0';
    len += 1;

    return len;
}

size_t SFTP::crLsPrimary(char* out, uint32_t totalPackets)
{
    clearBuffer(BUFLEN, out);
    size_t len = 0;

    out[0] = SUCCESS;
    len += 1;

    memcpy(out+len, &totalPackets, sizeof(totalPackets));
    len += sizeof(totalPackets);

    out[len] = '\0';
    len += 1;

    return len;
}

size_t SFTP::crLs(char* out, const string& data)
{
    clearBuffer(BUFLEN, out);
    size_t len = 0;

    out[0] = SUCCESS;
    len += 1;

    strncpy(out+len, data.c_str(), BUFLEN-2);  
    len += std::min(data.size(), (size_t)(BUFLEN-2)); // Max buffer length minus header and termination

    out[len] = '\0';
    len += 1;

    return len;
}

size_t SFTP::crCd(char* out, const string& finalPath)
{
    clearBuffer(BUFLEN, out);
    size_t numReservedBytes = 2;
    size_t len = 0;

    out[0] = SUCCESS;
    len += 1;

    strncpy(out+len, finalPath.c_str(), BUFLEN - numReservedBytes);
    len += min(finalPath.size(), (size_t) BUFLEN - numReservedBytes);

    out[len] = '\0';
    len += 1;

    return len;
}

size_t SFTP::crGrabPrimary(char* out, uint32_t totalPackets, const std::string& path)
{
    clearBuffer(BUFLEN, out);

    size_t numReservedBytes = 2 + sizeof(totalPackets); // status, termination, length
    size_t len = 0; 

    out[0] = SUCCESS;
    len += 1;

    memcpy(out+len, &totalPackets, sizeof(totalPackets));
    len += sizeof(totalPackets);

    strncpy(out+len, path.c_str(), BUFLEN - numReservedBytes);
    len += min(path.size(), (size_t) BUFLEN - numReservedBytes);

    out[len] = '\0';
    len += 1;

    return len;
}

/* This function breaks convention for efficiency
 * It does not append data to a buffer, but instead 
 * takes a buffer that is already modified except for
 * the ends. This is to avoid copying large buffers 
 * multiple times */
size_t SFTP::crGrab(char* out, uint16_t dataLength)
{
    size_t len = 0;  

    out[0] = SUCCESS;
    len += 1;

    memcpy(out+len, &dataLength, sizeof(dataLength));
    len += sizeof(dataLength);

    // Skip the data already written
    len += dataLength;

    out[len] = '\0';
    len += 1;
    
    return len;
}

size_t SFTP::crPut(char* out, const std::string& path)
{
    
}

size_t SFTP::crMkDir(char* out, const std::string& output)
{
    return createStringSandwich(out, SUCCESS, output);
}

size_t SFTP::createSuccessResponse(char* out)
{
    clearBuffer(BUFLEN, out);

    size_t len = 0;

    // Response type
    out[0] = SUCCESS;
    len += 1;

    // No data in this case

    // Null termination
    out[len] = '\0';
    len += 1;

    return len;
}

size_t SFTP::createFailureResponse(char* out, uint8_t code)
{
    clearBuffer(BUFLEN, out);

    size_t len = 0;

    out[0] = FAILURE;
    len += 1;

    out[len] = code;
    len += 1;

    out[len] = '\0';
    len += 1;

    return len;
}


