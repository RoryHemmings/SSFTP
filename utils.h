#ifndef SFTP_UTILS_H
#define SFTP_UTILS_H

#include <string>
#include <vector>
#include <cstdio>

/* Will be used to clear input and output buffers
 * throughout communication period between client 
 * and server
 */
void clearBuffer(size_t len, char* buffer);
void clearBuffers(size_t len, char* a, char* b);

std::vector<std::string> split(const std::string& s);

void hexDump(const char*, const void*, const int);

std::string exec(const std::string& command);

bool isSlash(char c);
void removeDotPairs(std::string& s);
std::string generateNewPath(const std::string& path, const std::string& next);

std::string getpass(const std::string& prompt, bool shows_asterisk=true);

#endif

