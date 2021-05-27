#include <sys/time.h>

#include "socket.h"
#include "logger.h"

using namespace std;

Socket::Socket()
    : sock(0), name("<null>")
{
    // Creates Socket with ipv4, socket communication, and version 0
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        LOGGER::LogError("Error Creating Socket");
        exit(1);
    }
}

// Create socket given a file descriptor
Socket::Socket(int fd)
    : sock(fd)
{
    struct sockaddr_in adress;
    size_t addrlen = sizeof(adress);

    // Get adress of fd
    getpeername(fd, (struct sockaddr*)&adress, (socklen_t*)&addrlen);
    name = string("<" + to_string(fd)) + ":" + string(inet_ntoa(adress.sin_addr)) + ">";
}

Socket::operator int() const
{
    // Converts Socket to internal socket_fd
    return sock;
}

Socket::~Socket()
{
}

void Socket::send(size_t len, const char* data) const
{
    ::send(sock, data, len, 0);
}

void Socket::sendLine(const string& line) const
{
    // Length is 1 longer because c_str() adds null termination
    ::send(sock, line.c_str(), line.size() + 1, 0);
}

size_t Socket::recv(char* buffer, size_t len) const
{
    return read(sock, buffer, len);
}

string Socket::Name() const
{
    return name;
}

ClientSocket::ClientSocket(const string& address, int port)
    : Socket()
{
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);

    // Set adress of servAddr object
    if (inet_pton(AF_INET, address.c_str(), &servAddr.sin_addr) <= 0)
    {
        LOGGER::LogError("Invalid adress/ Adress not supported");
        exit(1);
    }

    // Connect
    if (connect(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
    {
        LOGGER::LogError("Connection Failed (connect())");
        exit(1);
    }
}

string Socket::recvLine(size_t len) const
{
    char buffer[len];
    read(sock, buffer, len);

    return string(buffer);
}

ServerSocket::ServerSocket(const string& adress, int port)
    : Socket()
{
    struct sockaddr_in servAddr;
    int opt = 1;
    size_t addrlen = sizeof(servAddr);

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
    {
        LOGGER::LogError("setsockopt() failed");
        exit(1);
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    // servAddr.sin_addr.s_addr = INADDR_ANY;

    if (inet_pton(AF_INET, adress.c_str(), &servAddr.sin_addr) <= 0)
    {
        LOGGER::LogError("Invalid adress/ Adress not supported");
        exit(1);
    }

    if (bind(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
    {
        LOGGER::LogError("Bind Failed (something else might be running on currently assigned port, or access may be denied.\nPort: " + port);
        exit(1);
    }

    if (listen(sock, 3) < 0)
    {
        LOGGER::LogError("Listen Failed");
        exit(1);
    }

    cout << "Server is running: " << inet_ntoa(servAddr.sin_addr) << endl;
}

/* Blocks until client joins
 */
Socket* ServerSocket::accept()
{
    static struct sockaddr_in adress;
    static int addrlen = sizeof(adress);

    int fd = ::accept(sock, (struct sockaddr*)&adress, (socklen_t*)&addrlen);
    Socket* ret = new Socket(fd);

    return ret;
}

void Socket::close()
{
    // Since names of functions are the same, I have to clarify the scope of close()
    ::close(sock);
}

