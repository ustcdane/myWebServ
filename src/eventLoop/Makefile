all: myWebServ stressTest
myWebServ:epoller.h epoller.cc eventLoop.h eventLoop.cc httpConn.cc  httpConn.h   main.cc 	
	g++ -g -o myWebServ epoller.h epoller.cc eventLoop.h eventLoop.cc httpConn.h \
		httpConn.cc  main.cc -pthread -std=c++11
stressTest:stressTest.cc
	g++ -g -o stressTest stressTest.cc 
