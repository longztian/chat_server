/* 
 * File:   Event.hpp
 * Author: ikki
 *
 * Created on February 17, 2015, 10:15 PM
 */

#ifndef EVENT_HPP
#define	EVENT_HPP

#include <functional>

class Event
{
    friend class EventLoop;
    using handler_t = std::function<void(const Event&) >;

public:
    explicit Event(const int fd, const uint32_t events, handler_t handler);

    Event(const Event& other) = delete;
    Event& operator=(const Event& other) = delete;

    Event(Event&& other);
    Event& operator=(Event&& other);

    ~Event();

    int getFd() const;
    bool isIn() const;
    bool isOut() const;
    bool isError() const;

private:
    int fd;
    uint32_t events;
    handler_t handler;
};

#endif	/* EVENT_HPP */

