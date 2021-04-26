#include <string.h>
#include <stdio.h>

#include "httpServer.h"
#include "logging.h"

HttpServer::HttpServer(int portNr)
{
    listenSocket.listen(static_cast<uint16_t>(portNr));
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

void HttpServer::update(float /*delta*/)
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

/** \brief Decode a percent-encoded URI
 * Uri decoding according to RFC1630, RFC1738, RFC2396
 * Credits: Jin Qing
 * \param sSrc const string&   Percent-encoded URI
 * \return string              Decoded URI-string
 *
 */
string HttpServerConnection::UriDecode(const string & sSrc)
{
   // Note from RFC1630: "Sequences which start with a percent
   // sign but are not followed by two hexadecimal characters
   // (0-9, A-F) are reserved for future extension"

   const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
   const size_t SRC_LEN = sSrc.length();
   const unsigned char * const SRC_END = pSrc + SRC_LEN;
   // last decodable '%'
   const unsigned char * const SRC_LAST_DEC = SRC_END - 2;

   char * const pStart = new char[SRC_LEN];
   char * pEnd = pStart;

   while (pSrc < SRC_LAST_DEC)
   {
      if (*pSrc == '%')
      {
         char dec1, dec2;
         if (-1 != (dec1 = HEX2DEC[*(pSrc + 1)])
            && -1 != (dec2 = HEX2DEC[*(pSrc + 2)]))
         {
            *pEnd++ = (dec1 << 4) + dec2;
            pSrc += 3;
            continue;
         }
      }

      *pEnd++ = *pSrc++;
   }

   // the last 2- chars
   while (pSrc < SRC_END)
      *pEnd++ = *pSrc++;

   std::string sResult(pStart, pEnd);
   delete [] pStart;
   return (string) sResult;
}


/**< Map to convert between character encodings */
const signed char HttpServerConnection::HEX2DEC[256] =
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


/** \brief Parse a URL, splitting it in its part and optional parameters
 *
 * \param sSrc const string&  URL
 * \return void
 *
 */
void HttpServerConnection::parseUri(const string & sSrc)
{
    string uri = UriDecode(sSrc);
    std::size_t found = uri.find('?');
    if (found==std::string::npos)
    {
        request.path = uri;
        return;
    }
    else
    {
        std::vector<string> parts = uri.split("?", 1);
        request.path = parts[0];

        std::vector<string> parameters = parts[1].split("&");
        for (unsigned int n=0; n<parameters.size(); n++)
        {
            string param = parameters[n];
            found = param.find('=');
            if (found==std::string::npos)
            {
                request.parameters[param] = "";
                LOG(DEBUG) << "HTTP Parameter: " << param;
            }
            else
            {
                if (param.endswith('='))
                {
                    auto param_end = static_cast<int>(param.length()) - 1;
                    auto param_key = param.substr(0, param_end);
                    request.parameters[param_key] = "";
                    LOG(DEBUG) << "HTTP Parameter: " << param_key;
                }
                else
                {
                    std::vector<string> items = param.split("=", 1);
                    request.parameters[items[0]] = items[1];
                    LOG(DEBUG) << "HTTP Parameter: " << items[0] << " = " << items[1];
                }
            }
        }
    }
    LOG(DEBUG) << "HTTP Path: " << request.path;
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
        parseUri(parts[1]);
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
#ifdef DEBUG
            for (std::unordered_map<string, string>::iterator iter = request.headers.begin(); iter != request.headers.end(); iter++)
            {
                string key=iter->first;
                string value=iter->second;
                LOG(DEBUG) << "HTTP header: (" << key << ", " << value << ")";
            }
#endif // DEBUG
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
    request.parameters.clear();
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
    if (!headers_send)
        sendHeaders();
    if (data_length < 1)
        return;
    string chunk_len_string = string::hex(static_cast<int>(data_length)) + "\r\n";
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
