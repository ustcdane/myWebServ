#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

namespace myWebServ
{

/**
 *  ��epoll�ķ�װ
 */
class epoller
{
public:
    enum EPOLL_OP {ADD = EPOLL_CTL_ADD, MOD = EPOLL_CTL_MOD, DEL = EPOLL_CTL_DEL};
    /**
     * ���������������Ļش��¼���
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
