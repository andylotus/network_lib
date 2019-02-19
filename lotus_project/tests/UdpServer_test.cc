#include <lotus/net/EventLoop.h>
#include <lotus/net/InetAddress.h>
#include <lotus/net/UdpServer.h>

#include <functional>
#include <string>

#include <stdio.h>

using namespace lotus;
using namespace lotus::net;
using namespace lotus::net::udp;
using namespace std::placeholders;

class TestServer
{
 public:
  TestServer(EventLoop* loop,
             const InetAddress& listenAddr)
    : loop_(loop),
      server_(loop, listenAddr, "TestServer")
  {
    //server_.setConnectionCallback(
        //std::bind(&TestServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&TestServer::onMessage, this, _1, _2, _3));

  }

  void start()
  {
	  server_.start();
  }

 private:
	/*
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
*/
  void onMessage( UdpServer* serv,
                 std::string buf,
                 InetAddress peer)
  {  std::string msg(buf);
    //std::string msg(buf->retrieveAllAsString());
    printf("onMessage(): received %zd bytes from connection [%s]\n",
           msg.size(),
           peer.toIpPort().c_str() );
    serv->sendTo(msg, peer);
    //serv->stop();
  }

  EventLoop* loop_;
  UdpServer server_;
};


int main()
{
  printf("main(): pid = %d\n", getpid());

  InetAddress listenAddr(8888);
  EventLoop loop;

  TestServer server(&loop, listenAddr);
  server.start();

  loop.loop();
}