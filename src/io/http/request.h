#ifndef SP2_IO_HTTP_REQUEST_H
#define SP2_IO_HTTP_REQUEST_H

#include <io/network/tcpSocket.h>
#include <unordered_map>

namespace sp {
namespace io {
namespace http {

class Request : sp::NonCopyable
{
public:
    class Response
    {
    public:
        bool success = false;
        int status = -1;
        std::unordered_map<string, string> headers;
        string body;
    };

    enum class Scheme
    {
        Auto,
        Http,
        Https
    };

    Request(const string& hostname, int port=80, Scheme scheme=Scheme::Auto);

    void setHeader(const string& key, const string& value);

    Response get(const string& path);
    Response post(const string& path, const string& data);

    Response request(const string& method, const string& path, const string& data);
private:
    int port;
    Scheme scheme;
    std::unordered_map<string, string> headers;
    sp::io::network::TcpSocket socket;
};

}//namespace http
}//namespace io
}//namespace sp

#endif//SP2_IO_HTTP_WEBSOCKET_H
