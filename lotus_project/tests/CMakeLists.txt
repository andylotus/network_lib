add_executable(tcpserver_test TcpServer_test.cc)
target_link_libraries(tcpserver_test lotus_net)

add_executable(tcpclient_test TcpClient_test.cc)
target_link_libraries(tcpclient_test lotus_net)

add_executable(udpserver_test UdpServer_test.cc)
target_link_libraries(udpserver_test lotus_net)

add_executable(udpclient_test UdpClient_test.cc)
target_link_libraries(udpclient_test lotus_net)
