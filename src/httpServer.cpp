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

HttpServerConnection::~HttpServerConnection()
{
    socket.disconnect();
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
                if (setPermissions(connection))
                {
                    connections.push_back(connection);
                    selector.add(connection->socket);
                }
                else delete connection;
            } else delete connection;
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

/** \brief Return true if the remote IP is allowed to do stuff
 * Match IP-address with list of addresses in HttpServer::allowed_*_from
 * Wildcard character is *
 *
 * \param HttpServerConnection * connection: Pointer to a Connection-object
 * \return ConnPermission: Current permissions for this connection.
 *
 */
ConnPermission HttpServer::setPermissions(HttpServerConnection * connection)
{
    sf::IpAddress ipAddr = connection->socket.getRemoteAddress();
    std::vector<string> allow_from;
    string sIpAddr;
    expandedIP remoteIp;

    sIpAddr = (string) ipAddr.toString();
    remoteIp = sIpAddr.split(".");

    if (testPermissions(remoteIp, PERM_EXEC))
    {
        LOG(DEBUG) << "Accepted connection from " << sIpAddr << " with Execute permissions";
        connection->permission = PERM_EXEC;
        return PERM_EXEC;
    }
    else if (testPermissions(remoteIp, PERM_RW))
    {
        LOG(DEBUG) << "Accepted connection from " << sIpAddr << " with Read/Write permissions";
        connection->permission = PERM_RW;
        return PERM_RW;
    }
    else if (testPermissions(remoteIp, PERM_R))
    {
        LOG(DEBUG) << "Accepted connection from " << sIpAddr << " with Read permissions";
        connection->permission = PERM_R;
        return PERM_R;
    }

    LOG(DEBUG) << "Rejected connection from " << sIpAddr;
    connection->permission = PERM_NONE;
    return PERM_NONE;
}

/** \brief Test if an IP address matches permission-level
 * Match address part by part, both literal and with *-wildcard
 * \param remoteIp expandedIP&
 * \param permission ConnPermission
 * \return bool: True if address matches the given permission
 *
 */
bool HttpServer::testPermissions(expandedIP & remoteIp, ConnPermission permission)
{
    bool success;
    expandedIP filterIp;
    std::vector<string> * allow_from;

    // Switch the allow_from pointer to the list of addresses in HttpServer
    if (permission == PERM_R)
        allow_from = &allow_r_from;
    else if (permission == PERM_RW)
        allow_from = &allow_rw_from;
    else if (permission == PERM_EXEC)
        allow_from = &allow_exec_from;

    for (unsigned int i = 0; i<allow_from->size(); i++ )
    {
        success = true;
        filterIp = allow_from->at(i).split(".");

        for (unsigned int n = 0; n<4; n++)
        {
            if ((filterIp[n] != remoteIp[n]) and (filterIp[n] != "*"))
            {
                success = false; // Match failed
                break;
            }
        }
        if (success == true)
            return success; // We have a match
    }
    return success;
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
   const int SRC_LEN = sSrc.length();
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
        request.path = uri;
    else
    {
        std::vector<string> parts = uri.split("?", 1);
        request.path = parts[0];

        std::vector<string> parameters = parts[1].split("&");
        for (unsigned int n=0; n<parameters.size(); n++)
        {
            string param = parameters[n];
            std::size_t found = param.find('=');
            if (found==std::string::npos)
            {
                request.parameters[param] = sFALSE;
                LOG(DEBUG) << "HTTP Parameter: " << param;
            }
            else
            {
                if (param.endswith('='))
                {
                    request.parameters[param.substr(0, param.length()-1)] = sFALSE;
                    LOG(DEBUG) << "HTTP Parameter: " << param.substr(0, param.length()-1);
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
            LOG(DEBUG) << "HTTP request: " << request.path;
#ifdef DEBUG
            for (std::map<string, string>::iterator iter = request.headers.begin(); iter != request.headers.end(); iter++)
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
    string chunk_len_string = string::hex(data_length) + "\r\n";
    socket.send(chunk_len_string.c_str(), chunk_len_string.size());
    socket.send(data, data_length);
    socket.send("\r\n", 2);
}

//const char HttpServerConnection::

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
