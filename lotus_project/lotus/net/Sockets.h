#ifndef LOTUS_NET_SOCKET_H
#define LOTUS_NET_SOCKET_H

#include <sys/socket.h>
#include <cerrno>
#include <netinet/in.h>

//Not a class

namespace lotus
{
namespace net
{
    class InetAddress;

    namespace sockets
    {
        int createNonblockingSocket();

        void bind(int sockfd, const InetAddress &localaddr);

        void listen(int sockfd);

        int connect(int sockfd, const InetAddress &localaddr);

        int accept(int sockfd, InetAddress &peeraddr);

        void close(int sockfd);

        int getSocketError(int sockfd);

        struct sockaddr_in getPeerAddr(int fd);

        struct sockaddr_in getLocalAddr(int fd);

        void shutdownWrite(int fd);
        
        void setReuseAddr(int fd, bool on);

        void setKeepAlive(int fd, bool on);

        //default delay 200ms
        void setTcpNoDelay(int fd, bool on = true); //disable(on=true)/enable Nagle's algorithm

        void setNonBlockAndCloseOnExec(int sockfd);
        
        bool isSelfConnect(int sockfd);

        
    };
}
}

#endif
