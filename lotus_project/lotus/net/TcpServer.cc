#include <unistd.h>
#include "TcpServer.h"
#include"Acceptor.h"
#include "Log.h"
#include "EventLoop.h"

namespace lotus
{
namespace net
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &name /*size_t threadSize */)
            : loop_(loop),
              servAddr_(listenAddr),
              servName_(name),
              acceptor_(new Acceptor(loop, listenAddr)),
              threadPool_(new EventLoopThreadPool(loop)),//here the loop arg is the mainReactor/baseloop
              connectionCallback_(defaultConnectionCallback),
              messageCallback_(defaultMessageCallback),
              started_(false),
              nextConnId_(1)
    {
        LOG_TRACE << "server:"<<servName_;
        // acceptor::handleRead--- newConnectionCallback _1对应的是connfd文件描述符，_2对应的是对等方的地址(peerAddr)
        acceptor_->setNewConnectionCallback(
          	std::bind(&TcpServer::newConnection, this, _1, _2));
    }

    TcpServer::~TcpServer() noexcept
    {
        LOG_TRACE << "TcpServer::~TcpServer [" << servName_ << "] destructing";
        //deal with remainder conn in connections_
        for(auto &connPair : connections_)
        {
            TcpConnectionPtr conn = connPair.second;
            conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
        }
    }

    void TcpServer::start()
    {
          if (!started_)
          {
            started_ = true;
            threadPool_->start(threadInitCallback_); //此函数中所有线程池中的子线程启动，且在其中开始运行子线程的loop.loop()
          }

          if (!acceptor_->listening())
          {
            // get() return the raw pointer of the smart pointer 返回原生指针
            loop_->runInLoop(
                std::bind(&Acceptor::listen, acceptor_.get()));
          }
    }

    void TcpServer::setThreadNum(int numThreads)
    {
      assert(numThreads >= 0);
      threadPool_->setThreadNum(numThreads);
    }

    void TcpServer::newConnection(int connfd, const InetAddress &peerAddr)
    {
          loop_->assertInLoopThread();
          EventLoop* ioLoop = threadPool_->getNextLoop();
          char buf[32];
          snprintf(buf, sizeof buf, ":%s#%d", servAddr_.toIpPort().c_str(), nextConnId_);
          ++nextConnId_;
        //example of connName: TestServer:0.0.0.0:8888#1
          std::string connName = servName_ + buf;

          LOG_INFO << "TcpServer::newConnection [" << servName_
                   << "] - new connection [" << connName
                   << "] from " << peerAddr.toIpPort();
          InetAddress localAddr(sockets::getLocalAddr(connfd));
          // FIXME poll with zero timeout to double confirm the new connection
          // FIXME use make_shared if necessary
          TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                                  connName,
                                                  connfd,
                                                  localAddr,
                                                  peerAddr));
          LOG_TRACE << "[1] usecount=" << conn.use_count();
          connections_[connName] = conn;
          LOG_TRACE << "[2] usecount=" << conn.use_count();
          conn->setConnectionCallback(connectionCallback_);
          conn->setMessageCallback(messageCallback_);
          conn->setCloseCallback(
            std::bind(&TcpServer::removeConnection, this, _1));
          conn->setWriteCompleteCallback(writeCompleteCallback_);  
        //conn->connectEstablished();
          ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
          LOG_TRACE << "[5] usecount=" << conn.use_count();

    }

    
    void TcpServer::removeConnection(const TcpConnectionPtr& conn)
    {
        loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
    }
        
    void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
    {
      loop_->assertInLoopThread();
      LOG_INFO << "TcpServer::removeConnectionInLoop [" << servName_
               << "] - connection " << conn->name();


      LOG_TRACE << "[8] usecount=" << conn.use_count();
      size_t n = connections_.erase(conn->name());
      LOG_TRACE << "[9] usecount=" << conn.use_count();

      (void)n;
      assert(n == 1);

      conn->getLoop()->queueInLoop(
          std::bind(&TcpConnection::connectDestroyed, conn)); //a uique_ptr(conn) object saved into the functor
      LOG_TRACE << "[10] usecount=" << conn.use_count();

    }
  
    void TcpServer::setConnectionCallback(const ConnectionCallback &cb)
    {
        //assert(_status==Stopped);
        connectionCallback_ = cb;
    }

    void TcpServer::setMessageCallback(const MessageCallback &cb)
    {
        //assert(_status==Stopped);
        messageCallback_ = cb;
    }
}
}
