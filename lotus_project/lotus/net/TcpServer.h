#pragma once

#include <string>
#include <map>
#include <vector>
#include "Callbacks.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include "noncopyable.h"

namespace lotus
{
namespace net
{
    class EventLoop;

    class TcpServer : public noncopyable
    {
    public:
        typedef std::function<void(EventLoop*)> ThreadInitCallback;
        /// threadSize 为0，意为着server为单线程模式，所有操作均在loop线程
        /// threadSize 为1，意味着IO操作在新线程，listen在loop线程
        /// threadSize 为N，意味着会有一个线程数为N的线程池，IO操作均在线程池内处理
        TcpServer(EventLoop *loop, const InetAddress &addr, const std::string &name = "Server"/* size_t threadSize = 0 */);

        ~TcpServer()noexcept;

        /// 线程安全
        /// 开始server。开始监听，处理accept
        void start();
        
        void setThreadInitCallback(const ThreadInitCallback& cb)
        { threadInitCallback_ = cb; }

        /// set up the following Calls at the User-Server
        void setConnectionCallback(const ConnectionCallback &cb);
        void setMessageCallback(const MessageCallback &cb);
        
		void setWriteCompleteCallback(const WriteCompleteCallback& cb)
		{ writeCompleteCallback_ = cb; }
       
        void setThreadNum(int numThreads);

        const std::string& name() const;

     private:
      /// Not thread safe, but in loop
        void newConnection(int connfd, const InetAddress& peerAddr);
        void removeConnection(const TcpConnectionPtr& conn);
        void removeConnectionInLoop(const TcpConnectionPtr& conn);

    private:
        EventLoop *loop_; //the acceptor loop, may not be the loop of connection
        InetAddress servAddr_;
        std::string servName_;
        std::unique_ptr<Acceptor> acceptor_;
        std::unique_ptr<EventLoopThreadPool> threadPool_;
        bool started_;
        int nextConnId_;
        std::map<std::string, TcpConnectionPtr> connections_;

        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        ThreadInitCallback threadInitCallback_;
		WriteCompleteCallback writeCompleteCallback_;	
    };

}
}
