#include <io/network/address.h>
#include <logging.h>

#ifdef __WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#elif !defined(EMSCRIPTEN)
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <errno.h>
#endif

namespace sp {
namespace io {
namespace network {

Address::Address()
{
}

Address::Address(const string& hostname)
{
    initSocketLib();
#ifndef EMSCRIPTEN
    struct addrinfo* result;
    if (::getaddrinfo(hostname.c_str(), nullptr, nullptr, &result))
        return;

    for(struct addrinfo* data=result; data != nullptr; data=data->ai_next)
    {
        if (data->ai_family != AF_INET && data->ai_family != AF_INET6)
            continue;

        char buffer[128];
        ::getnameinfo(data->ai_addr, data->ai_addrlen, buffer, sizeof(buffer), nullptr, 0, NI_NUMERICHOST);

        addr_info.emplace_back(data->ai_family, buffer, data->ai_addr, data->ai_addrlen);
    }
    ::freeaddrinfo(result);
#endif
}

Address::Address(std::list<AddrInfo>&& addr_info)
: addr_info(std::forward<std::list<AddrInfo>>(addr_info))
{
}

std::vector<string> Address::getHumanReadable() const
{
    std::vector<string> result;
    for(const auto& info : addr_info)
        result.push_back(info.human_readable);
    return result;
}

Address Address::getLocalAddress()
{
    initSocketLib();

    std::list<AddrInfo> addr_info;

#ifdef __WIN32
    char buffer[128];

    PIP_ADAPTER_ADDRESSES addresses;
    ULONG table_size = 0;
    GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr, &table_size);
    if (table_size > 0)
    {
        addresses = static_cast<PIP_ADAPTER_ADDRESSES>(calloc(table_size, 1));
        if (GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, addresses, &table_size) == NO_ERROR)
        {
            for(PIP_ADAPTER_ADDRESSES address = addresses; address; address = address->Next)
            {
                if (address->OperStatus != IfOperStatusUp)
                    continue;

                for (PIP_ADAPTER_UNICAST_ADDRESS ua = address->FirstUnicastAddress; ua != nullptr; ua = ua->Next)
                {
                    ::getnameinfo(ua->Address.lpSockaddr, ua->Address.iSockaddrLength, buffer, sizeof(buffer), nullptr, 0, NI_NUMERICHOST);
                    if (ua->Address.iSockaddrLength == sizeof(sockaddr_in))
                        addr_info.emplace_back(AF_INET, buffer, ua->Address.lpSockaddr, ua->Address.iSockaddrLength);
                    if (ua->Address.iSockaddrLength == sizeof(sockaddr_in6))
                        addr_info.emplace_back(AF_INET6, buffer, ua->Address.lpSockaddr, ua->Address.iSockaddrLength);
                }
            }
        }
        free(addresses);
    }
#elif (!defined(__ANDROID_API__) || (__ANDROID_API__ >= 24)) && !defined(EMSCRIPTEN)
    char buffer[128];

    struct ifaddrs* addrs;
    if (!::getifaddrs(&addrs))
    {
        for(struct ifaddrs* addr = addrs; addr != nullptr; addr = addr->ifa_next)
        {
            if ((addr->ifa_flags & IFF_LOOPBACK) || !(addr->ifa_flags & IFF_UP))
            {
                continue;
            }
            if (addr->ifa_addr->sa_family == AF_INET)
            {
                ::getnameinfo(addr->ifa_addr, sizeof(struct sockaddr_in), buffer, sizeof(buffer), nullptr, 0, NI_NUMERICHOST);
                addr_info.emplace_back(addr->ifa_addr->sa_family, buffer, addr->ifa_addr, sizeof(struct sockaddr_in));
            }
            if (addr->ifa_addr->sa_family == AF_INET6)
            {
                ::getnameinfo(addr->ifa_addr, sizeof(struct sockaddr_in6), buffer, sizeof(buffer), nullptr, 0, NI_NUMERICHOST);
                addr_info.emplace_back(addr->ifa_addr->sa_family, buffer, addr->ifa_addr, sizeof(struct sockaddr_in6));
            }
        }
        freeifaddrs(addrs);
    }
#else
    LOG(Warning, "No method to get local IP address.");
#endif
    return Address(std::move(addr_info));
}

Address::AddrInfo::AddrInfo(int family, const string& human_readable, const void* addr, size_t addrlen)
: family(family), human_readable(human_readable), addr(static_cast<const char*>(addr), addrlen)
{
}

Address::AddrInfo::~AddrInfo()
{
}

void Address::initSocketLib()
{
#ifdef __WIN32
    static bool wsa_startup_done = false;
    if (!wsa_startup_done)
    {
        WSADATA wsa_data;
        memset(&wsa_data, 0, sizeof(WSADATA));
        WSAStartup(MAKEWORD(2, 2), &wsa_data);
        wsa_startup_done = true;
    }
#endif
}


}//namespace network
}//namespace io
}//namespace sp
