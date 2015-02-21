#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <SFML/Network.hpp>
#include "Updatable.h"
#include "stringImproved.h"

class HttpRequest : public sf::NonCopyable
{
public:
    string method;
    string path;
    string post_data;
    std::map<string, string> headers;
};

class HttpServerConnection;
class HttpRequestHandler : public sf::NonCopyable
{
public:
    virtual bool handleRequest(HttpRequest& request, HttpServerConnection* connection) = 0;
};

class HttpServerConnection: public sf::NonCopyable
{
private:
    enum Status
    {
        METHOD,
        HEADERS
    } status;

    const static size_t recvBufferSize = 2048;
    char recvBuffer[recvBufferSize];
    size_t recvBufferCount;
    
    HttpRequest request;

public:
    sf::TcpSocket socket;
    
    HttpServerConnection();
    bool read();

    void sendData(const char* data, size_t data_length);
private:    
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
