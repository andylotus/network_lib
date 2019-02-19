#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include "Log.h"
#include "Sockets.h"
#include "InetAddress.h"

namespace lotus
{
namespace net
{
    namespace sockets
    {
        int createNonblockingSocket()
        {
            int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);

            if (sockfd < 0) {
                LOG_FATAL << "failed";
            }
            return sockfd;
        }

        void setNonBlockAndCloseOnExec(int sockfd)
        {
            int flags = ::fcntl(sockfd, F_GETFL, 0);
            flags |= O_NONBLOCK | FD_CLOEXEC;
            ::fcntl(sockfd, F_SETFL, flags);
        }
        
        void bind(int sockfd, const InetAddress &addr)
        {
            if (::bind(sockfd, addr.toSockaddr(), InetAddress::size()) < 0) 
            {
                LOG_ERROR << "bind" << errno;
            }
        }

        void listen(int sockfd)
        {
            if (::listen(sockfd, SOMAXCONN) < 0) {
                LOG_ERROR << "listen";
            }
        }

        int connect(int sockfd, const InetAddress &peeraddr)
        {
            return ::connect(sockfd, peeraddr.toSockaddr(), InetAddress::size());
        }

        void close(int sockfd)
        {
            if(::close(sockfd)<0) {
                LOG_ERROR << "fd= " << sockfd << "close";
            }
        }

        int accept(int sockfd, InetAddress &peeraddr)
        {
            socklen_t addrlen = InetAddress::size();

            int connfd = ::accept4(sockfd, peeraddr.toSockaddr(),
                                   &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (connfd < 0) {

                int savedErrno = errno;
                LOG_ERROR << "sockets::accept";
                switch (savedErrno) {
                    case EAGAIN:
                    case ECONNABORTED:
                    case EINTR:
                    case EPROTO:
                    case EPERM:
                    case EMFILE:
                        // expected errors
                        errno = savedErrno;
                        break;
                    case EBADF:
                    case EFAULT:
                    case EINVAL:
                    case ENFILE:
                    case ENOBUFS:
                    case ENOMEM:
                    case ENOTSOCK:
                    case EOPNOTSUPP:
                        // unexpected errors
                        LOG_FATAL<< "unexpected error of ::accept " << savedErrno;
                        break;
                    default:
                        LOG_FATAL << "unknown error of ::accept " << savedErrno;
                        break;
                }
            }

            return connfd;
        }

        void shutdownWrite(int fd)
        {
            if (::shutdown(fd, SHUT_WR) < 0) 
            {
                LOG_ERROR << "failed";
            }
        }



        int getSocketError(int sockfd)
        {
            int optval;
            socklen_t optlen = sizeof optval;

            if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) 
            {
                return errno;
            }
            else 
            {
                return optval;
            }
        }

        struct sockaddr_in getPeerAddr(int fd)
        {
            struct sockaddr_in peeraddr{};
            socklen_t addrlen = sizeof peeraddr;
            if (::getpeername(fd, reinterpret_cast< sockaddr *>(&peeraddr), &addrlen) < 0) 
            {
                LOG_ERROR << "sockets::getPeerAddr";
            }
            return peeraddr;
        }

        struct sockaddr_in getLocalAddr(int fd)
        {
            struct sockaddr_in localaddr{};
            socklen_t addrlen = sizeof localaddr;
            if (::getsockname(fd, reinterpret_cast< sockaddr *>(&localaddr), &addrlen) < 0) 
            {
                LOG_ERROR << "sockets::getLocalAddr";
            }
            return localaddr;
        }
        
        void setKeepAlive(int fd, bool on)
        {
            int optval = on ? 1 : 0;
            int rc = ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
                                  reinterpret_cast<const char *>(&optval), static_cast<socklen_t>(sizeof optval));
            if (rc != 0) 
            {
                int serrno = errno;
                LOG_ERROR << "setsockopt(SO_KEEPALIVE) failed, errno=" << serrno << " " << strerror(serrno);
            }
        }

         void setTcpNoDelay(int fd, bool on)
        {
            int optval = on ? 1 : 0;
            int rc = ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                                  &optval, static_cast<socklen_t>(sizeof optval));
            if (rc != 0) 
            {
                int serrno = errno;
                LOG_ERROR << "setsockopt(TCP_NODELAY) failed, errno=" << serrno << " " << strerror(serrno);
            }
        }
        
        void setReuseAddr(int fd, bool on)
        {
          int optval = on ? 1 : 0;
          int rc = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                       &optval, sizeof optval);
            if (rc != 0) 
            {
                LOG_ERROR << "setsockopt(SO_REUSEADDR) failed:" ;
            }
        }

        // 自连接是指(sourceIP, sourcePort) = (destIP, destPort)
        // 自连接发生 --- must meet the three conditions simultaneously!
        // 1) 客户端与服务器端在同一台机器，即sourceIP = destIP，
        // 2) sourcePort = destPort
        // 3) 服务器尚未开启，即服务器还没有在destPort端口上处于监听. Meanwhile, 客户端发起connect
        // 就有可能出现自连接，这样，服务器也无法启动了

        bool isSelfConnect(int sockfd)
        {
          struct sockaddr_in localaddr = getLocalAddr(sockfd);
          struct sockaddr_in peeraddr = getPeerAddr(sockfd);
          return localaddr.sin_port == peeraddr.sin_port
              && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
        }
    }
}
    
}
