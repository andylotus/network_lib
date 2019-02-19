#pragma once

#include <functional>
#include "Sockets.h"
#include "InetAddress.h"
#include "Channel.h"
#include "noncopyable.h"

namespace lotus
{
namespace net
{
    class TcpConnection;

    class EventLoop;

        class Acceptor : public noncopyable
        {
        public:
            using NewConnectionCallback = std::function<void(int, const InetAddress &)>;

            Acceptor(EventLoop *loop, const InetAddress &listen_addr) noexcept;

            ~Acceptor()noexcept;

            void setNewConnectionCallback(const NewConnectionCallback &cb);

            void listen();
			bool listening() const;

        private:
            void handleRead();

        private:
            EventLoop *loop_;
            int fd_; //listenfd
            InetAddress listenAddr_;
            
            Channel acceptChannel_;
            NewConnectionCallback newConnectionCallback_;
			bool listening_;
            int idleFd_;
        };

}
}
