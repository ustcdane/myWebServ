all: webServ stressTest
webServ: common.h  httpConn.cc  httpConn.h   main.cc threadPool.h	
	g++ -g -o webServ httpConn.cc common.h httpConn.h  main.cc \
		threadPool.h -pthread -std=c++11
stressTest:stressTest.cc
	g++ -g -o stressTest stressTest.cc 
