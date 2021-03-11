#ifndef SFTP_H
#define SFTP_H

#define PORT 115
#define BUFLEN 1024

#include <cstdarg>
#include <cstdint>
#include <string>
#include <map>

/*struct Cmd
{
  const char* command;
  int mode;
}*/

namespace SFTP
{

// Commands along with their binary represenations (in hex form)
enum COMMAND
{
    USER = 0x01, // 0000 0001
    PRWD = 0x02, // 0000 0010
    LIST = 0x03, // 0000 0011
    CDIR = 0x04, // 0000 0100
    KILL = 0x05, // 0000 0101
    NAME = 0x06, // 0000 0110
    DONE = 0x07, // 0000 0111
    RETR = 0x08, // 0000 1000
    STOR = 0x09, // 0000 1001
};

enum RESPONSE
{
    SUCCESS = 0x01,  // 0000 0001
    FAILURE = 0x02,    // 0000 0010
};

enum SFTP_ERROR
{
    MISC_ERROR = 1,
    INVALID_COMMAND = 2,
    INVALID_USER = 3,
    INVALID_PASSWORD = 4,
    NOT_LOGGED_IN = 5,
    INVALID_RESPONSE = 6,
    INVALID_CLIENT_RESPONSE = 7,
    COMMAND_EXECUTION_FAILED = 8
};

COMMAND resolveCommand(const char cmd);

/*
 * Command Factories (used by client)
 */
size_t ccUser(char* out, const std::string& username, const std::string& password);
size_t ccPwd(char* out);
size_t ccLs(char* out);
size_t ccCd(char* out, const std::string& path);

/*
 * Response Factories (used by server)
 */

size_t crPwd(char* out, const std::string& workingDir);
size_t crLs(char* out, const std::string& data, uint32_t index, uint32_t end);
size_t crCd(char* out, const std::string& finalPath);

/*
 * Stock Response Factories (used by server)
 */
size_t createSuccessResponse(char* out);
size_t createFailureResponse(char* out, uint8_t code);

}

#endif
