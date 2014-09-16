// Copyright (c) 2014 Daniel Wang. All rights reserved.
// https://github.com/ustcdane/myWebServ
// Use of this source code is governed by a BSD-style license
// that can be found in the License file
// Author: Daniel Wang(daneustc at gmail dot com)

#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <fcntl.h>
#include <sys/eventfd.h>
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <thread>

#include "epoller.h"
#include "httpConn.h"

namespace myWebServ
{

class httpConn;

/**
 *  对具体事件处理的封装loop的封装
 */
class eventLoop
{
public:
   eventLoop(int efd):event_fd(efd), pEpoll(new epoller())
   {
		if(pEpoll->create()<0)// create epoll error
		{
			delete pEpoll;
			pEpoll = nullptr;
		}

		// add event fd to epoll, set don't EPOLLONESHOT
		pEpoll->add(event_fd, false);

		// create running thread
		thread_ = std::thread(std::bind(&eventLoop::loop, this));
   }

    ~eventLoop()
    {
        if(!running && isValid())
        {
            for(auto it = map_fd_http.begin(); it!=map_fd_http.end(); ++it)
            {
                close_con(it->first);
            }
			delete pEpoll;
        }
	thread_.join();
    }

    void push_new_con(int fd)
    {
        std::lock_guard<std::mutex> lck(mx);
        fd_con_queue.push(fd);
    }

    int get_event_fd() const
    {
	return event_fd;
    }

    bool isValid() const
    {
        return pEpoll && event_fd>0;
    }

    void close_con(int fd);// close connection
    void loop();
private:
    bool running;
    epoller *pEpoll;
    int event_fd;
    std::thread thread_;
    std::unordered_map<int, httpConn* > map_fd_http;
    std::queue<int> fd_con_queue;
    std::mutex mx;
};

}

#endif //EVENTLOOP_H
