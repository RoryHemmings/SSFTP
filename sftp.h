#ifndef SFTP_H
#define SFTP_H

#define PORT 115
#define BUFLEN 1024

#include <cstdio>
#include <cstdint>
#include <cstdarg>

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

/*
 * Both of these functions take an output buffer
 * instead of returning a string because of issues
 * with managing memory
 *
 * Return length of message (strnlen + 1 to account for \0)
 */
size_t createMessage(char* out, COMMAND cmd, int argc, ...);
size_t createResponse(char* out, RESPONSE code, const char* message);

int16_t sftpLogin(const char* username, const char* password);

}

#endif
