#ifndef LOTUS_NET_TCPCLIENT_H
#define LOTUS_NET_TCPCLIENT_H

#include "noncopyable.h"
#include <mutex>
#include "TcpConnection.h"
#include <string>

namespace lotus
{
namespace net
{

class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient : public noncopyable
{
 public:
  TcpClient(EventLoop* loop,
            const InetAddress& serverAddr,
            const std::string& name);
  ~TcpClient();  // force out-line dtor, for scoped_ptr members.

  void connect();
  void disconnect();
  void stop();

  TcpConnectionPtr connection() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return connection_;
  }

  bool retry() const;
  void enableRetry() { retry_ = true; }

  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_ = cb; }

 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd);
  /// Not thread safe, but in loop
  void removeConnection(const TcpConnectionPtr& conn);

  EventLoop* loop_;
  ConnectorPtr connector_;	// 用于主动发起连接
  const std::string name_;		// 名称
  ConnectionCallback connectionCallback_;		// 连接建立回调函数
  MessageCallback messageCallback_;				// 消息到来回调函数
  WriteCompleteCallback writeCompleteCallback_;	// 数据发送完毕回调函数
  bool retry_;   // 重连，是指连接建立之后又断开的时候是否重连
  bool connect_; // atomic
  // always in loop thread
  int nextConnId_;			// name_ + nextConnId_用于标识一个连接
  mutable std::mutex mutex_;
  TcpConnectionPtr connection_; // Connector连接成功以后，得到一个TcpConnection
};

}
}

#endif  // LOTUS_NET_TCPCLIENT_H
