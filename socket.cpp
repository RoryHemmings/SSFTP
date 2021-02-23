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

void ClientSocket::recv(char* buffer, size_t len) const
{
    // Recieves info
    read(sock, buffer, len);
}

string ClientSocket::recvLine(size_t len) const
{
    char buffer[len];
    read(sock, buffer, len);

    return string(buffer);
}

ServerSocket::ServerSocket(const string& adress, int port, void (*onConnect)(Socket*), void (*onDisconnect)(Socket*))
    : Socket(), onConnect(onConnect), onDisconnect(onDisconnect)
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

    LOGGER::DebugLog("Server is running: ");
    cout << "Server is running: " << inet_ntoa(servAddr.sin_addr) << endl;
}

Socket* ServerSocket::recv(char* buffer, size_t len)
{
    int fd, maxFd, activity, valread;

    // File descriptor set data structure
    fd_set readfds;

    // Holds the adress and other useful information about the connection (port,
    // ...)
    struct sockaddr_in adress;
    size_t addrlen = sizeof(adress);

    // Wait for activity until non-join data is read
    while (true)
    {
        // Clear fd set
        FD_ZERO(&readfds);

        // Add server fd to fd set
        FD_SET(sock, &readfds);
        maxFd = sock;

        for (vector<Socket*>::const_iterator iter = clients.begin();
                iter != clients.end(); ++iter)
        {
            // Invokes implicit conversion from Socket to int
            fd = *(*iter);

            // Checks whether or not the file descriptor is valid
            if (fd > 0)
            {
                FD_SET(fd, &readfds);
            }

            if (fd > maxFd)
            {
                maxFd = fd;
            }
        }

        // Waits indefinitely until activity happens on one of the sockets in the
        // fd set
        activity = select(maxFd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            LOGGER::LogError("select error");
        }

        // Server got connection request
        if (FD_ISSET(sock, &readfds))
        {
            fd = accept(sock, (struct sockaddr*)&adress, (socklen_t*)&addrlen);

            if (fd > 0)
            {
                // Allocate new memory to this new client
                Socket* client = new Socket(fd);
                clients.push_back(client);

                (*onConnect)(client);
            }
            else
            {
                // Error accepting client
                LOGGER::LogError("Accept failure");
            }
        }

        vector<Socket*>::iterator iter = clients.begin();
        while (iter != clients.end())
        {
            // Implicit conversion from Socket to int
            fd = *(*iter);

            // If socket has activity
            if (FD_ISSET(fd, &readfds))
            {
                // Read from the socket
                valread = read(fd, buffer, len);

                if (valread == 0)
                {
                    (*onDisconnect)(*iter);
                    LOGGER::Log("Client disconnected: " + (*iter)->Name(), LOGGER::COLOR::YELLOW);
                    // Closes fd, deallocates the Socket*, and removes Socket* from clients
                    ::close(fd);
                    delete *iter;
                    clients.erase(iter);
                    // continues without incrementing iter
                    continue;
                }
                else
                {
                    /*
                     *  This function only reads data from one client socket, however I don't
                     *  think that this matters beacause the client sockets that are not
                     *  read are left in states that indicate that they should be read.
                     *  Therefore, when the algorithm is run again, it will eventually
                     *  get the the socket that hasn't been read and return it.
                     */
                    return *iter;
                }
            }

            ++iter;
        }
    }
}

Socket* ServerSocket::recvLine(string& in, size_t len)
{
    Socket* client;
    char buffer[len];

    // Forward call to char* version of recv
    client = recv(buffer, len);

    // Sets in to everything up until the null byte
    in = string(buffer);
    return client;
}

void Socket::close()
{
    // Since names of functions are the same, I have to clarify the scope of
    // close()
    ::close(sock);
}

void ServerSocket::close()
{
    int fd;

    for (vector<Socket*>::iterator iter = clients.begin();
            iter != clients.end(); iter++)
    {
        fd = *(*iter);

        // Closes and deallocates each Socket* in clients vector
        ::close(fd);
        delete *iter;
    }

    Socket::close();
}

