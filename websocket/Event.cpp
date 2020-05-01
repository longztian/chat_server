/*
 * File:   Event.cpp
 * Author: Longzhang Tian
 *
 * Created on February 17, 2015, 10:20 PM
 */

#include <sys/epoll.h>
#include <utility>

#include "Event.hpp"
#include "Log.hpp"

namespace websocket {

Event::handler_t Event::dummyEventHandler = Event::handler_t();

Event::Event(const int fd, const uint32_t events, handler_t handler) :
fd(fd),
events(events | EPOLLRDHUP | EPOLLET),
handler(std::move(handler))
{
#ifdef DEBUG
    LOG_DEBUG << "created";
#endif
}

Event::Event(Event&& other) :
fd(other.fd),
events(other.events),
handler(std::move(other.handler))
{
#ifdef DEBUG
    LOG_DEBUG << "moved from " << &other;
#endif
}

Event& Event::operator=(Event&& other)
{
#ifdef DEBUG
    LOG_DEBUG << "moved from " << &other;
#endif
    if (this != &other)
    {
        fd = other.fd;
        events = other.events;
        handler = std::move(other.handler);
    }

    return *this;
}

Event::~Event()
{
#ifdef DEBUG
    LOG_DEBUG << "destroyed";
#endif
}

int Event::getFd() const
{
    return fd;
}

bool Event::isIn() const
{
    return events & EPOLLIN;
}

bool Event::isOut() const
{
    return events & EPOLLOUT;
}

bool Event::isError() const
{
    return (events & EPOLLRDHUP) || (events & EPOLLERR) || (events & EPOLLHUP);
}

} // namespace websocket