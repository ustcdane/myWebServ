all: webServ stressClient
webServ: common.h  httpConn.cc  httpConn.h   main.cpp threadPool.h	
	g++ -g -o webServ httpConn.cc common.h httpConn.h  main.cpp \
		threadPool.h -pthread -std=c++11
stressClient:stress_client.cpp
	g++ -g -o stressClient stress_client.cpp 
