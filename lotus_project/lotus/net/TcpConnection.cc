#include <cstring>
#include "TcpConnection.h"
#include "EventLoop.h"
#include "Buffer.h"
#include "Log.h"
#include "Sockets.h"
#include <functional>
#include <string>

    
void lotus::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
  LOG_TRACE << conn->localAddress().toIpPort() << " -> "
			<< conn->peerAddress().toIpPort() << " is "
			<< (conn->connected() ? "UP" : "DOWN");
}

void lotus::net::defaultMessageCallback(const TcpConnectionPtr&,
										Buffer* buf,
										Timestamp)
{
  buf->retrieveAll();
}    
 

namespace lotus
{
namespace net
{
    
    TcpConnection::TcpConnection(EventLoop *loop, const std::string& name, int sockfd, const InetAddress &local_addr
                                 , const InetAddress &peer_add)
            : 
              loop_(loop),
              name_(name),
              sockfd_(sockfd)
              , channel_(new  Channel(loop, sockfd))
              , status_(Connecting)
              , localAddr_(local_addr)
              , peerAddr_(peer_add),
                 highWaterMark_(64*1024*1024) //64*1K*1K = 64M(B)
    {
      channel_->setReadCallback(
          std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
      //Mostly TcpConnection::handleClose is called at TcpConnection::handleRead when read = 0, not from here
      channel_->setCloseCallback(
          std::bind(&TcpConnection::handleClose, this));
      
      channel_->setErrorCallback(
          std::bind(&TcpConnection::handleError, this));                  
      LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
                << " fd=" << sockfd;
      sockets::setKeepAlive(sockfd, true);
    }

    TcpConnection::~TcpConnection()noexcept
    {
        LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd();
    }
    
    void TcpConnection::connectEstablished()
    {
          loop_->assertInLoopThread();
          assert(status_ == Connecting);
          setStatus(Connected);
          LOG_TRACE << "[3] usecount=" << shared_from_this().use_count();
          channel_->tie(shared_from_this());
          channel_->enableReading();	// TcpConnection所对应的通道加入到Poller关注

          connectionCallback_(shared_from_this());
          LOG_TRACE << "[4] usecount=" << shared_from_this().use_count();
    }
    
    void TcpConnection::connectDestroyed()
    {
      loop_->assertInLoopThread();
      LOG_TRACE << "[13] usecount=" <<shared_from_this().use_count();
      if (status_ == Connected)
      {
        setStatus(Disconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
      }
      channel_->remove();
    }

    void TcpConnection::handleRead(Timestamp receiveTime)
    {
        loop_->assertInLoopThread();
        //char buf[65536];
        //ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
        int savedErrno = 0;
        ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
        if(n > 0)
          messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
        else if(n == 0)
        {
            //LOG_TRACE << "get into read::n==0";
            handleClose();
        }
        else
        {
            //errno = savedErrno;
            LOG_ERROR << "TcpConnection::handleRead";
            handleError();
        }

    }

    void TcpConnection::handleClose()
    {
      loop_->assertInLoopThread();
      LOG_TRACE << "fd = " << channel_->fd() << " status = " << status_;
      assert(status_ == Connected || status_ == Disconnecting);
      // we don't close fd, leave it to dtor, so we can find leaks easily.
      setStatus(Disconnected);
      channel_->disableAll(); //call Epoll::update() --- CTL_MOD

      TcpConnectionPtr guardThis(shared_from_this()); //should NOT be TcpConnectionPtr guardThis(this) --- this creates a new TcpConnectionPtr obj, instead of copying the existing one;
      connectionCallback_(guardThis);		// 这一行，可以不调用。实际调用的就是用户提供的OnConnection（）函数，由TcpServer注册
      LOG_TRACE << "[7] usecount=" << guardThis.use_count();
      // must be the last line
      closeCallback_(guardThis);	// 调用TcpServer::removeConnection --> TcpConnection::connectDestroyed -->Epoll::remove -- CTL_DEL
      LOG_TRACE << "[11] usecount=" << guardThis.use_count();
    } //退出handleClose（）后，guardThis会被销毁

    void TcpConnection::handleError()
    {
      int err = sockets::getSocketError(channel_->fd());
      LOG_ERROR << "TcpConnection::handleError [" << name_
                << "] - SO_ERROR = " << err << " " << strerror(err);
    } 


    
// 线程安全，可以跨线程调用
void TcpConnection::send(const char* data, size_t len)
{
  if (status_ == Connected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(data, len);
    }
    else
    {
      std::string message(static_cast<const char*>(data), len);
      loop_->runInLoop(
          std::bind(&TcpConnection::sendStrInLoop,
                      this,
                      message));
    }
  }
}

// 线程安全，可以跨线程调用
void TcpConnection::send(const std::string& message)
{
  if (status_ == Connected)
  {
    if (loop_->isInLoopThread())
    {
      sendStrInLoop(message);
    }
    else
    {
      loop_->runInLoop(std::bind(&TcpConnection::sendStrInLoop, this, message));
                    //std::forward<std::string>(message)));
    }
  }
}

// 线程安全，可以跨线程调用
void TcpConnection::send(Buffer* buf)
{
  if (status_ == Connected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    }
    else
    {
      loop_->runInLoop(
          std::bind(&TcpConnection::sendStrInLoop,
                      this,
                      buf->retrieveAllAsString()));
                    //std::forward<string>(message)));
    }
  }
}

void TcpConnection::sendStrInLoop(const std::string& message)
{
  sendInLoop(message.data(), message.size());
     //loop_->assertInLoopThread();
    //::write(channel_->fd(), message.data(), message.size());
}


void TcpConnection::sendInLoop(const char* data, size_t len)
{
  
  loop_->assertInLoopThread();
  //::write(channel_->fd(), data, len);

  ssize_t nwrote = 0;
  size_t remaining = len;
  bool error = false;
  if (status_ == Disconnected)
  {
    LOG_WARN << "disconnected, give up writing";
    return;
  }
  // if no thing in output queue, try writing directly
  // 通道没有关注可写事件并且发送缓冲区没有数据，直接write
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
  {
    nwrote = ::write(channel_->fd(), data, len);
    if (nwrote >= 0)
    {
      remaining = len - nwrote;
	  // 写完了，回调writeCompleteCallback_
      if (remaining == 0 && writeCompleteCallback_)
      {
        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
      }
    }
    else // nwrote < 0
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
        LOG_ERROR << "TcpConnection::sendInLoop";
        if (errno == EPIPE) // FIXME: any others?
        {
          error = true;
        }
      }
    }
  }

