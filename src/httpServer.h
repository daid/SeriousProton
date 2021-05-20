#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <unordered_map>
#include <SFML/Network.hpp>
#include "Updatable.h"
#include "stringImproved.h"

class HttpRequest : public sf::NonCopyable
{
public:
    string method;
    string path;
    string post_data;
    std::unordered_map<string, string> headers;
    std::unordered_map<string, string> parameters;
};

class HttpServer;
class HttpServerConnection;
class HttpRequestHandler : public sf::NonCopyable
{
public:
    HttpRequestHandler() {}
    virtual ~HttpRequestHandler() {}

    virtual bool handleRequest(HttpRequest& request, HttpServerConnection* connection) = 0;
};

class HttpRequestFileHandler : public HttpRequestHandler
{
    string base_path;
public:
    HttpRequestFileHandler(string base_path) : base_path(base_path) {}

    virtual bool handleRequest(HttpRequest& request, HttpServerConnection* connection);
};

class HttpServerConnection: public sf::NonCopyable
{
private:
    enum Status
    {
        METHOD,
        HEADERS
    } status;

    constexpr static size_t recvBufferSize = 16384;
    char recvBuffer[recvBufferSize];
    size_t recvBufferCount;
    const static signed char HEX2DEC[256];
    HttpRequest request;
    HttpServer* server;
    int reply_code;
    bool headers_send;
public:
    sf::TcpSocket socket;

    HttpServerConnection(HttpServer* server);
    bool read();
    void sendData(const char* data, size_t data_length);
    void sendString(string data) { sendData(data.c_str(), data.length()); }
private:
    bool handleLine(string line);

    string UriDecode(const string & sSrc);
    void parseUri(const string & sSrc);
    void handleRequest();
    void sendHeaders();
};

class HttpServer: public Updatable
{
private:
    sf::TcpListener listenSocket;
    sf::SocketSelector selector;
    std::vector<HttpServerConnection*> connections;
    std::vector<HttpRequestHandler*> handlers;
public:
    HttpServer(int portNr = 80);
    ~HttpServer();

    void addHandler(HttpRequestHandler* handler) { handlers.push_back(handler); }

    virtual void update(float delta);

    friend class HttpServerConnection;
};


#endif//HTTP_SERVER_H
