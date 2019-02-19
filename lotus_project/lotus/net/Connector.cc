#include "Connector.h"
#include "Log.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Sockets.h"
#include "InetAddress.h"

#include <functional>
#include <errno.h>
#include <string.h>

using namespace lotus;
using namespace lotus::net;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
  : loop_(loop),
    serverAddr_(serverAddr),
    connect_(false),
    state_(kDisconnected),
    retryDelayMs_(kInitRetryDelayMs)
{
  LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector()
{
  LOG_DEBUG << "dtor[" << this << "]";
  assert(!channel_);
}

// thread-safe
void Connector::start()
{
  connect_ = true;
  loop_->runInLoop(std::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}

void Connector::startInLoop()
{
  loop_->assertInLoopThread();
  assert(state_ == kDisconnected);
  if (connect_)
  {
    connect();
  }
  else
  {
    LOG_DEBUG << "do not connect";
  }
}

void Connector::stop()
{
  connect_ = false;
  loop_->runInLoop(std::bind(&Connector::stopInLoop, this)); // FIXME: unsafe
  // FIXME: cancel timer
}

void Connector::stopInLoop()
{
  loop_->assertInLoopThread();
  if (state_ == kConnecting)
  {
    setState(kDisconnected);
    int sockfd = removeAndResetChannel();	// remove channel from poller, and reset channel as empty
    retry(sockfd);		// only to call sockets::close(sockfd);
  }
}

void Connector::connect()
{
  int sockfd = sockets::createNonblockingSocket();	
  int ret = sockets::connect(sockfd, InetAddress(serverAddr_.getSockAddrInet()));
  int savedErrno = (ret == 0) ? 0 : errno;
  switch (savedErrno)
  {
    case 0:
    case EINPROGRESS:	// 非阻塞套接字，未连接成功返回码是EINPROGRESS表示正在连接
    case EINTR:
    case EISCONN:			// 连接成功
      connecting(sockfd);
      break;

    case EAGAIN: //本机ephemeral port暂时用完，要关闭socket再延期重试
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      retry(sockfd);		// 重连
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      LOG_ERROR << "connect error in Connector::startInLoop " << savedErrno;
      sockets::close(sockfd);	// 不能重连，关闭sockfd
      break;

    default:
      LOG_ERROR << "Unexpected error in Connector::startInLoop " << savedErrno;
      sockets::close(sockfd);
      // connectErrorCallback_();
      break;
  }
}

// 不能跨线程调用
void Connector::restart()
{
  loop_->assertInLoopThread();
  setState(kDisconnected);
  retryDelayMs_ = kInitRetryDelayMs;
  connect_ = true;
  startInLoop();
}

void Connector::connecting(int sockfd)
{
  setState(kConnecting);
  assert(!channel_);
  // Channel与sockfd关联
  channel_.reset(new Channel(loop_, sockfd));
  // 设置可写回调函数，这时候如果socket没有错误，sockfd就处于可写状态
  channel_->setWriteCallback(
      std::bind(&Connector::handleWrite, this)); // FIXME: unsafe
  // 设置错误回调函数
  channel_->setErrorCallback(
      std::bind(&Connector::handleError, this)); // FIXME: unsafe

  // channel_->tie(shared_from_this()); is not working,
  // as channel_ is not managed by shared_ptr
  channel_->enableWriting();		// 让Poller关注可写事件
}

int Connector::removeAndResetChannel()
{
  channel_->disableAll();
  channel_->remove();			// 从poller移除关注
  int sockfd = channel_->fd();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
  // 不能在这里重置channel_，因为正在调用Channel::handleEvent
  loop_->queueInLoop(std::bind(&Connector::resetChannel, this)); // FIXME: unsafe
  return sockfd;
}

void Connector::resetChannel()
{
  channel_.reset();		// channel_ 置空
}


void Connector::handleWrite()
{
  LOG_TRACE << "Connector::handleWrite " << state_;

  if (state_ == kConnecting)
  {
    int sockfd = removeAndResetChannel();	// 从poller中移除关注，并将channel置空
    // socket可写并不意味着连接一定建立成功
    // 还需要用getsockopt(sockfd, SOL_SOCKET, SO_ERROR, ...)再次确认一下。
    int err = sockets::getSocketError(sockfd);
    if (err)		// 有错误
    {
      LOG_WARN << "Connector::handleWrite - SO_ERROR = "
               << err << " " << strerror(err);
      retry(sockfd);		// 重连
    }
    else if (sockets::isSelfConnect(sockfd))		// 自连接
    {
      LOG_WARN << "Connector::handleWrite - Self connect";
      retry(sockfd);		// 重连
    }
    else	// 连接成功
    {
      setState(kConnected);
      if (connect_)
      {
        newConnectionCallback_(sockfd);		// 回调
      }
      else
      {
        sockets::close(sockfd);
      }
    }
  }
  else
  {
    // what happened?
    assert(state_ == kDisconnected);
  }
}

void Connector::handleError()
{
  LOG_ERROR << "Connector::handleError";
  assert(state_ == kConnecting);

  int sockfd = removeAndResetChannel();		// 从poller中移除关注，并将channel置空
  int err = sockets::getSocketError(sockfd);
  LOG_TRACE << "SO_ERROR = " << err << " " << strerror(err);
  retry(sockfd);
}

// 采用back-off策略重连，即重连时间逐渐延长，0.5s, 1s, 2s, ...直至30s
void Connector::retry(int sockfd)
{
  sockets::close(sockfd);
  setState(kDisconnected);
  if (connect_)
  {
    LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
             << " in " << retryDelayMs_ << " milliseconds. ";
    // 注册一个定时操作，重连
    loop_->runAfter(retryDelayMs_/1000.0,
                    std::bind(&Connector::startInLoop, shared_from_this()));
    retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
  }
  else
  {
    LOG_DEBUG << "do not connect";
  }
}

