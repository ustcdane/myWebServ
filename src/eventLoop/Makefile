all: webServ stressTest
webServ:  epoller.h epoller.cc eventLoop.h eventLoop.cc httpConn.cc  httpConn.h   main.cc 	
	g++ -g -o webServ epoller.h epoller.cc eventLoop.h eventLoop.cc httpConn.h \
		httpConn.cc  main.cc -pthread -std=c++11
stressClient:stressTest.cc
	g++ -g -o stressTest stressTest.cc 
