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

#endif

