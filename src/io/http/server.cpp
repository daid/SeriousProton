#include <io/http/server.h>
#include <io/network/selector.h>
#include <stringutil/sha1.h>
#include <logging.h>

#include <string.h>

/**< Map to convert between character encodings */
static const signed char HEX2DEC[256] =
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


/** \brief Decode a percent-encoded URI
 * Uri decoding according to RFC1630, RFC1738, RFC2396
 * Credits: Jin Qing
 * \param sSrc const string&   Percent-encoded URI
 * \return string              Decoded URI-string
 *
 */
string uriDecode(const string& sSrc)
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


namespace sp {
namespace io {
namespace http {
namespace websocket {
    static constexpr int fin_mask = 0x80;
    static constexpr int rsv_mask = 0x70;
    static constexpr int opcode_mask = 0x0f;
    
    static constexpr int mask_mask = 0x80;
    static constexpr int payload_length_mask = 0x7f;
    static constexpr int payload_length_16bit = 126;
    static constexpr int payload_length_64bit = 127;
    
    static constexpr int opcode_continuation = 0x00;
    static constexpr int opcode_text = 0x01;
    static constexpr int opcode_binary = 0x02;
    static constexpr int opcode_close = 0x08;
    static constexpr int opcode_ping = 0x09;
    static constexpr int opcode_pong = 0x0a;
};

Server::Server(int port_nr)
{
    if (!listen_socket.listen(port_nr))
    {
        LOG(Error, "Failed to listen on port:", port_nr, "for http server");
        return;
    }

    handler_thread = std::thread([this]() { handlerThread(); });
}

Server::~Server()
{
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        listen_socket.close();
    }
    handler_thread.join();
}

void Server::setStaticFilePath(const string& static_file_path)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    
    this->static_file_path = static_file_path;
    if (!this->static_file_path.endswith("/"))
        this->static_file_path += "/";
}

void Server::addURLHandler(const string& url, std::function<string(const Request&)> func)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    
    http_handlers[url] = func;
}

void Server::addSimpleWebsocketHandler(const string& url, std::function<void(const string& data)> func)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    
    simple_websocket_handlers[url] = func;
}

void Server::addAdvancedWebsocketHandler(const string& url, std::function<P<WebsocketHandler>()> func)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    
    advanced_websocket_handlers[url] = func;
}

void Server::broadcastToWebsockets(const string& url, const string& data)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    
    for(Connection& connection : connections)
    {
        if (connection.remove)
            continue;
        if (connection.state == Connection::State::Websocket && connection.request.path == url)
            connection.sendWebsocketTextPacket(data);
    }
}

void Server::handlerThread()
{
    sp::io::network::Selector selector;
    selector.add(listen_socket);
    while(listen_socket.isListening())
    {
        selector.wait(1000);

        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (selector.isReady(listen_socket))
        {
            connections.emplace_back(*this);
            Connection& connection = connections.back();
            listen_socket.accept(connection.socket);
            connection.last_received_data_time = std::chrono::steady_clock::now();
            selector.add(connection.socket);
        }
        for(auto it = connections.begin(); it != connections.end();)
        {
            Connection& connection = *it;
            if (connection.remove)
            {
                it++;
                continue;
            }
            if (selector.isReady(connection.socket))
            {
                connection.last_received_data_time = std::chrono::steady_clock::now();
                connection.remove = !connection.processIncommingData();
            }
            else
            {
                if (std::chrono::steady_clock::now() - connection.last_received_data_time > std::chrono::seconds(5))
                    connection.remove = !connection.handleTimeout();
            }
            
            if (connection.remove)
                selector.remove(connection.socket);
            it++;
        }
    }
}

