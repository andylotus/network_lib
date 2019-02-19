#include "UdpServer.h"
#include <fcntl.h>
#include "Sockets.h"

namespace lotus{
namespace net{

namespace udp {

    UdpServer::UdpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg)
       :loop_(loop),
        hostport_(listenAddr.toIpPort()),
        addr_(listenAddr),
        name_(nameArg),
        running_(false),
        threadPool_(new EventLoopThreadPool(loop))
	{
	}
    
    UdpServer:: ~UdpServer() { 
        if(running_)
            stop();
        LOG_TRACE << "UdpServer dtor";
        delete channel_ ; 
	}    
    

    void UdpServer::setThreadNum(int numThreads)
    {
      assert(numThreads >= 0);
      threadPool_->setThreadNum(numThreads);
    }    
    
    
	void UdpServer::start()
	{
		if(!running_)
		{       
			running_ = true;
			threadPool_->start(threadInitCallback_);
		}
		int sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (sockfd< 0) {
		LOG_ERROR << "socket creation error";
		}

		sockets::setNonBlockAndCloseOnExec(sockfd);

		sockets::bind(sockfd, this->addr_);

		channel_= new Channel(loop_, sockfd);
		channel_->setReadCallback(std::bind(&UdpServer::handleRead, this, std::placeholders::_1));
		channel_->enableReading();

	}

	void UdpServer::stop(){
				running_ = false;
				channel_->disableAll();
				channel_->remove();
				::close(channel_->fd());
				LOG_TRACE << "UdpServer stop";
	}
		
		
	void UdpServer::handleRead(Timestamp receiveTime){
		int fd = channel_->fd();
		int BUFFSIZE = 1024;
		char buf[BUFFSIZE];
		memset(buf, 0, BUFFSIZE);
			//bzero(buf, BUFSIZE);
		struct sockaddr_in clientaddr;
		socklen_t clientlen = sizeof(clientaddr);
		ssize_t rn = recvfrom(fd, buf, BUFFSIZE, 0,
			 reinterpret_cast<struct sockaddr *>( &clientaddr), &clientlen);
		if(rn < 0)
			LOG_ERROR << "recvfrom " << strerror(errno) ;
		
		loop_->assertInLoopThread();
		EventLoop *ioLoop = threadPool_->getNextLoop();

		ioLoop->runInLoop(std::bind(msgcb_, this, std::string(buf), InetAddress(clientaddr))); //multi-loop for onMessage, similar to Sudoku case
	  

	}

	void UdpServer::sendTo(const char* buf, size_t len, InetAddress addr){
		loop_->queueInLoop(std::bind(&UdpServer::sendToInLoop, this, buf, len, addr));
	}
		
	void UdpServer::sendToInLoop(const char* buf, size_t len, InetAddress addr) {
		loop_->assertInLoopThread();
		if (!channel_ || channel_->fd() < 0) {
			LOG_FATAL << "channel_ not created successfully";   
		}
		int fd = channel_->fd();
		ssize_t wn = ::sendto(fd, buf, len, 0, reinterpret_cast<struct sockaddr *>(&addr), sizeof(sockaddr));
		if (wn < 0) {
			//error("udp %d sendto %s error: %d %s", fd, addr.toString().c_str(), errno, strerror(errno));
			//return;
			LOG_ERROR << "udp sendTo failed";
		}
		LOG_TRACE<<"udp "<< fd << " sendto " << addr.toIpPort().c_str() <<" "<< wn << " bytes";
	}

}
}
}