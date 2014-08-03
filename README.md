myWebServ
=========

A simple web server, writing by c/c++11, based on thread pool. 

#write in c++11
http_conn.cpp  http_conn.h main.cpp threadPool.h are server code.
stress_client.cpp is stress test code.

To run,just make Makefile.

Run:
./webServ 127.0.0.1 1111 #ip port
./stressClient 127.0.0.1 1111 1000 # 

基于C++11，多线程。属于半同步/半反应堆模式的服务端程序。
webServ 是生成的web程序。stressClient 为压力测试程序。
