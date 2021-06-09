#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <io/network/selector.h>
#include <algorithm>
#include <vector>

#ifdef __WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <errno.h>
#include <poll.h>
#endif


namespace sp {
namespace io {
namespace network {


class Selector::SelectorData
{
public:
    std::vector<struct pollfd> fds;
};

Selector::Selector()
: data(new SelectorData())
{
}

Selector::Selector(const Selector& other)
: data(new SelectorData(*other.data))
{
}

Selector::~Selector()
{
    delete data;
}

Selector& Selector::operator =(const Selector& other)
{
    *data = *other.data;
    return *this;
}

void Selector::add(SocketBase& socket)
{
    if (socket.handle != -1)
    {
        struct pollfd fds;
        fds.fd = socket.handle;
        fds.events = POLLIN;
        fds.revents = 0;
        data->fds.push_back(fds);
    }
}

void Selector::remove(SocketBase& socket)
{
    if (socket.handle != -1)
    {
        data->fds.erase(std::remove_if(data->fds.begin(), data->fds.end(), [&socket](const struct pollfd& pfd)
        {
            return int(pfd.fd) == socket.handle;
        }), data->fds.end());
    }
}

void Selector::wait(int timeout_ms)
{
#ifdef __WIN32
    WSAPoll(data->fds.data(), data->fds.size(), timeout_ms);
#else
    poll(data->fds.data(), data->fds.size(), timeout_ms);
#endif
}

bool Selector::isReady(SocketBase& socket)
{
    for(const auto& pfd : data->fds)
    {
        if (int(pfd.fd) == socket.handle)
            return pfd.revents & POLLIN;
    }
    return false;
}

}//namespace network
}//namespace io
}//namespace sp
