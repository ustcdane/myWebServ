#include "epoller.h"

namespace myWebServ
{

epoller::epoller(int _max, int maxevents):max(_max),
    epoll_fd(-1),
    epoll_timeout(-1),
    epoll_maxevents(maxevents),
    events_array(nullptr)
    {
    }

epoller::~epoller()
{
    if (isValid())
    {
        close(epoll_fd);
    }
    delete[] events_array;
}


bool epoller::isValid() const
{
    return epoll_fd > 0;
}

void epoller::setTimeout(int timeout)
{
    epoll_timeout = timeout;
}

void epoller::setMaxEvents(int maxevents)
{
    epoll_maxevents = maxevents;
}

const epoll_event* epoller::events() const
{
    return events_array;
}



int epoller::create()
{
    epoll_fd = ::epoll_create(max);
    if (isValid())
    {
        events_array = new epoll_event[epoll_maxevents];
    }
    return epoll_fd;
}

int epoller::add(int fd, bool one_shot)
{
    if (isValid())
    {
		setnonblocking(fd);
		epoll_event event={0};
		event.data.fd = fd;
		event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
		if(one_shot)
			event.events |= EPOLLONESHOT;
		return ::epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &event );
    }
    return -1;

}

int epoller::mod(int fd, int ev)
{
    if (isValid())
    {
		epoll_event event={0};
		event.data.fd = fd;
		event.events = ev|EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
        return ::epoll_ctl(epoll_fd, MOD, fd, &event);
		 
    }
    return -1;

}

int epoller::del(int fd)
{
    if (isValid())
    {
		epoll_event event={0};
        return ::epoll_ctl(epoll_fd, DEL, fd, &event);
    }
    return -1;
}

int epoller::setnonblocking( int fd )// 
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}
int epoller::wait()
{
    if (isValid())
    {
        return ::epoll_wait(epoll_fd, events_array, epoll_maxevents, epoll_timeout);
    }
    return -1;
}

}
