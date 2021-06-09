#include <io/network/tcpListener.h>
#include <io/network/tcpSocket.h>

#ifdef __WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#endif


namespace sp {
namespace io {
namespace network {


TcpListener::TcpListener()
{
}

TcpListener::~TcpListener()
{
    close();
}

bool TcpListener::listen(int port)
{
    Address::initSocketLib();
    if (isListening())
        close();
    
    handle = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (handle != -1)
    {
        int optval = 1;
        ::setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(int));
        optval = 0;
        ::setsockopt(handle, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&optval), sizeof(int));

        struct sockaddr_in6 server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin6_family = AF_INET6;
        server_addr.sin6_addr = in6addr_any;
        server_addr.sin6_port = htons(port);

        if (::bind(handle, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
        {
            close();
            return false;
        }
    }
    else
    {
        handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (handle == -1)
            return false;

        int optval = 1;
        ::setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(int));

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(port);

        if (::bind(handle, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
        {
            close();
            return false;
        }
    }

    if (::listen(handle, 8) < 0)
    {
        close();
        return false;
    }
    return true;
}

void TcpListener::close()
{
    if (isListening())
    {
#ifdef __WIN32
        closesocket(handle);
#else
        ::close(handle);
#endif
        handle = -1;
    }
}

bool TcpListener::isListening()
{
    return handle != -1;
}

bool TcpListener::accept(TcpSocket& socket)
{
    if (!isListening())
        return false;
    
    int result = ::accept(handle, nullptr, nullptr);
    if (result < 0)
    {
        if (!isLastErrorNonBlocking())
            close();
        return false;
    }
    if (socket.isConnected())
        socket.close();
    socket.handle = result;
    socket.setBlocking(socket.blocking);
    return true;
}

}//namespace network
}//namespace io
}//namespace sp
