#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <SFML/Network.hpp>

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
    
    bool handleLine(char* line);
    
    void sendReply();
};

class HttpServer: private sf::NonCopyable
{
private:
    int portNr;
    sf::TcpListener listenSocket;
    sf::SocketSelector selector;
    sf::Thread thread;
    bool running;
    std::vector<HttpServerConnection*> connections;
    std::string fileBasePath;
public:
    HttpServer(std::string fileBasePath, int portNr = 80);
    ~HttpServer();
    
private:
    void handleSocketsThread();
};


#endif//HTTP_SERVER_H
