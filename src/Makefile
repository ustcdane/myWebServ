all: webServ stressClient
webServ: common.h  httpConn.cc  httpConn.h   main.cc threadPool.h	
	g++ -g -o webServ httpConn.cc common.h httpConn.h  main.cc \
		threadPool.h -pthread -std=c++11
stressClient:stressClient.cc
	g++ -g -o stressClient stressClient.cc 
