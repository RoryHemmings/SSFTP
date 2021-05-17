/*
 *
 * SSFTP Server
 * Author Rory Hemmings
 *
 */

#include <iostream>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <future>
#include <map>
#include <cmath>
#include <fstream>

#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <sys/stat.h>
#include <shadow.h>
#include <signal.h>

#include "sftp.h"
#include "logger.h"
#include "socket.h"
#include "connection.h"
#include "utils.h"

// Keeps track of the connections so they can be deleted
std::vector<Connection*> connections;
const unsigned int maxConnections = 1024;

//bool checkStatus()
//{
    //if (in[0] == SFTP::SUCCESS)
        //return true;

    //return false;
//}

//// Takes string for file path to prevent buffer issues
//void grabFile(Socket* client, const std::string& filePath)
//{
    //// Temporary buffers so that the async buffer doesn't share with the sync one
    //char tempIn[BUFLEN];
    //char tempOut[BUFLEN];

    //User* user = getUserByClient(client);
    //if (user == NULL)
    //{
        //client->send(SFTP::createFailureResponse(tempOut, SFTP::NOT_LOGGED_IN), tempOut);
        //return;
    //}

    //std::string path;
    //path = generateNewPath(user->currentDir, filePath);
    
    //std::ifstream file;
    //file.open(path, std::ios::binary);

    //file.seekg(0, file.end);
    //int fileSize = file.tellg();
    //file.seekg(0, file.beg);

    //uint32_t totalPackets = floor(fileSize / (BUFLEN - 4)) + 1;

    //client->send(SFTP::crGrabPrimary(tempOut, totalPackets, filePath), tempOut);

    //if (file.is_open())
    //{
        //while (!file.eof())
        //{
            //size_t i = 3; // Starts at 3 to leave space for status byte and length 
            //clearBuffer(BUFLEN, tempOut);
            //while (i < BUFLEN - 1) // Leave space for null termination
            //{
                //char c = (char) file.get();
                //if (file.eof())
                    //break;

                //tempOut[i] = c;
                //++i;
            //}
            
            //uint16_t length = i - 3;

            //[> crGrab works differently from all other response facotries
             //* Instead of overwriting buffer, it only changes the status byte
             //* and the length

             //* This is done to prevent copying of the entire buffer which would be inefficient
             //*/
            //client->send(SFTP::crGrab(tempOut, length), tempOut);
        //}
        //file.close();
    //}
    //else
    //{
        //client->send(SFTP::createFailureResponse(tempOut, SFTP::FAILED_TO_OPEN_FILE), tempOut);
        //return;
    //}
//}

//void receiveFile()
//{
    //// TODO figure this out
//}

void cleanConnections()
{
    std::vector<Connection*>::size_type i = 0;
    while (i < connections.size())
    {
        if (!(connections[i]->isActive()))
        {
            std::cout << "Deleting: " << connections[i]->Name() << std::endl;
            Connection* c = connections[i];
            connections.erase(connections.begin() + i);

            delete c;
            continue; // Keeps index valid
        }
        ++i;
    }
}

void listen(ServerSocket& server)
{
    char out[BUFLEN];

    LOGGER::DebugLog("Listening for clients...");
    while (true)
    {
        Socket* sock;
        sock = server.accept();
        if (sock < 0)
            LOGGER::LogError("Coundn't accept connection");
        
        if (connections.size() + 1 > maxConnections)
        {
            clearBuffer(BUFLEN, out);
            sock->send(SFTP::createFailureResponse(out, SFTP::SERVER_FULL), out);
            sock->close();

            delete sock;
            continue;
        }

        // Deletes any inactive connection objects
        cleanConnections();
        
        /* I allocate the Connection object to the heap
         * so that it doesn't go out of scope and get destroyed
         */
        Connection* connection = new Connection(sock);
        connections.push_back(connection);
        connection->start();
        
        LOGGER::Log("Client " + sock->Name() + " Established Connection", LOGGER::COLOR::GREEN);
        std::cout << "Number of Connections: " << connections.size() << std::endl;
    }

    // TODO maybe add cmd interface for server running on a seperate thread
}

void onExit(int signum)
{
    std::cout << std::endl;
    cleanConnections();

    std::cout << "\nbye" << std::endl;
    exit(signum);
}

int main(int argc, char** argv)
{
    signal(SIGINT, &onExit);

    /* Although serverSocket is not explicitly closed on SIGINT its memory
     * will automatically be freed by default when the program exits
     */
    ServerSocket serverSocket("127.0.0.1", PORT);
    listen(serverSocket);

    serverSocket.close();
    return 0;
}

