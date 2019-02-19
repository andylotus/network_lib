#pragma once

#include <atomic>
#include <string>
#include "Callbacks.h"
#include "InetAddress.h"
#include "Buffer.h"
#include "Epoll.h"
#include "Channel.h"
#include "noncopyable.h"

namespace lotus
{
namespace net
{
    class EventLoop;

    /// 不会建立连接，只是用来管理连接
    class TcpConnection : public noncopyable, public std::enable_shared_from_this<TcpConnection>
    {
    public:

		TcpConnection(EventLoop *loop, const std::string& name, int sockfd, const InetAddress &local_addr
					  , const InetAddress &peer_addr);

		~TcpConnection()noexcept;
		
		//called at TcpServer
		void setConnectionCallback(const ConnectionCallback& cb)
		{ connectionCallback_ = cb; }

		void setMessageCallback(const MessageCallback& cb)
		{ messageCallback_ = cb; }

		void setCloseCallback(const CloseCallback& cb) //internal use only
		{ closeCallback_ = cb; }
	
		void setWriteCompleteCallback(const WriteCompleteCallback& cb)
		{ writeCompleteCallback_ = cb; }

		void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
		{ highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }
		
		// called when TcpServer has finished building a new connection
        void connectEstablished();   
        void connectDestroyed();

        EventLoop* getLoop() const { return loop_; }
        const std::string& name() const { return name_; }
        const InetAddress& localAddress() { return localAddr_; }
        const InetAddress& peerAddress() { return peerAddr_; }
        bool connected() const { return status_ == Connected; }

        void send(const char* message, size_t len);
        void send(const std::string& message);
        void send(Buffer* message);  // this one will swap data
        void shutdown(); // NOT thread safe, no simultaneous calling
        void setTcpNoDelay(bool on);

    private:
        void handleRead(Timestamp receiveTime);
        void handleClose();
        void handleError();
		void handleWrite();
        
        void sendStrInLoop(const std::string& message);
        void sendInLoop(const char* message, size_t len);
        void shutdownInLoop();
		
        enum Status
        {
            Disconnected = 0,
            Connecting = 1,
            Connected = 2,
            Disconnecting = 3,
        };
        void setStatus(Status s) { status_ = s;}
        
    private:
        
        EventLoop* loop_;
		std::string name_;
        int sockfd_; //connfd
        Status status_;
        std::unique_ptr<Channel> channel_;

        InetAddress localAddr_;
        InetAddress peerAddr_;        
        
        Buffer inputBuffer_;			// 应用层接收缓冲区
        Buffer outputBuffer_;
		size_t highWaterMark_;		// 高水位标

        //size_t _high_level_mark = 64 * 1024 * 1024; // Default 64MB
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        CloseCallback  closeCallback_;

 		WriteCompleteCallback writeCompleteCallback_;		// 数据发送完毕回调函数，即所有的用户数据都已拷贝到内核缓冲区时回调该函数
													// outputBuffer_被清空也会回调该函数，可以理解为低水位标回调函数
 		HighWaterMarkCallback highWaterMarkCallback_;	    // 高水位标回调函数
  
    };
}
}
