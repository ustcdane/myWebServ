// Copyright (c) 2014 Daniel Wang. All rights reserved.
// https://github.com/ustcdane/myWebServ
// Use of this source code is governed by a BSD-style license
// that can be found in the License file
// Author: Daniel Wang(daneustc at gmail dot com)

#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

namespace myWebServ
{

/**
 *  对epoll的封装
 */
class epoller
{
public:
    enum EPOLL_OP {ADD = EPOLL_CTL_ADD, MOD = EPOLL_CTL_MOD, DEL = EPOLL_CTL_DEL};
    /**
     * 最大的连接数和最大的回传事件数
     */
    epoller(int _max = 4096, int maxevents = 2048);
    ~epoller();
    int create();
    int add(int fd, bool one_shot = true);
    int mod(int fd, int ev);
    int del(int fd);
    void setTimeout(int timeout);
    void setMaxEvents(int maxevents);
    int wait();
    const epoll_event* events() const;
    const epoll_event& operator[](int index)
    {
        return events_array[index];
    }
   int setnonblocking( int fd );// set nonblocking 

private:
    bool isValid() const;
    int max;
    int epoll_fd;
    int epoll_timeout;
    int epoll_maxevents;
    epoll_event *events_array;
};

}

#endif //EPOLLER_H
