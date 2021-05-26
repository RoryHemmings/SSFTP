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
    REMV = 0x05, // 0000 0101
    MOVE = 0x06, // 0000 0110
    GRAB = 0x07, // 0000 0111
    PUTF = 0x08, // 0000 1000
    COPY = 0x09, // 0000 1001
    MDIR = 0x10, // 0000 1010
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
    COMMAND_EXECUTION_FAILED = 8,
    INVALID_PATH = 9,
    FAILED_TO_OPEN_FILE = 10,
    ACCESS_DENIED = 11,
    SERVER_FULL = 12
};

COMMAND resolveCommand(const char cmd);

/*
 * Command Factories (used by client)
 */
size_t ccUser(char* out, const std::string& username, const std::string& password);
size_t ccPwd(char* out);
size_t ccLs(char* out, const std::string& path);
size_t ccCd(char* out, const std::string& path);
size_t ccGrab(char* out, const std::string& path);
size_t ccPutPrimary(char* out, uint32_t totalPackets, const std::string& path);
size_t ccPut(char* out, uint16_t dataLength);
size_t ccMkDir(char* out, const std::string& name);
size_t ccRm(char* out, const std::string& path);

/*
 * Response Factories (used by server)
 */

size_t crPwd(char* out, const std::string& workingDir);
size_t crLsPrimary(char* out, uint32_t totalPackets);
size_t crLs(char* out, const std::string& data);
size_t crCd(char* out, const std::string& finalPath);
size_t crGrabPrimary(char* out, uint32_t totalPackets, const std::string& path);
size_t crGrab(char* out, uint16_t dataLength);
size_t crMkDir(char* out, const std::string& output);

/*
 * Stock Response Factories (used by server)
 */
size_t createSuccessResponse(char* out);
size_t createFailureResponse(char* out, uint8_t code);
size_t createFailureResponse(char* out, uint8_t code, const std::string& details);

}

#endif
