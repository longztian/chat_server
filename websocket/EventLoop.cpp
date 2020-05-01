/*
 * File:   EventLoop.cpp
 * Author: Longzhang Tian
 *
 * Created on February 17, 2015, 10:44 PM
 */

#include <sys/epoll.h>
#include <unistd.h>
#include <utility>
#include <cstring>

#include "EventLoop.hpp"
#include "Exception.hpp"
#include "Log.hpp"

namespace websocket {

EventLoop::EventLoop() :
fd(::epoll_create1(0)),
currentEventFd(0)
{
    if (fd == -1)
    {
        throw Exception(std::strerror(errno));
    }
}

EventLoop::~EventLoop()
{
#ifdef DEBUG
    LOG_DEBUG << "destroyed";
#endif
}

bool EventLoop::registerEvent(const Event& ev) noexcept
{
    if (currentEventFd > 0 && currentEventFd == ev.fd)
    {
        LOG_ERROR << "trying to register an existing event which will overwrite the current event. fd=" << currentEventFd;
        return false;
    }

    // will overwrite previously bund event if any
    int efd = ev.fd;
    auto iter = events.find(efd);
    if (iter == events.end())
    {
        iter = events.emplace(efd, std::move(*(const_cast<Event*> (&ev)))).first;
    }
    else
    {
        iter->second = std::move(*(const_cast<Event*> (&ev)));
    }

    Event& evo = iter->second;

    epoll_event epev = {0};
    // do we really want EDGE on listening socket?
    epev.data.ptr = (void*) &evo;
    epev.events = evo.events;

    int status = ::epoll_ctl(fd, EPOLL_CTL_ADD, evo.fd, &epev);

    if (status == -1)
    {
        LOG_ERROR << std::strerror(errno);
        return false;
    }

    return true;
}

bool EventLoop::unregisterEvent(const Event& ev) noexcept
{
#ifdef DEBUG
    LOG_DEBUG << "unregister event " << ev.fd;
#endif
    epoll_event epev = {0};
    ::epoll_ctl(fd, EPOLL_CTL_DEL, ev.fd, &epev);

    // delete the event object
    if (ev.fd != currentEventFd)
    {
        ::close(ev.fd);
        events.erase(ev.fd);
    }
    else
    {
        // currently being processed event, deferred delete, just remove all events
        auto& evo = *(const_cast<Event*> (&ev));
        evo.events = 0;
    }

    return true;
}

void EventLoop::run() noexcept
{
#define MAXEVENTS 64

    if (fd == -1)
    {
        LOG_ERROR << "no event loop available";
        return;
    }

    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS] = {0};

    int i, n;
    while (true)
    {
        n = ::epoll_wait(fd, events, MAXEVENTS, -1);

        // do we really want to stop here?
        if (n == -1)
        {
            if (errno == EINTR)
            {
                // interrupted by a signal handler
                // log here
                // signal handler?
#ifdef DEBUG
                LOG_DEBUG << "signal interrupted";
#endif
            }
            else
            {
                LOG_ERROR << std::strerror(errno);
            }

            continue;
        }

        for (i = 0; i < n; ++i)
        {
            epoll_event& epev = events[i];
            // get event object
            Event& evo = *(static_cast<Event*> (epev.data.ptr));
            evo.events = epev.events;
#ifdef DEBUG
            LOG_DEBUG << "get event: " << evo.fd << " [" << evo.events << "]";
#endif

            // unregister error event
            if (!evo.isError())
            {
                // mark this as the current being processed event
                currentEventFd = evo.fd;
                // process the event
                (evo.handler)(evo);
                // unmark this as the current being processed event
                currentEventFd = 0;

                // see if we need to finish a deferred unregister
                if (!evo.events)
                {
                    // no events, a deferred unregister event, unregister now
                    unregisterEvent(evo);
                }
            }
            else
            {
                // process error event
                // create an error event object
                Event errorEv(evo.fd, EPOLLERR, evo.handler);
                // unregister current event
                unregisterEvent(evo);

                // process the error event
                (errorEv.handler)(errorEv);
            }
        }
    }
}

void EventLoop::stop()
{
    ::close(fd);
    fd = -1;
}

} // namespace websocket