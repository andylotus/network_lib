#pragma once

#include "EventLoop.h"
#include "noncopyable.h"
#include "Channel.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include <string>
#include <cstring>
#include <memory>
#include "Log.h"
#include "EventLoopThreadPool.h"

namespace lotus{
namespace net{
namespace udp{
	class UdpServer;
	typedef std::function<void( UdpServer*, std::string, InetAddress)> UdpSvrCallBack;
	
	class UdpServer: public noncopyable //, public std::enable_shared_from_this<UdpServer>
	{
	public:
		typedef std::function<void(EventLoop*)> ThreadInitCallback;
			
		UdpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg);
		~UdpServer();
		
		const std::string& hostport() const { return hostport_; }
		const std::string& name() const { return name_; }
		void start();
		void stop();

		void setThreadNum(int numThreads);
		void setThreadInitCallback(const ThreadInitCallback& cb)
		{ threadInitCallback_ = cb; }

		EventLoop* getLoop() { return loop_; }

		void sendTo(const char* buf, size_t len, InetAddress addr);
		void sendTo(const std::string& s, InetAddress addr)  { sendTo(s.data(), s.size(), addr); }
		void sendTo(const char* s, InetAddress addr) { sendTo(s, strlen(s), addr); }
		void sendToInLoop(const char* buf, size_t len, InetAddress addr);

		//messageCallback: onMessage
		void setMessageCallback(const UdpSvrCallBack& cb) { msgcb_ = cb; }


	private:
		void handleRead(Timestamp receiveTime);
		EventLoop* loop_;
		const std::string hostport_;		// 服务端口
		const std::string name_;			// 服务名
		InetAddress addr_; //server address
		Channel* channel_;
		bool running_;
		std::unique_ptr<EventLoopThreadPool> threadPool_;
		ThreadInitCallback threadInitCallback_;
		UdpSvrCallBack msgcb_;
	};

}
}
}
