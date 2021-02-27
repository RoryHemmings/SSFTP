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
   * the buffer was initially cleared
   * so null termination for full 
   * command is already taken care of
   */
  len += 1; // Length still has to be increased to include the termination

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

  return len;
}

size_t SFTP::crPwd(char* out, const string& path)
{
  clearBuffer(BUFLEN, out);  
  size_t len = 0;

  out[0] = SUCCESS;
  len += 1;

  strcpy(out+len, path.c_str());
  len += path.size();

  /* strcpy automatically adds null termination
     however str::size() doesn't actually include it */
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
  out[1] = '\0';
  len += 1;

  return len;
}

size_t SFTP::createFailureResponse(char* out, uint8_t code)
{
  clearBuffer(BUFLEN, out);

  size_t len = 0;

  out[0] = FAILURE;
  len += 1;

  out[1] = code;
  len += 1;

  out[2] = '\0';
  len += 1;

  return len;
}


