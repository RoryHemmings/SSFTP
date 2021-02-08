#include "utils.h"

void clearBuffer(size_t len, char* buffer) 
{
    for (size_t i = 0; i < len; i++)
    {
        buffer[i] = '\0';
    }
}

void clearBuffers(size_t len, char* a, char* b)
{
    clearBuffer(len, a);
    clearBuffer(len, b);
}

