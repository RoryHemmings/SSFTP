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
        { 0x05, KILL },
        { 0x06, NAME },
        { 0x07, DONE },
        { 0x08, RETR },
        { 0x09, STOR },
    };

    uint8_t val = (uint8_t)cmd;

    auto it = commands.find(val);
    if (it != commands.end())
        return it->second;

    // TODO figure out how to return safely
    LOGGER::LogError("Invalid Protocol Command");
    exit(1);
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

size_t SFTP::crPwd(char* out, const string& currentDir)
{
    clearBuffer(BUFLEN, out);
    size_t len = 0;

    out[0] = SUCCESS;
    len += 1;

    strncpy(out+len, currentDir.c_str(), BUFLEN);
    len += currentDir.size();

    /* Make sure that null termination is added
       in case that ls content len goes over BUFLEN */
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

    strncpy(out+len, data.c_str(), BUFLEN);  
    len += std::min(data.size(), (size_t)(BUFLEN - 2)); // Max buffer length minus header and termination

    out[len] = '\0';
    len += 1;

    return len;
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


