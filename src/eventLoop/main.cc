// Copyright (c) 2014 Daniel Wang. All rights reserved.
// https://github.com/ustcdane/myWebServ
// Use of this source code is governed by a BSD-style license
// that can be found in the License file
// Author: Daniel Wang(daneustc at gmail dot com)

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <memory>
#include <algorithm>
#include <vector>
#include <thread>
#include <memory>

#include "epoller.h"
#include "eventLoop.h"
#include "httpConn.h"

using myWebServ::epoller;
using myWebServ::httpConn;
using myWebServ::eventLoop;

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000


void addsig( int sig, void( handler )(int), bool restart = true )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    if( restart )
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

void show_error( int connfd, const char* info )
{
    printf( "%s", info );
    send( connfd, info, strlen( info ), 0 );
    close( connfd );
}

static int last_thread_id=-1;
// round-robin 方式分配
int disPatch_con_new(const std::vector<eventLoop*> &vec)
{
	int index = (last_thread_id+1)%vec.size();
	last_thread_id = index;
	return index;// 返回处理新连接的线程下标
}

int main( int argc, char* argv[] )
{
    if( argc <= 3 )
    {
        printf( "usage: %s ip_address port_number loops's numbers\n", basename( argv[0] ) );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );
	int loops = atoi(argv[3]);

    addsig( SIGPIPE, SIG_IGN );
    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );
	// set SO_LINGER
    struct linger tmp = { 1, 0 };
    setsockopt( listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof( tmp ) );
    // set TCP_DEFER_ACCEPT
	int timeout=10;
	setsockopt(listenfd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(timeout));

    int ret = 0;
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret >= 0 );

    ret = listen( listenfd, 5 );
    assert( ret >= 0 );


    epoller *pEpoll = new epoller();
	pEpoll->setMaxEvents(10240);
	if(pEpoll->create()<0)
	{
		delete pEpoll;
		perror("epoll create.");
		return -1;
	}
	//pEpoll->setnonblocking(listenfd);
	pEpoll->add(listenfd, false);// don't set EPOLLONESHOT
   // create event loops

	std::vector<eventLoop*> vec_loops(loops, nullptr);
	for(int i=0; i<loops; ++i)
	{
		int efd = eventfd(0, EFD_NONBLOCK);
		vec_loops[i] = new eventLoop(efd);
		printf("create loop %d\n", i);
	}
    
    int nums;
	/// main event loop
    while(1)
    {
		nums = pEpoll->wait();
        int i;
        if((nums<0) && errno!=EINTR)
		{
			printf("epoll failure\n");
			break;
		}

		for(i=0; i<nums; ++i)
        {
			struct epoll_event event = (*pEpoll)[i];
			int sockfd = event.data.fd; 
			if ((event.events & EPOLLERR) ||
					(event.events & EPOLLHUP) ||
					(!(event.events & EPOLLIN)))
			{
				/* An error has occured on this fd, or the socket is not
                 ready for reading (why were we notified then?) */
				fprintf (stderr, "epoll error\n");
				close (sockfd);
				continue;
			}
            else if( sockfd == listenfd ) // new socket
            {
				 /* We have a notification on the listening socket, which
				    means one or more incoming connections. */
				while (1)
				{
					struct sockaddr_in client_address;
					socklen_t client_addrlength = sizeof( client_address );
					int connfd = accept( listenfd, ( struct sockaddr* )&client_address, 
							&client_addrlength );
					
					if ( connfd ==-1 )
					{
						if ((errno == EAGAIN)||(errno == EWOULDBLOCK))
						{  /* We have processed all incomingconnections. */
							break;
						}
						else
						{
							perror ("accept");
							break;
						}
					}

					/// dispatch new socket

					int index =disPatch_con_new(vec_loops); 
					vec_loops[index]->push_new_con(connfd);// import lock seq
					printf("--> socket %d push in no: %d\n", connfd, index);
					while(1)// nonblocking, notify
					{
						uint64_t u=1;
						int ret = write(vec_loops[index]->get_event_fd(), &u, sizeof(u));
						if(ret==-1)
						{
							if(errno != EAGAIN)
							{
								printf("errno is:%d\n", errno);
								break;
							}
						}
						else// success write
						{
							break;
						}
					}// while

				}// while(1)
            }// if
        }// for

    }// while
	std::for_each(vec_loops.begin(), vec_loops.end(), [](eventLoop *p)
			{
				delete p;
			} 
			);
    delete pEpoll;
    close( listenfd );
    return 0;
}
