#include <string.h>
#include <stdio.h>

#include "httpServer.h"
#include "logging.h"

HttpServer::HttpServer(int portNr)
{
    listenSocket.listen(portNr);
    selector.add(listenSocket);
}

HttpServer::~HttpServer()
{
    listenSocket.close();
    for(unsigned int n=0; n<connections.size(); n++)
        delete connections[n];
    for(unsigned int n=0; n<handlers.size(); n++)
        delete handlers[n];
}

void HttpServer::update(float delta)
{
    if (selector.wait(sf::microseconds(1)))
    {
        if (selector.isReady(listenSocket))
        {
            HttpServerConnection* connection = new HttpServerConnection(this);
            if (listenSocket.accept(connection->socket) == sf::Socket::Done)
            {
                connections.push_back(connection);
                selector.add(connection->socket);
            }else{
                delete connection;
            }
        }
        for(unsigned int n=0; n<connections.size(); n++)
        {
            if (selector.isReady(connections[n]->socket))
            {
                if (!connections[n]->read())
                {
                    selector.remove(connections[n]->socket);
                    delete connections[n];
                    connections.erase(connections.begin() + n);
                }
            }
        }
    }
}

HttpServerConnection::HttpServerConnection(HttpServer* server)
: server(server)
{
    recvBufferCount = 0;
    status = METHOD;
}

bool HttpServerConnection::read()
{
    char buffer[1024];
    size_t size;
    if (socket.receive(buffer, sizeof(buffer), size) != sf::Socket::Done)
        return false;
    if (recvBufferCount + size > recvBufferSize)
        size = recvBufferSize - recvBufferCount;
    if (size < 1)
        return false;
    memcpy(recvBuffer + recvBufferCount, buffer, size);
    recvBufferCount += size;
    
    while(true)
    {
        char* ptr = (char*)memchr(recvBuffer, '\n', recvBufferCount);
        if (!ptr)
            break;
        *ptr = '\0';
        string line(recvBuffer);
        ptr++;
        size_t len = ptr - recvBuffer;
        recvBufferCount -= len;
        memmove(recvBuffer, ptr, recvBufferCount);
        if (line.endswith("\r"))
            line = line.substr(0, -1);
        if (!handleLine(line))
            return false;
    }
    return true;
}

bool HttpServerConnection::handleLine(string line)
{
    switch(status)
    {
    case METHOD:{
        std::vector<string> parts = line.split();
        if (parts.size() != 3)
            return false;
        request.method = parts[0];
        request.path = parts[1];
        status = HEADERS;
        }break;
    case HEADERS:
        if (line.length() == 0)
        {
            request.post_data = "";
            if (request.method == "POST")
            {
                if (request.headers.find("content-length") != request.headers.end())
                {
                    unsigned int body_length = request.headers["content-length"].toInt();
                    if (body_length > recvBufferSize)
                        return false;
                    while (body_length > recvBufferCount)
                    {
                        size_t received;
                        if (socket.receive(recvBuffer + recvBufferCount, body_length - recvBufferCount, received) != sf::Socket::Done)
                            return false;
                        recvBufferCount += received;
                    }
                    request.post_data = string(recvBuffer, body_length);
                    recvBufferCount -= body_length;
                    memmove(recvBuffer, recvBuffer + body_length, recvBufferCount);
                }
            }
            status = METHOD;
            LOG(DEBUG) << "HTTP request:" << request.path;
            handleRequest();
        }else{
            std::vector<string> parts = line.split(":", 1);
            if (parts.size() != 2)
                LOG(WARNING) << "Invalid HTTP header: " << line;
            else
                request.headers[parts[0].strip().lower()] = parts[1];
        }
        break;
    }
    return true;
}

void HttpServerConnection::handleRequest()
{
    reply_code = 200;
    headers_send = false;
    
    for(unsigned int n=0; n<server->handlers.size(); n++)
    {
        if (server->handlers[n]->handleRequest(request, this))
            break;
        if (headers_send)
            break;
    }
    
    if (!headers_send)
    {
        reply_code = 404;
        string replyData = "File not found";
        sendData(replyData.c_str(), replyData.size());
    }
    string end_chunk = "0\r\n\r\n";
    socket.send(end_chunk.c_str(), end_chunk.size());
}

void HttpServerConnection::sendHeaders()
{
    string reply = string("HTTP/1.1 ") + string(reply_code) + " OK\r\n";
    reply += "Content-type: text/html\r\n";
    reply += "Connection: Keep-Alive\r\n";
    reply += "Transfer-Encoding: chunked\r\n";
    reply += "\r\n";
    socket.send(reply.c_str(), reply.size());
    headers_send = true;
}

void HttpServerConnection::sendData(const char* data, size_t data_length)
{
    if (data_length < 1)
        return;
    if (!headers_send)
        sendHeaders();
    string chunk_len_string = string::hex(data_length) + "\r\n";
    socket.send(chunk_len_string.c_str(), chunk_len_string.size());
    socket.send(data, data_length);
    socket.send("\r\n", 2);
}

bool HttpRequestFileHandler::handleRequest(HttpRequest& request, HttpServerConnection* connection)
{

    string replyData = "";
    FILE* f = NULL;
    if (request.path == "/")
        request.path = "/index.html";
    if (request.path.find("..") != -1)
        return false;
    
    string fullPath = base_path + request.path;
    f = fopen(fullPath.c_str(), "rb");
    if (!f)
        return false;

    while(true)
    {
        char buffer[1024];
        size_t n = fread(buffer, 1, sizeof(buffer), f);
        if (n < 1)
            break;
        connection->sendData(buffer, n);
    }
    fclose(f);
    return true;
}