void Server::update(float delta)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    for(auto it = connections.begin(); it != connections.end();)
    {
        Connection& connection = *it;

        if (connection.remove)
        {
            if (connection.websocket_handler)
            {
                connection.websocket_handler->connection = nullptr;
                connection.websocket_handler->onDisconnect();
            }
            it = connections.erase(it);
            continue;
        }
        
        if (connection.request_pending)
        {
            string reply = http_handlers[connection.request.path](connection.request);
            connection.startHttpReply(200);
            if (reply.size() > 0)
                connection.httpChunk(reply);
            connection.httpChunk("");

            connection.request_pending = false;
        }
        if (connection.websocket_connected)
        {
            if (advanced_websocket_handlers.find(connection.request.path) != advanced_websocket_handlers.end())
                connection.websocket_handler = advanced_websocket_handlers[connection.request.path]();
            if (connection.websocket_handler)
            {
                connection.websocket_handler->connection = &connection;
                connection.websocket_handler->onConnect();
            }
            connection.websocket_connected = false;
        }
        for(string& message : connection.websocket_received_pending)
        {
            if (simple_websocket_handlers.find(connection.request.path) != simple_websocket_handlers.end())
                simple_websocket_handlers[connection.request.path](message);
            if (connection.websocket_handler)
                connection.websocket_handler->onMessage(message);
        }
        connection.websocket_received_pending.clear();
        
        it++;
    }
}

Server::Connection::Connection(Server& server)
: server(server)
{
    buffer.reserve(4096);
    state = State::HTTPRequest;
    remove = false;
}

bool Server::Connection::processIncommingData()
{
    char receive_buffer[4096];
    size_t received_size;
    received_size = socket.receive(receive_buffer, sizeof(receive_buffer));
    if (received_size < 1)
        return false;
    buffer.resize(buffer.size() + received_size);
    memcpy(&buffer[buffer.size()] - received_size, receive_buffer, received_size);
    
    switch(state)
    {
    case State::HTTPRequest:{
        int headers_end = buffer.find("\r\n\r\n");
        if (headers_end)
        {
            std::vector<string> header_data = buffer.substr(0, headers_end).split("\r\n");
            
            std::vector<string> parts = header_data[0].split();
            if (parts.size() != 3)
                return false;
            auto path_query = parts[1].partition("?");
            request.method = parts[0];
            request.path = path_query.first;
            for(auto& param : path_query.second.split("&"))
            {
                auto key_value = param.partition("=");
                if (!key_value.first.empty())
                    request.query[uriDecode(key_value.first)] = uriDecode(key_value.second);
            }
            request.post_data = "";
            request.headers.clear();
            for(unsigned int n=1; n<header_data.size(); n++)
            {
                auto header_entry = header_data[n].partition(":");
                request.headers[header_entry.first.strip().lower()] = header_entry.second.strip();
            }
            int post_length = 0;
            if (request.headers.find("content-length") != request.headers.end())
            {
                post_length = request.headers["content-length"].toInt();
                if (int(buffer.size()) < headers_end + 4 + post_length)
                    break; //Not enough data yet, continue receiving.
                request.post_data = buffer.substr(headers_end + 4, headers_end + 4 + post_length);
            }
            
            buffer = buffer.substr(headers_end + 4 + post_length);
            handleRequest(request);
        }
        }break;
    case State::Websocket:
        while(true)
        {
            if (buffer.size() < 2)
                return true;
            unsigned int payload_length = buffer[1] & websocket::payload_length_mask;
            int opcode = buffer[0] & websocket::opcode_mask;
            bool fin = buffer[0] & websocket::fin_mask;
            bool mask = buffer[1] & websocket::mask_mask;
            unsigned int index = 2;

            //Close the connection if any of the RSV bits are set.
            if (buffer[0] & websocket::rsv_mask)
            {
                LOG(Warning, "Closing websocket due to RSV bits, we do not support extensions.");
                return false;
            }

            if (payload_length == websocket::payload_length_16bit)
            {
                if (buffer.size() < index + 2)
                    return true;
                payload_length = uint8_t(buffer[index++]) << 8;
                payload_length |= uint8_t(buffer[index++]);
            }else if (payload_length == websocket::payload_length_64bit)
            {
                if (buffer.size() < index + 8)
                    return true;
                index += 4;
                payload_length = uint8_t(buffer[index++]) << 24;
                payload_length |= uint8_t(buffer[index++]) << 16;
                payload_length |= uint8_t(buffer[index++]) << 8;
                payload_length |= uint8_t(buffer[index++]);
            }

            uint8_t mask_values[4] = {0, 0, 0, 0};
            if (mask)
            {
                if (buffer.size() < index + 4)
                    return true;
                for(unsigned int n=0; n<4; n++)
                    mask_values[n] = buffer[index++];
                if (buffer.size() < index + payload_length)
                    return true;
                for(unsigned int n=0; n<payload_length; n++)
                    buffer[index + n] ^= mask_values[n % 4];
            }
            if (buffer.size() < index + payload_length)
                return true;
            
            string message = buffer.substr(index, index + payload_length);
            buffer = buffer.substr(index + payload_length);

            switch(opcode)
            {
            case websocket::opcode_continuation:
                websocket_received_fragment += message;
                if (fin)
                {
                    websocket_received_pending.push_back(std::move(websocket_received_fragment));
                    websocket_received_fragment = "";
                }
                break;
            case websocket::opcode_text:
            case websocket::opcode_binary:
                if (fin)
                    websocket_received_pending.push_back(message);
                else
                    websocket_received_fragment = message;
                break;
            case websocket::opcode_close:
                {
                    uint8_t reply[] = {websocket::fin_mask | websocket::opcode_close, 0};//close packet
                    socket.send(reply, sizeof(reply));
                }
                return false;
            case websocket::opcode_ping:
                {
                    //Note: The standard says that we need to include the payload of the ping packet as payload in the pong packet.
                    //      We ignore this, as this no client seems to use this.
                    uint8_t reply[] = {websocket::fin_mask | websocket::opcode_pong, 0};//pong packet
                    socket.send(reply, sizeof(reply));
                }
                break;
            case websocket::opcode_pong:
                //There is no real need to track PONG replies. TCP/IP will close the connection if the other side is gone.
                break;
            }
        }
        break;
    }
    
    return true;
}

