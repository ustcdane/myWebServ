// Copyright (c) 2014 Daniel Wang. All rights reserved.
// https://github.com/ustcdane/myWebServ
// Use of this source code is governed by a BSD-style license
// that can be found in the License file
// Author: Daniel Wang(daneustc at gmail dot com)

#include "eventLoop.h"

namespace myWebServ
{
    // close connection
    void eventLoop::close_con(int fd)
    {
		auto it=map_fd_http.find(fd);
        if( it != map_fd_http.end())
        {
            delete map_fd_http[fd];
            map_fd_http.erase(it);
        }
    }

    void eventLoop::loop()
    {
        if(isValid())
        {
            int nums;
            running = true;
            while(running)
            {
                nums = pEpoll->wait();
                int i;
                for(i=0; i<nums; ++i)
                {
                    int sockfd = (*pEpoll)[i].data.fd;
                    if( sockfd == event_fd ) //
                    {
                        int con_items=0;
                        std::vector<int> vec;
                        vec.reserve(8);
                        {
                            uint64_t u;
                            read(event_fd, &u, sizeof(uint64_t));
                            std::lock_guard<std::mutex> lg(mx);
                            while(!fd_con_queue.empty())// 把连接取出来
                            {
                                int fd = fd_con_queue.front();
                                fd_con_queue.pop();
                                vec.push_back(fd);
                            }// while
                        }// mutex

                        //初始化连接
                        for(auto it=vec.begin(); it!=vec.end(); ++it)
                        {
                            // 未释放的连接
                            assert(map_fd_http.find(*it)==map_fd_http.end());
                            /// **************
                            map_fd_http[*it] = new httpConn(*it, pEpoll, this);
							pEpoll->add(*it);// add event
                        }
                    }// if
                    else if((*pEpoll)[i].events & EPOLLIN)
                    {
                        assert(map_fd_http.find(sockfd)!=map_fd_http.end());
                        if(map_fd_http[sockfd]->read())
                        {
                            map_fd_http[sockfd]->process();
                        }
                        else
                        {
                            close_con(sockfd);
                        }
                    }
                    else if((*pEpoll)[i].events & EPOLLOUT)
                    {
                        assert(map_fd_http.find(sockfd)!=map_fd_http.end());
                        if(!map_fd_http[sockfd]->write())
                        {
                            close_con(sockfd);
                        }
                    }
                    else if((*pEpoll)[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) )
                    {
                        close_con(sockfd);
                    }
                }// for

            }// while
        }// if
		running = false;
    }// loop
}
