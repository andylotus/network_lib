#ifndef LOTUS_NET_INETADDRESS_H
#define LOTUS_NET_INETADDRESS_H

#include <string>
#include <netinet/in.h>
#include "copyable.h"


//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

namespace lotus
{
namespace net
{   
    //Wrapper of sockaddr_in, for IPv4 only
    
    class InetAddress final
    {
    public:
        InetAddress() noexcept;

        InetAddress(const std::string &ip, uint16_t port);

        explicit InetAddress(const struct sockaddr_in &addr);

        //explicit InetAddress(const struct sockaddr_in6 &addr);

        /// addr_any true: use 'any' address
        /// false： use Loop back address---127.0.0.1
        explicit InetAddress(uint16_t port,bool addr_any = true);

        const sockaddr *toSockaddr() const;

        sockaddr *toSockaddr();

        sa_family_t family() const;

        in_port_t port() const; //in_port_t: uint16_t 

        uint32_t ip() const;

        bool operator==(const InetAddress &addr) const noexcept ;

        static constexpr socklen_t size()
        {
            return static_cast<socklen_t>(sizeof(sockaddr_in));
        }


        std::string toIp() const; //dotted-decimal IP

        std::string toIpPort() const;

        in_port_t toPort() const;
		
		const struct sockaddr_in& getSockAddrInet() const { return addr_; }
		void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }

    private:
        struct sockaddr_in addr_;

    };
}
}

#endif