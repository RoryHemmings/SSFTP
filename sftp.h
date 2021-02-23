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
    ACCT = 0x02, // 0000 0010
    PASS = 0x03, // 0000 0011
    TYPE = 0x04, // 0000 0100
    LIST = 0x05, // 0000 0101
    CDIR = 0x06, // 0000 0110
    KILL = 0x07, // 0000 0111
    NAME = 0x08, // 0000 1000
    DONE = 0x09, // 0000 1001
    RETR = 0x0a, // 0000 1010
    STOR = 0x0b  // 0000 1011
};

enum RESPONSE
{
    SUCCESS = 0x01,  // 0000 0001
    FAILURE = 0x02,    // 0000 0010
};

enum ERROR
{
    MISC_ERROR = 1,
    INVALID_COMMAND = 2,
    INVALID_USER = 3,
    INVALID_PASSWORD = 4,
};

COMMAND resolveCommand(const char cmd);

/*
 * Command Factories (used by client)
 */
size_t ccUser(char* out, const std::string& username, const std::string& password);

/*
 * Response Factories (used by server)
 */

/*
 * Stock Response Factories (used by server)
 */
size_t createSuccessResponse(char* out);
size_t createFailureResponse(char* out, uint8_t code);

}

#endif
