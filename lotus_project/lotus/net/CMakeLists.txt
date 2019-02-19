set(net_SRCS
  EventLoop.cc
  Channel.cc
  Epoll.cc
  Log.cc
  Timer.cc
  TimerQueue.cc
  EventLoopThread.cc
  EventLoopThreadPool.cc
  Acceptor.cc
  InetAddress.cc
  Sockets.cc
  TcpConnection.cc
  TcpServer.cc
  Buffer.cc
  Connector.cc
  TcpClient.cc
  UdpServer.cc
  UdpClient.cc
  Threadpool.cc
  )

add_library(lotus_net ${net_SRCS})
#target_link_libraries(lotus_net lotus_base)

install(TARGETS lotus_net DESTINATION lib)
set(HEADERS
  EventLoop.h
  Channel.h
  Epoll.h
  CurrentThread.h
  noncopyable.h
  Log.h
  Timer.h
  TimerQueue.h
  TimerId.h
  EventLooopThread.h
  EventLoopThreadPool.h
  Acceptor.h
  InetAddress.h
  Sockets.h
  TcpConnection.h
  TcpServer.h
  Buffer.h
  Connector.h
  TcpClient.h
  UdpServer.h
  UdpClient.h
  Threadpool.h
  )
install(FILES ${HEADERS} DESTINATION include/lotus/net)

#add_subdirectory(http)
#add_subdirectory(inspect)
#
#if(NOT CMAKE_BUILD_NO_EXAMPLES)
#  add_subdirectory(tests)
#endif()
#
#if(PROTOBUF_FOUND)
#  add_subdirectory(protorpc)
#else()
#  add_subdirectory(protorpc EXCLUDE_FROM_ALL)
#endif()
