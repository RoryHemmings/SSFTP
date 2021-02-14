#ifndef SFTP_H
#define SFTP_H

#define PORT 115
#define BUFLEN 1024

#include <cstdarg>
#include <string>
#include <map>

/*struct Cmd
{
  const char* command;
  int mode;
}*/

namespace SFTP
{

enum COMMAND
{
    USER,
    ACCT,
    PASS,
    TYPE,
    LIST,
    CDIR,
    KILL,
    NAME,
    DONE,
    RETR,
    STOR
};

enum RESPONSE
{
    SUCCESS,
    ERROR,
    LOGGED_IN
};

COMMAND resolveCommand(const std::string& cmd);

/*
 * Both of these functions take an output buffer
 * instead of returning a string because of issues
 * with managing memory
 *
 * Return length of message (strnlen + 1 to account for \0)
 */
std::string createCommand(COMMAND cmd, int argc, ...);
std::string createResponse(RESPONSE code, const std::string& message);

int16_t sftpLogin(const std::string& username, const std::string& password);

}

#endif
