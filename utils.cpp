#include "utils.h"

using namespace std;

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

bool isspace(char c)
{
    return c == ' ';
}

vector<string> split(const string& s)
{
    vector<string> ret;
    typedef string::size_type string_size;
    string_size i = 0;

    while (i != s.size())
    {
        while (i != s.size() && isspace(s[i]))
            ++i;

        string_size j = i;
        while (j != s.size() && !isspace(s[j]))
            ++j;

        if (i != j)
        {
            ret.push_back(s.substr(i, j - i));
            i = j;
        }
    }

    return ret;
}

