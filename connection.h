#ifndef SFTP_CONNECTION_H
#define SFTP_CONNECTION_H

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>

#include <cmath>

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

    void start();
    void close();

    bool isActive() const { return running; };
    std::string Name() const { return sock->Name(); }

private:
    void listen();
    void setActive(bool active) { running = active; };

    bool isLoggedIn() const { return user.username.size() > 0; };

    void checkPassword();
    void printWorkingDirectory();
    void listDirectory();
    void changeUserDirectory();
    void createDirectory();
    void grabFile();
    void receiveFile();
    void handleCommand();

private:
    User user;
    Socket* sock;

    // Thread stuff
    std::unique_ptr<std::thread> t;
    std::atomic<bool> running;

    // Buffer
    char* in;
    char* out;

    // Mutex
    std::mutex mtx;
};

#endif
