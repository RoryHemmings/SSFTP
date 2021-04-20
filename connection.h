#ifndef SFTP_CONNECTION_H
#define SFTP_CONNECTION_H

#include <thread>
#include <mutex>
#include <array>

#include "socket.h"
#include "sftp.h"
#include "utils.h"

struct User
{
    std::string username;
    std::string homeDir;
    std::string currentDir;
};

class Connection
{
    
public:
    Connection(Socket*);
    ~Connection();

    void close();

private:
    void listen();

private:
    User user;
    Socket* sock;

    std::thread t;
    bool running;

    // Buffer
    char* buf;
    size_t bufferLength;

    std::mutex mut;

};

#endif
