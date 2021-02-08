#include <cstring>

#include "sftp.h"

size_t SFTP::createMessage(char* out, COMMAND sftp_cmd, int argc, ...)
{
  const char* cmd;

  switch (sftp_cmd)
  {
    case USER:
      cmd = "USER";
      break;
    case ACCT:
      cmd = "ACCT";
      break;
    case PASS:
      cmd = "PASS";
      break;
    case TYPE:
      cmd = "TYPE";
      break;
    case LIST:
      cmd = "LIST";
      break;
    case CDIR:
      cmd = "CDIR";
      break;
    case KILL:
      cmd = "KILL";
      break;
    case NAME:
      cmd = "NAME";
      break;
    case DONE:
      cmd = "DONE";
      break;
    case RETR:
      cmd = "RETR";
      break;
    case STOR:
      cmd = "STOR";
      break;
  }

  strncpy(out, cmd, BUFLEN);
  strncat(out, " ", BUFLEN);

  va_list argv;
  va_start(argv, argc);

  for (int i = 0; i < argc; i++)
  {
    strncat(out, va_arg(argv, const char*), BUFLEN);

    // NULL byte is always added on the end
    strncat(out, " ", BUFLEN);
  }

  va_end(argv);

  return strlen(out) + 1;
}

size_t SFTP::createResponse(char* out, RESPONSE code, const char* message)
{
  const char* response;
  switch (code)
  {
    case SUCCESS:
      response = "+";
      break;
    case ERROR:
      response = "-";
      break;
    case LOGGED_IN:
      response = "!";
      break;
  }

  strncpy(out, response, BUFLEN);
  strncat(out, message, BUFLEN);

  return strlen(out) + 1;
}
