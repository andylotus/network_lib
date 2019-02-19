#ifndef LOTUS_NET_CALLBACKS_H
#define LOTUS_NET_CALLBACKS_H

#include <memory>
#include <functional>
#include <chrono>

// All client visible callbacks go here.

namespace lotus
{
namespace net
{
	class TcpConnection;

    class Buffer;

    class InetAddress;
    
    using EventCallback = std::function<void()>;
	typedef std::function<void()> TimerCallback;
	
	
	using Timestamp =std::chrono::time_point<std::chrono::system_clock>;

	typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
	typedef std::function<void (const TcpConnectionPtr&,
                              Buffer*,
                              Timestamp)> MessageCallback;

    using ConnectionCallback =std::function<void(const TcpConnectionPtr &)>;

    using CloseCallback = std::function<void(const TcpConnectionPtr &)>;

    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;

    using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)> ;
    //defined in TcpConnection
    void defaultConnectionCallback(const TcpConnectionPtr& conn);
    void defaultMessageCallback(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp receiveTime);
}
    
}

#endif //LOTUS_NET_CALLBACKS_H