bool Server::Connection::handleTimeout()
{
    switch(state)
    {
    case State::HTTPRequest:{
        return false;
        }break;
    case State::Websocket:{
        uint8_t ping[] = {websocket::fin_mask | websocket::opcode_ping, 0};
        socket.send(ping, sizeof(ping));
        }break;
    }
    return true;
}

void Server::Connection::handleRequest(const Request& request)
{
    if (server.http_handlers.find(request.path) != server.http_handlers.end())
    {
        //Mark this request to be handled in the main thread.
        request_pending = true;
        return;
    }

    //Check if this is a websocket upgrade request.
    //We allow any path to be a websocket.
    if (request.method == "GET"
    && request.headers.find("upgrade") != request.headers.end()
    && request.headers.find("connection") != request.headers.end()
    && request.headers.find("sec-websocket-version") != request.headers.end()
    && request.headers.find("sec-websocket-key") != request.headers.end()
    )
    {
        if (request.headers.find("upgrade")->second.lower() == "websocket"
        && request.headers.find("connection")->second.lower() == "upgrade"
        && request.headers.find("sec-websocket-version")->second.lower() == "13"
        )
        {
            if (server.simple_websocket_handlers.find(request.path) != server.simple_websocket_handlers.end() || server.advanced_websocket_handlers.find(request.path) != server.advanced_websocket_handlers.end())
            {
                string reply = "HTTP/1.1 101 Switching Protocols\r\n";
                reply += "Upgrade: websocket\r\n";
                reply += "Connection: upgrade\r\n";
                reply += "Sec-WebSocket-Accept: " + stringutil::SHA1(request.headers.find("sec-websocket-key")->second + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").base64() + "\r\n";
                if (request.headers.find("sec-websocket-protocol") != request.headers.end())
                    reply += "Sec-WebSocket-Protocol: chat\r\n";
                reply += "Expires: 0\r\n";
                reply += "Cache-Control: max-age=0, no-cache, must-revalidate, proxy-revalidate\r\n";
                reply += "\r\n";
                socket.send(reply.c_str(), reply.size());
                
                websocket_connected = true;
                
                state = State::Websocket;
            }
            else
            {
                LOG(Warning, "Tried to open websocket without handler:", request.path);
                startHttpReply(404);
                httpChunk("404 - File not found.");
                httpChunk("");
            }
            return;
        }
    }

    string full_path;
    
    if (request.path.find("..") == -1)
    {
        full_path = server.static_file_path + request.path;
        if (request.path.endswith("/"))
            full_path = full_path + "index.html";
    }
    
    FILE* f = fopen(full_path.c_str(), "rb");
    if (f)
    {
        if (request.path.endswith(".js"))
            startHttpReply(200, "text/javascript");
        else if (request.path.endswith(".css"))
            startHttpReply(200, "text/css");
        else
            startHttpReply(200);
        while(true)
        {
            char buffer[1024];
            size_t n = fread(buffer, 1, sizeof(buffer), f);
            if (n < 1)
                break;

            string chunk_len_string = string::hex(n) + "\r\n";
            socket.send(chunk_len_string.c_str(), chunk_len_string.size());
            socket.send(buffer, n);
            socket.send("\r\n", 2);
        }
        fclose(f);
    }
    else
    {
        startHttpReply(404);
        httpChunk("404 - File not found.");

        LOG(Warning, "File not found:", request.path);
    }
    httpChunk("");
}

void Server::Connection::startHttpReply(int reply_code, const string& mimetype)
{
    string reply = string("HTTP/1.1 ") + string(reply_code) + " OK\r\n";
    reply += "Connection: Keep-Alive\r\n";
    if (mimetype.length() > 0)
        reply += "Content-Type: " + mimetype + "\r\n";
    reply += "Transfer-Encoding: chunked\r\n";
    reply += "\r\n";
    socket.send(reply.c_str(), reply.size());
}

void Server::Connection::httpChunk(const string& data)
{
    string chunk_len_string = string::hex(data.size()) + "\r\n";
    socket.send(chunk_len_string.c_str(), chunk_len_string.size());
    if (data.size() > 0)
        socket.send(data.data(), data.size());
    socket.send("\r\n", 2);
}

void Server::Connection::sendWebsocketTextPacket(const string& data)
{
    if (data.size() < websocket::payload_length_16bit)
    {
        uint8_t header[] = {
            websocket::fin_mask | websocket::opcode_text,
            uint8_t(data.size()),
        };
        socket.send(header, sizeof(header));
    }
    else if (data.size() < (1 << 16))
    {
        uint8_t header[] = {
            websocket::fin_mask | websocket::opcode_text,
            websocket::payload_length_16bit,
            uint8_t((data.size() >> 8) & 0xFF),
            uint8_t(data.size() & 0xFF),
        };
        socket.send(header, sizeof(header));
    }
    else
    {
        uint8_t header[] = {
            websocket::fin_mask | websocket::opcode_text,
            websocket::payload_length_64bit,
            0, 0, 0, 0,
            uint8_t((data.size() >> 24) & 0xFF),
            uint8_t((data.size() >> 16) & 0xFF),
            uint8_t((data.size() >> 8) & 0xFF),
            uint8_t(data.size() & 0xFF),
        };
        socket.send(header, sizeof(header));
    }
    
    socket.send(data.data(), data.size());
}

void WebsocketHandler::send(const string& message)
{
    if (connection)
        connection->sendWebsocketTextPacket(message);
}

}//namespace http
}//namespace io
}//namespace sp
