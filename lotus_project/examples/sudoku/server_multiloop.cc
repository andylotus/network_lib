#include "sudoku.h"

//#include <lotus/base/Atomic.h>
//#include <lotus/base/Logging.h>
//#include <lotus/base/Thread.h>
#include <lotus/net/EventLoop.h>
#include <lotus/net/InetAddress.h>
#include <lotus/net/TcpServer.h>
#include <lotus/net/Log.h>

//#include <std/bind.hpp>
#include <functional>
#include <string>

#include <utility>

#include <mcheck.h>
#include <stdio.h>
#include <unistd.h>

using namespace lotus;
using namespace lotus::net;
using namespace std::placeholders;


class SudokuServer
{
 public:
  SudokuServer(EventLoop* loop, const InetAddress& listenAddr, int numThreads)
    : loop_(loop),
      server_(loop, listenAddr, "SudokuServer"),
      numThreads_(numThreads),
      startTime_(std::chrono::system_clock::now())
  {
    server_.setConnectionCallback(
        std::bind(&SudokuServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&SudokuServer::onMessage, this, _1, _2, _3));
    server_.setThreadNum(numThreads);
  }

  void start()
  {
    LOG_INFO << "starting " << numThreads_ << " threads.";
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
        std::string request(buf->peek(), crlf);
        buf->retrieveUntil(crlf + 2);
        len = buf->readableBytes();
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
    }
  }

  bool processRequest(const TcpConnectionPtr& conn, const std::string& request)
  {
    std::string id;
    std::string puzzle;
    bool goodRequest = true;

    std::string::const_iterator colon = find(request.begin(), request.end(), ':');
    if (colon != request.end())
    {
      id.assign(request.begin(), colon);
      puzzle.assign(colon+1, request.end());
    }
    else
    {
      puzzle = request;
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
    }
    return goodRequest;
  }

  EventLoop* loop_;
  TcpServer server_;
  int numThreads_;
  Timestamp startTime_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
  int numThreads = 0;
  if (argc > 1)
  {
    numThreads = atoi(argv[1]);
  }
  EventLoop loop;
  InetAddress listenAddr(8888);
  SudokuServer server(&loop, listenAddr, numThreads);

  server.start();

  loop.loop();
}

