#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <SFML/Network.hpp>
#include "Updatable.h"
#include "stringImproved.h"

// Placeholder for empty URL-parameters
#define sFALSE  "_FALSE_"
// Use _OBJECT_ in URL-parameters to set used object to
// something other than the default playerShip(-1)
#define sOBJECT "_OBJECT_"

typedef std::vector<string> expandedIP;

enum ConnPermission
{
    PERM_NONE,
    PERM_R,
    PERM_RW

};

const char HEX2DEC[256] =
{
    /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
    /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,

    /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

class HttpRequest : public sf::NonCopyable
{
public:
    string method;
    string path;
    string post_data;
    std::map<string, string> headers;
    std::map<string, string> parameters;
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

    const static size_t recvBufferSize = 2048;
    char recvBuffer[recvBufferSize];
    size_t recvBufferCount;
    HttpRequest request;
    HttpServer* server;
    int reply_code;
    bool headers_send;
public:
    sf::TcpSocket socket;
    ConnPermission permission;
    HttpServerConnection(HttpServer* server);
    bool read();
    void sendData(const char* data, size_t data_length);
    void sendString(string data) { sendData(data.c_str(), data.length()); }
    ~HttpServerConnection();
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
    int portNr;
    sf::TcpListener listenSocket;
    sf::SocketSelector selector;
    std::vector<HttpServerConnection*> connections;
    std::vector<HttpRequestHandler*> handlers;
    bool testPermissions(expandedIP & ipAddr, ConnPermission permission);

public:
    HttpServer(int portNr = 80);
    ~HttpServer();
    std::vector<string> allow_r_from;
    std::vector<string> allow_rw_from;

    ConnPermission setPermissions(HttpServerConnection * connection);
    void addHandler(HttpRequestHandler* handler) { handlers.push_back(handler); }

    virtual void update(float delta);

    friend class HttpServerConnection;
private:
};


#endif//HTTP_SERVER_H
