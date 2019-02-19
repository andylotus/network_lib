#include "sudoku.h"

//#include <lotus/base/Atomic.h>
//#include <lotus/base/Logging.h>
//#include <lotus/base/Thread.h>
#include <lotus/net/Log.h>
#include <lotus/net/EventLoop.h>
#include <lotus/net/InetAddress.h>
#include <lotus/net/TcpServer.h>

#include <functional>
#include <string>

#include <utility>

#include <mcheck.h>
#include <stdio.h>
#include <unistd.h>
#include <lotus/net/Callbacks.h>

using namespace lotus;
using namespace lotus::net;
using namespace std::placeholders;

class SudokuServer
{
 public:
  SudokuServer(EventLoop* loop, const InetAddress& listenAddr)
    : loop_(loop),
      server_(loop, listenAddr, "SudokuServer"),
      startTime_(std::chrono::system_clock::now()) //Timestamp::now())
  {
    server_.setConnectionCallback(
        std::bind(&SudokuServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&SudokuServer::onMessage, this, _1, _2, _3));
  }

  void start()
  {
    server_.start();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
        << conn->localAddress().toIpPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp)
  {
    LOG_DEBUG << conn->name();
    size_t len = buf->readableBytes();
    while (len >= kCells + 2)
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        //in current setup, please NO /r/n at the end of the string!!!
        std::string request(buf->peek(), crlf);
        buf->retrieveUntil(crlf + 2);
        //printf("buf is now %s", buf);
        len = buf->readableBytes();
        //LOG_TRACE << request;
        LOG_TRACE <<"get before processRequest()";
        if (!processRequest(conn, request))
        {
          conn->send("Bad Request!\r\n");
          conn->shutdown();
          break;
        }
      }
      else if (len > 100) // id + ":" + kCells + "\r\n"
      {
        conn->send("Id too long!\r\n");
        conn->shutdown();
        break;
      }
      else
      {
        break;
      }
      //LOG_TRACE <<"in while";
    }
    //LOG_TRACE <<"end of onMessage";
  }

  bool processRequest(const TcpConnectionPtr& conn, const std::string& request)
  {
    std::string id;
    std::string puzzle;
    bool goodRequest = true;
//LOG_TRACE << "get before colon ";
    std::string::const_iterator colon = find(request.begin(), request.end(), ':');
    if (colon != request.end())
    {
      id.assign(request.begin(), colon);
      puzzle.assign(colon+1, request.end());
    }
    else
    {
      puzzle = request;
      //LOG_TRACE << "get here";
    }

    if (puzzle.size() == static_cast<size_t>(kCells))
    {
      LOG_DEBUG << conn->name();
      std::string result = solveSudoku(puzzle);
      if (id.empty())
      {
        conn->send(result+"\r\n");
      }
      else
      {
        conn->send(id+":"+result+"\r\n");
      }
    }
    else
    {
      goodRequest = false;
      //LOG_TRACE << "get goodRequest = false;";
    }
    return goodRequest;
  }

  EventLoop* loop_;
  TcpServer server_;
  Timestamp startTime_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
  EventLoop loop;
  InetAddress listenAddr(8888);
  SudokuServer server(&loop, listenAddr);

  server.start();

  loop.loop();
}

