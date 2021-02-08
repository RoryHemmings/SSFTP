#ifndef UTILS_H
#define UTILS_H

#include <cstdio>

/* Will be used to clear input and output buffers
 * throughout communication period between client 
 * and server
 */
void clearBuffer(size_t len, char* buffer);
void clearBuffers(size_t len, char* a, char* b);

#endif

