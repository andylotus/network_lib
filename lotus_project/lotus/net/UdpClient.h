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


namespace lotus{
namespace net{
namespace udp{
	class UdpClient;
	typedef std::function<void( UdpClient*, std::string)> UdpClientCallBack;   

    class UdpClient: public noncopyable, public std::enable_shared_from_this<UdpClient> {
	public:
        UdpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& name);
        ~UdpClient(); 

		void stop();
		void connect();

        EventLoop* getLoop() { return loop_; }
        Channel* getChannel() { return channel_; }
		const std::string& name() const { return name_; }
		
        void send(const char* buf, size_t len);
        void send(const std::string& s) { send(s.data(), s.size()); }
        void send(const char* s) { send(s, strlen(s)); }
        void setMessageCallback(const UdpClientCallBack& cb) { msgcb_ = cb; }

    private:
        void handleRead(Timestamp receiveTime);
        EventLoop* loop_;
        Channel* channel_;
		const std::string name_;
        InetAddress  peer_; //local_,
		bool connected_; //if true, udp client is using connect()
        UdpClientCallBack msgcb_;
    };
}
}
}