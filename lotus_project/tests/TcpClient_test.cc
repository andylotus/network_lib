#include <lotus/net/Channel.h>
#include <lotus/net/TcpClient.h>

#include <lotus/net/Log.h>
#include <lotus/net/EventLoop.h>
#include <lotus/net/InetAddress.h>
#include <lotus/net/Buffer.h>

#include <functional>

#include <stdio.h>

using namespace lotus;
using namespace lotus::net;
using namespace std::placeholders;

class TestClient
{
 public:
  TestClient(EventLoop* loop, const InetAddress& listenAddr)
    : loop_(loop),
      client_(loop, listenAddr, "TestClient"),
      stdinChannel_(loop, 0)// 0 is the stdin fd
  {
    client_.setConnectionCallback(
        std::bind(&TestClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&TestClient::onMessage, this, _1, _2, _3));
    //client_.enableRetry();
    // 标准输入缓冲区中有数据的时候，回调TestClient::handleRead
    stdinChannel_.setReadCallback(std::bind(&TestClient::handleRead, this));
	stdinChannel_.enableReading();		// 关注可读事件
  }

  void connect()
  {
    client_.connect();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    if (conn->connected())
    {
      printf("onConnection(): new connection [%s] from %s\n",
             conn->name().c_str(),
             conn->peerAddress().toIpPort().c_str());
    }
    else
    {
      printf("onConnection(): connection [%s] is down\n",
             conn->name().c_str());
    }
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
  {
    std::string msg(buf->retrieveAllAsString());
    printf("onMessage(): recv a message [%s]\n", msg.c_str());
    LOG_TRACE << conn->name() << " recv " << msg.size() << " bytes at " << time.time_since_epoch().count();
  }

  // 标准输入缓冲区中有数据的时候，回调该函数
  void handleRead()
  {
    char buf[1024] = {0};
    fgets(buf, 1024, stdin);
	buf[strlen(buf)-1] = '\0';		// 去除\n
	client_.connection()->send(buf);
  }

  EventLoop* loop_;
  TcpClient client_;
  Channel stdinChannel_;		// 标准输入Channel
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
  EventLoop loop;
  InetAddress serverAddr("127.0.0.1", 8888);
  TestClient client(&loop, serverAddr);
  client.connect();
  loop.loop();
}

