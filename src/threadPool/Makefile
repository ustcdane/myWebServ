all: MyWebServ stressTest
MyWebServ: common.h  httpConn.cc  httpConn.h   main.cc threadPool.h	
	g++ -g -o myWebServ httpConn.cc common.h httpConn.h  main.cc \
		threadPool.h -pthread -std=c++11
stressTest:stressTest.cc
	g++ -g -o stressTest stressTest.cc 
