#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <SFML/Network.hpp>
#include "Updatable.h"
#include "stringImproved.h"

class HttpServerConnection: public sf::NonCopyable
{
public:
    enum Status
    {
        METHOD,
        HEADERS,
        BODY
    } status;

    sf::TcpSocket socket;
    const static size_t recvBufferSize = 2048;
    char recvBuffer[recvBufferSize];
    size_t recvBufferCount;
    
    std::string requestMethod;
    std::string requestPath;
    
    HttpServerConnection();
    
    bool read();
    
    bool handleLine(string line);
    
    void sendReply();
};

class HttpServer: public Updatable
{
private:
    int portNr;
    sf::TcpListener listenSocket;
    sf::SocketSelector selector;
    std::vector<HttpServerConnection*> connections;
    string fileBasePath;
public:
    HttpServer(string fileBasePath, int portNr = 80);
    ~HttpServer();
    
    virtual void update(float delta);
};


#endif//HTTP_SERVER_H