  assert(remaining <= len);
  // 没有错误，并且还有未写完的数据（说明内核发送缓冲区满，要将未写完的数据添加到output buffer中）
  if (!error && remaining > 0)
  {
    LOG_TRACE << "I am going to write more data";
    size_t oldLen = outputBuffer_.readableBytes(); //目前outputBuffer_中的数据长度
	// 如果超过highWaterMark_（高水位标），回调highWaterMarkCallback_
    if (oldLen + remaining >= highWaterMark_
        && oldLen < highWaterMark_
        && highWaterMarkCallback_)
    {
      loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
    }
    outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
    if (!channel_->isWriting())
    {
      channel_->enableWriting();		// 关注POLLOUT事件
    }
  } 
} 

// 内核发送缓冲区有空间了，回调该函数
void TcpConnection::handleWrite()
{
  loop_->assertInLoopThread();
  if (channel_->isWriting())
  {
    ssize_t n = ::write(channel_->fd(),
                               outputBuffer_.peek(),
                               outputBuffer_.readableBytes());
    if (n > 0)
    {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0)	 // 发送缓冲区已清空
      {
        channel_->disableWriting();		// 停止关注POLLOUT事件，以免出现busy loop
        if (writeCompleteCallback_)		// 回调writeCompleteCallback_
        {
          // 应用层发送缓冲区被清空，就回调用writeCompleteCallback_
          loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (status_ == Disconnecting)	// 发送缓冲区已清空并且连接状态是kDisconnecting, 要关闭连接
        {
          shutdownInLoop();		// 关闭连接
        }
      }
      else
      {
        LOG_TRACE << "I am going to write more data";
      }
    }
    else
    {
      LOG_ERROR << "TcpConnection::handleWrite";
    }
  }
  else
  {
    LOG_TRACE << "Connection fd = " << channel_->fd()
              << " is down, no more writing";
  }
}

void TcpConnection::shutdown()
{
  // FIXME: use compare and swap
  if (status_ == Connected)
  {
    setStatus(Disconnecting);
    // FIXME: shared_from_this()?
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop()
{
  loop_->assertInLoopThread();
  if (!channel_->isWriting())
  {
    // we are not writing
    sockets::shutdownWrite(sockfd_);
  }
}

void TcpConnection::setTcpNoDelay(bool on)
{
  sockets::setTcpNoDelay(sockfd_, on);
}

}
}

