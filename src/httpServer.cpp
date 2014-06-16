#include <string.h>
#include <stdio.h>

#include "httpServer.h"
#include "stringImproved.h"

HttpServer::HttpServer(std::string fileBasePath, int portNr)
: thread(&HttpServer::handleSocketsThread, this)
{
    this->fileBasePath = fileBasePath;
    listenSocket.listen(portNr);
    selector.add(listenSocket);
    running = true;
    thread.launch();
}

HttpServer::~HttpServer()
{
    listenSocket.close();
    running = false;
    thread.wait();
    for(unsigned int n=0; n<connections.size(); n++)
        delete connections[n];
}

void HttpServer::handleSocketsThread()
{
    while(running)
    {
        if (selector.wait())
        {
            if (!running)
                break;
            if (selector.isReady(listenSocket))
            {
                HttpServerConnection* connection = new HttpServerConnection();
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
}

HttpServerConnection::HttpServerConnection()
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
        ptr++;
        size_t len = ptr - recvBuffer;
        char line[recvBufferSize];
        memcpy(line, recvBuffer, len);
        recvBufferCount -= len;
        memmove(recvBuffer, ptr, recvBufferCount);
        line[len] = '\0';
        while(len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (!handleLine(line))
            return false;
    }
    return true;
}

bool HttpServerConnection::handleLine(char* line)
{
    switch(status)
    {
    case METHOD:{
        char* pathPtr = strchr(line, ' ');
        if (!pathPtr)
            return false;
        char* httpVersionPtr = strchr(pathPtr+1, ' ');
        if (!httpVersionPtr)
            return false;
        *pathPtr++ = '\0';
        *httpVersionPtr++ = '\0';

        requestMethod = line;
        requestPath = pathPtr;

        status = HEADERS;
        }break;
    case HEADERS:
        if (strlen(line) == 0)
        {
            status = METHOD;
            sendReply();
        }else{
            //printf("Header %i: %s\n", strlen(line), line);
        }
        break;
    case BODY:
        status = HEADERS;
        break;
    }
    return true;
}

void HttpServerConnection::sendReply()
{
    const char* replyCode = "200";
    std::string replyData = "";
    FILE* f = NULL;
    if (requestPath == "/")
        requestPath = "/index.html";
    if (requestPath.find("..") != std::string::npos)
    {
        replyCode = "403";
        replyData = "Forbidden";
    }else{
        std::string fullPath = "www/" + requestPath;
        f = fopen(fullPath.c_str(), "rb");
        if (!f)
        {
            replyCode = "404";
            replyData = "File not found";
        }
    }
    size_t replyDataSize = replyData.size();
    std::string reply = std::string("HTTP/1.1 ") + replyCode + " OK\r\n";
    if (f)
    {
        fseek(f, 0, SEEK_END);
        replyDataSize = ftell(f);
        fseek(f, 0, SEEK_SET);
    }
    reply += "Content-type: text/html\r\n";
    reply += "Content-length: " + string(replyDataSize) + "\r\n";
    reply += "\r\n";
    socket.send(reply.c_str(), reply.size());
    if (replyData.size() > 0)
        socket.send(replyData.c_str(), replyData.size());
    if (f)
    {
        while(true)
        {
            char buffer[1024];
            size_t n = fread(buffer, 1, sizeof(buffer), f);
            if (n < 1)
                break;
            socket.send(buffer, n);
        }
        fclose(f);
    }
}
