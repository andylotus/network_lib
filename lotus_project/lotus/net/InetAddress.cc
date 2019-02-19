#include <arpa/inet.h>
#include <cstring>
#include <strings.h>
#include <cassert>
#include <cstddef>
#include "Log.h"
#include "InetAddress.h"


namespace lotus
{
namespace net 
{
    InetAddress::InetAddress() noexcept
            
    {
        static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in), "InetAddress is same size as sockaddr_in6");
        static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
        static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
        static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");
    }

    InetAddress::InetAddress(const std::string &ip, uint16_t port)
    {
        bzero(&addr_, sizeof(addr_));
        addr_.sin_port = htons(port);
        addr_.sin_family = AF_INET;
        if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) 
        {
          LOG_ERROR << "inet_pton failed";
        }

    }

    InetAddress::InetAddress(const struct sockaddr_in &addr)
        : addr_(addr) 
    {
        assert(addr.sin_family == AF_INET);
    }

    InetAddress::InetAddress(uint16_t port, bool addr_any)
    {
        bzero(&addr_, sizeof(addr_));  
        addr_.sin_port = htons(port);
        addr_.sin_family = AF_INET;
        addr_.sin_addr.s_addr = htonl(addr_any? INADDR_ANY:INADDR_LOOPBACK);

    }

    std::string InetAddress::toIp() const 
    {
        char buff[64] = {0};
        auto size = static_cast<socklen_t>(sizeof(buff));
        if (addr_.sin_family == AF_INET) 
        {
            static_assert(sizeof(buff) >= INET_ADDRSTRLEN, "");
            ::inet_ntop(AF_INET, &addr_.sin_addr, buff, size);
        } 
        return std::string(buff);
    }

    std::string InetAddress::toIpPort() const 
    {
        return toIp().append(":").append(std::to_string(toPort()));
    }

    in_port_t InetAddress::toPort() const 
    {
        return ntohs(addr_.sin_port);
    }

    const sockaddr* InetAddress::toSockaddr() const 
    {
        return reinterpret_cast<const sockaddr *>(&addr_);
    }

    sockaddr *InetAddress::toSockaddr() 
    {
        return reinterpret_cast<sockaddr *>(&addr_);
    }

    sa_family_t InetAddress::family() const 
    {
        return addr_.sin_family;
    }

    in_port_t InetAddress::port() const 
    {
        return addr_.sin_port;
    }

    uint32_t InetAddress::ip() const 
    {
        assert(family() == AF_INET);
        return addr_.sin_addr.s_addr;
    }

    bool InetAddress::operator==(const InetAddress &another) const noexcept 
    {
        assert(family() == AF_INET);
        if (port() == another.port() && family() == another.family()) 
            return ip() == another.ip();

        return false;
    }
}
}