#include <cstring>
#include <cassert>
#include "Acceptor.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "Log.h"
#include <errno.h>
#include <fcntl.h>

namespace lotus
{
namespace net
{
   Acceptor::Acceptor(EventLoop *loop, const InetAddress &listen_addr)noexcept
                : loop_(loop)
                , fd_(sockets::createNonblockingSocket())
                , listenAddr_(listen_addr)
                , acceptChannel_(loop, fd_)
				, listening_(false)
                ,idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
        {
           sockets::setReuseAddr(fd_, true);
           acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
        }

        Acceptor::~Acceptor()noexcept
        {
          acceptChannel_.disableAll();
          acceptChannel_.remove();
          ::close(idleFd_);
        }

        void Acceptor::listen()
        {
            loop_->assertInLoopThread();
            assert(fd_ > 0);
			listening_ = true;
            sockets::bind(fd_, listenAddr_);
            sockets::listen(fd_);

            acceptChannel_.enableReading();

        }
	
		bool Acceptor::listening() const
		{
			return listening_;
		}

        void Acceptor::handleRead()
        {
            loop_->assertInLoopThread();

            InetAddress peerAddr;
            int connfd = sockets::accept(fd_, peerAddr);
            if (connfd >= 0)
            {
                if (newConnectionCallback_) //this cb will be set up in TcpServer.cc
                {
                  newConnectionCallback_(connfd, peerAddr);
                }
                else
                {
                  sockets::close(connfd);
                }
             }
              else
              {
                if (errno == EMFILE) //too many fds, so make connfd < 0 (unsuccessful)
                {
                  ::close(idleFd_);
                  idleFd_ = ::accept(fd_, NULL, NULL);
                  ::close(idleFd_);
                  idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
                }
              }
        }

        void Acceptor::setNewConnectionCallback(const Acceptor::NewConnectionCallback &cb)
        {
            newConnectionCallback_ = cb;
        }
}
}
