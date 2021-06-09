#ifndef SP2_IO_NETWORK_ADDRESS_H
#define SP2_IO_NETWORK_ADDRESS_H

#include <stringImproved.h>
#include <SFML/System/NonCopyable.hpp>
#include <list>


namespace sp {
namespace io {
namespace network {


class Address
{
public:
    Address();
    Address(const string& hostname);

    std::vector<string> getHumanReadable() const;

    bool operator==(const Address& other) const;

    static Address getLocalAddress();
private:
    static void initSocketLib();

    class AddrInfo
    {
    public:
        AddrInfo(int family, const string& human_readable, const void* addr, size_t addrlen);
        ~AddrInfo();
    
        int family; //One of the AF_* macros, currently only AF_INET or AF_INET6
        string human_readable;
        std::string addr;    //We abuse the std::string as a data buffer, it allows for easy memory management.
    };

    Address(std::list<AddrInfo>&& addr_info);
    
    std::list<AddrInfo> addr_info;
    
    friend class TcpSocket;
    friend class UdpSocket;
    friend class TcpListener;
};

}//namespace network
}//namespace io
}//namespace sp

#endif//SP2_IO_NETWORK_TCP_SOCKET_H
