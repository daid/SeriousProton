#include <io/http/websocket.h>
#include <stringutil/base64.h>
#include <stringutil/sha1.h>
#include <random.h>
#include <logging.h>

#ifdef EMSCRIPTEN
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <poll.h>
#endif


namespace sp {
namespace io {
namespace http {
#ifndef EMSCRIPTEN
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
#endif

Websocket::Websocket()
{
}

Websocket::Websocket(Websocket&& other)
{
    *this = std::move(other);
}

Websocket::~Websocket()
{
#ifdef EMSCRIPTEN
    close();
#endif
}

Websocket& Websocket::operator=(Websocket&& other)
{
    state = other.state;
#ifdef EMSCRIPTEN
    socket_handle = other.socket_handle;
    other.socket_handle = -1;
    other.state = State::Disconnected;
#else
    websock_key = std::move(other.websock_key);
    socket = std::move(other.socket);
    buffer = std::move(other.buffer);
    received_fragment = std::move(other.received_fragment);
    headers = std::move(other.headers);
    other.close();
#endif
    return *this;
}

void Websocket::setHeader(const string& key, const string& value)
{
#ifdef EMSCRIPTEN
    LOG(Error, "Tried to set a header on a websocket in web build. Which is not possible.");
#else
    headers[key] = value;
#endif
}

bool Websocket::connect(const string& url)
{
    close();

    Scheme scheme = Scheme::Http;
    int scheme_length = 5;
    if (!url.startswith("ws://") && !url.startswith("wss://"))
        return false;
    if (url.startswith("wss://"))
    {
        scheme_length = 6;
        scheme = Scheme::Https;
    }
    int end_of_hostname = url.find("/", scheme_length);
    string hostname = url.substr(scheme_length, end_of_hostname);
    int port = 80;
    if (hostname.find(":") != -1)
    {
        port = hostname.substr(hostname.find(":") + 1).toInt();
        hostname = hostname.substr(0, hostname.find(":"));
    }
    string path = url.substr(end_of_hostname);

    return connect(hostname, port, path, scheme);
}

bool Websocket::connect(const string& hostname, int port, const string& path, Scheme scheme)
{
    close();

    if (scheme == Scheme::Auto)
        scheme = port == 443 ? Scheme::Https : Scheme::Http;

#ifdef EMSCRIPTEN
    struct sockaddr_in server_addr;
    struct hostent* he = gethostbyname((hostname + path).c_str());
    if (!he)
        return false;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = *(u_long *)he->h_addr_list[0];
    server_addr.sin_port = htons(port);

    socket_handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_handle == -1)
        return false;
    fcntl(socket_handle, F_SETFL, O_NONBLOCK);
    if (::connect(socket_handle, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr)))
    {
        if (errno != EAGAIN && errno != EINPROGRESS)
        {
            close();
            return false;
        }
    }
    state = State::Connecting;
#else
    if (scheme == Scheme::Https)
    {
        if (!socket.connectSSL(io::network::Address(hostname), port))
            return false;
    }
    else
    {
        if (!socket.connect(io::network::Address(hostname), port))
            return false;
    }

