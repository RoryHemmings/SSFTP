#ifndef SFTP_SOCKET_H
#define SFTP_SOCKET_H

#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include <sys/socket.h>

#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstdlib>
#include <cstdint>
#include <memory>

#include "sftp.h"
#include "logger.h"

class Socket
{

public:
    Socket();
    Socket(int fd);
    virtual ~Socket();

    void send(size_t len, const char* data) const;
    void sendLine(const std::string&) const;

    // Returns error code from recv
    size_t recv(char* buffer, size_t len=BUFLEN) const;
    std::string recvLine(size_t len=BUFLEN) const;

    // Facilitates conversion from Socket to socket_fd
    operator int() const;

    std::string Name() const;
    void close();

protected:
    // Socket file descriptor
    int sock;

    // Name in form of <fd:ip> that can be used for debugging purposes
    std::string name;

};


class ClientSocket : public Socket
{

public:
    explicit ClientSocket(const std::string& address, int port);

};

class ServerSocket : public Socket
{

public:
    explicit ServerSocket(const std::string& adress, int port);

    Socket* accept();

};

#endif

