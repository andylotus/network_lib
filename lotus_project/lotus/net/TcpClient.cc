#include "TcpClient.h"
#include "Log.h"
#include "Connector.h"
#include "EventLoop.h"
#include "Sockets.h"

#include <functional>
#include <stdio.h>  // snprintf

using namespace lotus;
using namespace lotus::net;
using namespace std::placeholders;

namespace lotus
{
namespace net
{
	namespace detail
	{

		void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
		{
		  loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
		}

		void removeConnector(const ConnectorPtr& connector)
		{
		  //connector->
		}

	}
}
}

TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr,
                     const std::string& name)
  : loop_(loop),
    connector_(new Connector(loop, serverAddr)),
    name_(name),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),
    retry_(false),
    connect_(true),
    nextConnId_(1)
{
  // 设置连接成功回调函数
  connector_->setNewConnectionCallback(
      std::bind(&TcpClient::newConnection, this, _1));
  // FIXME setConnectFailedCallback
  LOG_INFO << "TcpClient::TcpClient[" << name_
           << "] - connector " << connector_.get();
}

TcpClient::~TcpClient()
{
  LOG_INFO << "TcpClient::~TcpClient[" << name_
           << "] - connector " << connector_.get();
  TcpConnectionPtr conn;
  {
     std::lock_guard<std::mutex> lock(mutex_);
    conn = connection_;
  }
  if (conn)
  {
    // FIXME: not 100% safe, if we are in different thread

	// 重新设置TcpConnection中的closeCallback_为detail::removeConnection
    CloseCallback cb = std::bind(&detail::removeConnection, loop_, _1);
    loop_->runInLoop(
        std::bind(&TcpConnection::setCloseCallback, conn, cb));
  }
  else
  {
    // 这种情况，说明connector处于未连接状态，将connector_停止
    connector_->stop();
    // FIXME: HACK
    loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
  }
}

void TcpClient::connect()
{
  // FIXME: check state
  LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
           << connector_->serverAddress().toIpPort();
  connect_ = true;
  connector_->start();	// 发起连接
}

// 用于连接已建立的情况下，关闭连接
void TcpClient::disconnect()
{
  connect_ = false;

  {
     std::lock_guard<std::mutex> lock(mutex_);
    if (connection_)
    {
      connection_->shutdown();
    }
  }
}

// 停止connector_ ---the connection may not be successful yet!
void TcpClient::stop()
{
  connect_ = false;
  connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
  loop_->assertInLoopThread();
  InetAddress peerAddr(sockets::getPeerAddr(sockfd));
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
  ++nextConnId_;
  std::string connName = name_ + buf;

  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(loop_,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));

  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      std::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
  {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_ = conn;		// 保存TcpConnection
  }
  conn->connectEstablished();		// 这里回调connectionCallback_
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  assert(loop_ == conn->getLoop());

  {
    std::lock_guard<std::mutex> lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  if (retry_ && connect_)
  {
    LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to "
             << connector_->serverAddress().toIpPort();
    // 这里的重连是指连接建立成功之后被断开的重连
    connector_->restart();
  }
}