    for(int n=0;n<16;n++)
        websock_key += char(irandom(0, 255));
    websock_key = stringutil::base64::encode(websock_key);
    string request = "GET " + path + " HTTP/1.1\r\n"
        "Host: " + hostname + "\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "Sec-Websocket-Version: 13\r\n"
        "Sec-Websocket-Key: " + websock_key + "\r\n"
        "Sec-WebSocket-Protocol: chat\r\n"
        "Pragma: no-cache\r\n";
    for(auto& it : headers)
        request += it.first + ": " + it.second + "\r\n";
    request +=
        "Cache-Control: no-cache, no-store, must-revalidate\r\n"
        "\r\n";
    socket.send(request.data(), request.length());
    state = State::Connecting;
    socket.setBlocking(false);
#endif
    return true;
}

void Websocket::close()
{
#ifdef EMSCRIPTEN
    if (socket_handle != -1)
    {
        ::close(socket_handle);
        socket_handle = -1;
    }
#else
    socket.close();
    buffer.clear();
    received_fragment.clear();
#endif
    state = State::Disconnected;
}

bool Websocket::isConnected()
{
    updateReceiveBuffer();
    return state == State::Operational;
}

bool Websocket::isConnecting()
{
    updateReceiveBuffer();
    return state == State::Connecting;
}

void Websocket::send(const io::DataBuffer& data_buffer)
{
    if (state != State::Operational)
        return;

#ifdef EMSCRIPTEN
    ::send(socket_handle, data_buffer.getData(), data_buffer.getDataSize(), 0);
#else
    if (data_buffer.getDataSize() < websocket::payload_length_16bit)
    {
        uint8_t header[] = {
            websocket::fin_mask | websocket::opcode_text,
            uint8_t(data_buffer.getDataSize()),
        };
        socket.send(header, sizeof(header));
    }
    else if (data_buffer.getDataSize() < (1 << 16))
    {
        uint8_t header[] = {
            websocket::fin_mask | websocket::opcode_text,
            websocket::payload_length_16bit,
            uint8_t((data_buffer.getDataSize() >> 8) & 0xFF),
            uint8_t(data_buffer.getDataSize() & 0xFF),
        };
        socket.send(header, sizeof(header));
    }
    else
    {
        uint8_t header[] = {
            websocket::fin_mask | websocket::opcode_text,
            websocket::payload_length_64bit,
            0, 0, 0, 0,
            uint8_t((data_buffer.getDataSize() >> 24) & 0xFF),
            uint8_t((data_buffer.getDataSize() >> 16) & 0xFF),
            uint8_t((data_buffer.getDataSize() >> 8) & 0xFF),
            uint8_t(data_buffer.getDataSize() & 0xFF),
        };
        socket.send(header, sizeof(header));
    }

    socket.send(data_buffer.getData(), data_buffer.getDataSize());
#endif
}

bool Websocket::receive(io::DataBuffer& data_buffer)
{
    updateReceiveBuffer();

    if (state == State::Operational)
    {
#ifdef EMSCRIPTEN
        char buffer[4096];
        size_t buffer_size = recv(socket_handle, buffer, sizeof(buffer), 0);
        if (buffer_size < 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
                close();
            return false;
        }
        if (buffer_size > 0)
        {
            data_buffer.appendRaw(buffer, buffer_size);
            return true;
        }
#else
        while(true)
        {
            if (buffer.size() < 2)
                return false;
            unsigned int payload_length = buffer[1] & websocket::payload_length_mask;
            int opcode = buffer[0] & websocket::opcode_mask;
            bool fin = buffer[0] & websocket::fin_mask;
            bool mask = buffer[1] & websocket::mask_mask;
            unsigned int index = 2;

            //Close the connection if any of the RSV bits are set.
            if (buffer[0] & websocket::rsv_mask)
            {
                LOG(Warning, "Closing client websocket due to RSV bits, we do not support extensions.");
                close();
                return false;
            }

            if (payload_length == websocket::payload_length_16bit)
            {
                if (buffer.size() < index + 2)
                    return false;
                payload_length = uint8_t(buffer[index++]) << 8;
                payload_length |= uint8_t(buffer[index++]);
            }else if (payload_length == websocket::payload_length_64bit)
            {
                if (buffer.size() < index + 8)
                    return false;
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
                    return false;
                for(unsigned int n=0; n<4; n++)
                    mask_values[n] = buffer[index++];
                if (buffer.size() < index + payload_length)
                    return false;
                for(unsigned int n=0; n<payload_length; n++)
                    buffer[index + n] ^= mask_values[n % 4];
            }
            if (buffer.size() < index + payload_length)
                return false;

            std::vector<uint8_t> message(buffer.begin() + index, buffer.begin() + index + payload_length);
            buffer.erase(buffer.begin(), buffer.begin() + index + payload_length);

            switch(opcode)
            {
            case websocket::opcode_continuation:
                received_fragment.insert(received_fragment.end(), message.begin(), message.end());
                if (fin)
                {
                    data_buffer = std::move(received_fragment);
                    received_fragment.clear();
                    return true;
                }
                break;
            case websocket::opcode_text:
            case websocket::opcode_binary:
                if (fin)
                {
                    data_buffer = std::move(message);
                    return true;
                }
                else
                {
                    received_fragment = message;
                }
                break;
            case websocket::opcode_close:
                {
                    uint8_t reply[] = {websocket::fin_mask | websocket::opcode_close, 0};//close packet
                    socket.send(reply, sizeof(reply));
                }
                close();
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
#endif
    }
    return false;
}

void Websocket::send(const string& message)
{
    io::DataBuffer data_buffer;
    data_buffer.appendRaw(message.data(), message.size());
    send(data_buffer);
}

bool Websocket::receive(string& output)
{
    io::DataBuffer data_buffer;
    if (!receive(data_buffer))
        return false;
    output = string(reinterpret_cast<const char*>(data_buffer.getData()), data_buffer.getDataSize());
    return true;
}

void Websocket::updateReceiveBuffer()
{
#ifdef EMSCRIPTEN
    if (state == State::Connecting)
    {
        struct pollfd pfd[1];
        pfd[0].fd = socket_handle;
        pfd[0].events = POLLIN | POLLOUT;
        pfd[0].revents = 0;
        if (poll(pfd, 1, 0))
        {
            if (pfd[0].revents & POLLOUT)
                state = State::Operational;
            else if (pfd[0].revents & POLLIN)
                close();
        }
    }
#else
    char receive_buffer[4096];
    size_t received_size;
    received_size = socket.receive(receive_buffer, sizeof(receive_buffer));
    if (received_size > 0)
    {
        buffer.resize(buffer.size() + received_size);
        memcpy(&buffer[buffer.size()] - received_size, receive_buffer, received_size);
    }
    if (!socket.isConnected())
        state = State::Disconnected;

    if (state == State::Connecting)
    {
        string buffer_str = string(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        int headers_end = buffer_str.find("\r\n\r\n");
        if (headers_end > -1)
        {
            std::vector<string> header_data = buffer_str.substr(0, headers_end).split("\r\n");
            buffer.erase(buffer.begin(), buffer.begin() + headers_end + 4);
            std::vector<string> parts = header_data[0].split(" ", 2);
            if (parts.size() != 3)
            {
                LOG(Warning, "Connecting to websocket failed, incorrect reply on HTTP upgrade request");
                close();
                return;
            }
            if (parts[1] != "101")
            {
                LOG(Warning, "Connecting to websocket failed, incorrect reply on HTTP upgrade request");
                close();
                return;
            }

            std::unordered_map<string, string> headers;
            for(unsigned int n=1; n<header_data.size(); n++)
            {
                auto header_parts = header_data[n].partition(":");
                headers[header_parts.first.strip().lower()] = header_parts.second.strip();
            }
            if (headers.find("upgrade") == headers.end() || headers.find("connection") == headers.end() || headers.find("sec-websocket-accept") == headers.end())
            {
                LOG(Warning, "Connecting to websocket failed, incorrect reply on HTTP upgrade request");
                close();
                return;
            }
            if (headers["upgrade"].lower() != "websocket" || headers["connection"].lower() != "upgrade")
            {
                LOG(Warning, "Connecting to websocket failed, incorrect reply on HTTP upgrade request");
                close();
                return;
            }
            if (headers["sec-websocket-accept"] != stringutil::SHA1(websock_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").base64())
            {
                LOG(Warning, "Connecting to websocket failed, incorrect reply on HTTP upgrade request");
                close();
                return;
            }
            state = State::Operational;
        }
    }
#endif
}

}//namespace http
}//namespace io
}//namespace sp
