#include "UdpClient.h"
#include "Sockets.h"

namespace lotus{
namespace net{

namespace udp {
	UdpClient::UdpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& name)
		:loop_(loop),
		peer_(serverAddr),
		name_(name),
		connected_(false)
	{
		int sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (sockfd< 0) {
			LOG_ERROR << "socket creation error";
		}
		sockets::setNonBlockAndCloseOnExec(sockfd);
		channel_= new Channel(loop_, sockfd);
		channel_->setReadCallback(std::bind(&UdpClient::handleRead, this, std::placeholders::_1));
		channel_->enableReading();		
	}

	UdpClient::~UdpClient(){ 
		stop();
		LOG_TRACE << "UdpServer dtor";
		delete channel_ ; 
	}    
		
	void UdpClient::stop()
	{
		connected_ = false;
		channel_->disableAll();
		channel_->remove();
		::close(channel_->fd());
		LOG_TRACE << "UdpServer stop";
	}

	void UdpClient::connect(){
		sockets::connect(channel_->fd(), peer_);
		connected_ = true;
	}
		
	void UdpClient::handleRead(Timestamp receiveTime){
		int fd = channel_->fd();
		int BUFFSIZE = 1024;
		char buf[BUFFSIZE];
		memset(buf, 0, BUFFSIZE);
		
		if(connected_){
			ssize_t rn = ::read(fd, buf, BUFFSIZE);
			if(rn <0)
				LOG_ERROR << "UdpClient read from " << fd << "with error " << strerror(errno);
		}
		
		else{
		struct sockaddr_in servaddr;
		socklen_t servlen = sizeof(servaddr);
		ssize_t rn = recvfrom(fd, buf, BUFFSIZE, 0,
			 reinterpret_cast<struct sockaddr *> (&servaddr), &servlen);
		if(rn < 0)
			LOG_ERROR << "recvfrom " << strerror(errno) ;
		
		}
		
		std::string str(buf);
		//LOG_TRACE << "before msgcb";
		msgcb_(this, str);    
		//LOG_TRACE << "after msgcb";
	}

	
	void UdpClient::send(const char *buf, size_t len) {
    if (!channel_ || channel_->fd() < 0) {
        LOG_WARN << "udp sending "<< len <<  "bytes to " << peer_.toIpPort() << "after channel closed";
    }
    int fd = channel_->fd();
	ssize_t wn = 0;
	if(connected_){
		wn = ::write(fd, buf, len);
		if (wn < 0) 
			LOG_FATAL <<"udp " << fd << "write error "<< strerror(errno);
    }
	
	else{
		wn = ::sendto(fd, buf, len, 0, reinterpret_cast<struct sockaddr *>(&peer_), sizeof(peer_));
		if (wn < 0) {
			LOG_ERROR << "udp sendTo failed";
		}		
	}
	
    LOG_TRACE<<"udp "<< fd << " sendto " << peer_.toIpPort().c_str() << wn << "bytes";
   }
    
}
}
}
