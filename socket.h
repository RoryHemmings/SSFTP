#ifndef DSFTP_SOCKET_H
#define DSFTP_SOCKET_H

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

#include "logger.h"

class Socket
{
  
public:
  Socket();
  Socket(int fd);
  // Socket(const Socket&);
  // operator=(const Socket& rhs);

  void send(size_t len, const char* data) const;
  void sendLine(const std::string&) const;

  // Facilitates conversion from Socket to socket_fd
  operator int() const;

  virtual ~Socket();

  std::string Name() const;

  void close();

protected:
  // Socket file descriptor
  int sock;

  // Name in form of <fd:ip> that can be used for debugging purposes
  std::string name;

  // Close is defined in Socket

};


class ClientSocket : public Socket
{

public:
  explicit ClientSocket(const std::string& address, int port);

  Socket* recv(char* buffer);

};

/*  when recv is called, the server will block with select until on of the
 *  sockets catches an interaction.
 *  otherwise, it will write the data to the output buffer and then return 
 *  the Socket object associated with the data
 */
class ServerSocket : public Socket
{

public:
  explicit ServerSocket(const std::string& adress, int port);

  Socket* recv(char* buffer);

  /* Does not override Socket::close(), instead it calls it for every socket in
   * the clients vector (before deallocating it) as well as its own socket.
   */
  void close();

private:
  std::vector<Socket*> clients;

};

#endif
