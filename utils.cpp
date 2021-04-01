#include "utils.h"
#include <algorithm>

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

void hexDump (const char* desc, const void* addr, const int len) {
    int i;
    unsigned char buff[17];
    const unsigned char * pc = (const unsigned char *)addr;

    // Output description if given.

    if (desc != NULL)
        printf ("%s:\n", desc);

    // Length checks.

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    else if (len < 0) {
        printf("  NEGATIVE LENGTH: %d\n", len);
        return;
    }

    // Process every byte in the data.

    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Don't print ASCII buffer for the "zeroth" line.

            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.

            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And buffer a printable ASCII character for later.

        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) // isprint() may be better.
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.

    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII buffer.

    printf ("  %s\n", buff);
}

std::string exec(const std::string& command)
{
    char buffer[128];
    std::string ret = "";
    FILE* pipe;

    pipe = popen(command.c_str(), "r");
    if (!pipe)
        return "";
    try
    {
        while (fgets(buffer, sizeof buffer, pipe) != NULL)
        {
            ret += buffer;
        }
    }
    catch (...)
    {
        pclose(pipe);
        return "";
    }

    pclose(pipe);
    return ret;
}

bool isSlash(char c)
{
    return c == '/';
}

/*
 *  Recursively removes dot pairs
 */
void removeDotPairs(std::string& s)
{
    std::vector<int> slashIndicies;
    for (std::string::size_type i = 0; i < s.size(); ++i)
    {
        if (s[i] == '/')
            slashIndicies.push_back(i);
    }
    
    for (auto iter = slashIndicies.begin(); iter != slashIndicies.end(); ++iter)
    {
        if (s[*(iter)+1] == '.') 
        {
            if (s[*(iter)+2] == '.') 
            {
                if (s[*(iter)+3] == '/') // whole path is wrapped in /, so this shouldn't violate bounds
                {
                    // succesfully found valid /..
                    // delete from last slash index to this slash index plus 3
                    
                    s.erase(*(iter - 1), ((*iter) + 3) - (*(iter - 1)));

                    /* Since slashIndicies is invalid, now I can
                     * just recursively call the function to start over
                     * and Eventually remove all pairs
                     */
                    removeDotPairs(s);
                    break;
                }
            }
        }
    }
}

std::string generateNewPath(const std::string& path, const std::string& next)
{
    std::string fullPath = "/" + path + "/" + next + "/";
    std::string ret = "";
    
    // Remove double (or more) slashes
    // this way it will work regardless 
    // of when people put slashes in their paths
    auto i = fullPath.begin();
    while (i != fullPath.end())
    {
        ret += *i;
        if ((*i) == '/')
            i = std::find_if_not(++i, fullPath.end(), isSlash); // Skip all preceding slashes until regular char is found
        else
            ++i;
    }

    // Apply instances of /..
    removeDotPairs(ret);

    // Remove ending slash
    if (ret[ret.size() - 1] == '/')
        ret.erase(ret.size() - 1);

    return ret;
}
