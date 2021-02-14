#include <cstring>
#include "logger.h"
#include "sftp.h"

using namespace std;
using namespace SFTP;

SFTP::COMMAND SFTP::resolveCommand(const string& cmd)
{
    static const map<string, COMMAND> commands
    {
        { "USER", USER },
        { "ACCT", ACCT },
        { "PASS", PASS },
        { "TYPE", TYPE },
        { "LIST", LIST },
        { "CDIR", CDIR },
        { "KILL", KILL },
        { "NAME", NAME },
        { "DONE", DONE },
        { "RETR", RETR },
        { "STOR", STOR }
    };

    auto it = commands.find(cmd);
    if (it != commands.end())
        return it->second;
    
    // TODO figure out how to return safely
    LOGGER::LogError("Invalid Protocol Command");
    exit(1);
}

string SFTP::createCommand(COMMAND sftp_cmd, int argc, ...)
{
  string ret;

  switch (sftp_cmd)
  {
    case USER:
      ret = "USER";
      break;
    case ACCT:
      ret = "ACCT";
      break;
    case PASS:
      ret = "PASS";
      break;
    case TYPE:
      ret = "TYPE";
      break;
    case LIST:
      ret = "LIST";
      break;
    case CDIR:
      ret = "CDIR";
      break;
    case KILL:
      ret = "KILL";
      break;
    case NAME:
      ret = "NAME";
      break;
    case DONE:
      ret = "DONE";
      break;
    case RETR:
      ret = "RETR";
      break;
    case STOR:
      ret = "STOR";
      break;
  }

  va_list argv;
  va_start(argv, argc);

  for (int i = 0; i < argc; i++)
  {
    ret += " ";
    ret += va_arg(argv, string);
  }

  va_end(argv);
  return ret;
}

string SFTP::createResponse(RESPONSE code, const string& message)
{
  string ret;

  switch (code)
  {
    case SUCCESS:
      ret = "+";
      break;
    case ERROR:
      ret = "-";
      break;
    case LOGGED_IN:
      ret = "!";
      break;
  }

  ret += message;
  return ret;
}
