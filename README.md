# network_lib
lotus is a multithreaded non-blocking event-driven C++ network library based on reactor pattern.

this medium-scale lotus project was a coding exercise(in 2018), which was referred to Chen Shuo's muduo project. 
It followed the same naming rules as in muduo.

Here is a description of this lotus project:
1) fully based on C++11, and uses std::bind/function widely instead of virtual function;
2) C++11 , and smart_pointer applied in related application;
3) the reactor pattern worked together with timer and Linux epoll;
4) both TCP and UDP covered in the lib, and managed by corresponding Classes;
5) a simplified logger introduced into the network lib;
6) timer_queue implemented as a mini-heap, which shows excellent performance;
7) implemented a concise threadpool for multithread programming;
8) supported concurrent programming for both TCP and UDP server in 'multithread + one loop per thread' model;
9) rewrote epoll, sockets related topics and 'send' in TCPConnection;

In the tests and examples folders, some testing files were provided to validate the code development.
