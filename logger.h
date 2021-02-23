#ifndef DSFTP_LOGGER_H
#define DSFTP_LOGGER_H

#include <iostream>
#include <string>

namespace LOGGER
{

enum COLOR
{
    BLACK = 30,
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36,
    WHITE = 37
};

void Log(const std::string&);
void Log(const std::string&, COLOR, bool newLine=true);
void DebugLog(const std::string&);
void LogError(const std::string&);
void HexDump(const char*, const void*, const int);

}

#endif
