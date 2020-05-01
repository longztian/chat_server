/*
 * File:   Log.hpp
 * Author: Longzhang Tian
 *
 * Created on February 16, 2015, 10:05 PM
 */

#ifndef LOG_HPP
#define	LOG_HPP

#include <time.h>
#include <sstream>
#include <iostream>
#include <iomanip>

#ifdef DEBUG
#define LOG_DEBUG if(true) Log::Debug(this, __PRETTY_FUNCTION__, __FILE__, __LINE__)
#define LOG_SDEBUG if(true) Log::Debug(nullptr, __PRETTY_FUNCTION__, __FILE__, __LINE__)
#endif

#define LOG_INFO if(true) Log::Info()
#define LOG_WARN if(true) Log::Warn()
#define LOG_ERROR if(true) Log::Error()

namespace Log {

// private base class

class Log
{
protected:

    explicit Log(std::ostream& os) :
    os(os)
    {
        //  Get the time
        timespec ts = {0};
        ::clock_gettime(CLOCK_REALTIME_COARSE, &ts);
        struct tm tm;
        ::localtime_r(&ts.tv_sec, &tm);

        ss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":" << std::setw(2) << tm.tm_min << ":" << std::setw(2) << tm.tm_sec << "." << std::setw(3) << (int) (ts.tv_nsec / 1000000) << ' ';
    }

    Log() = delete;
    Log(const Log& other) = delete;
    Log& operator=(const Log& other) = delete;

    Log(Log&& other) = delete;
    Log& operator=(Log&& other) = delete;

    ~Log()
    {
        os << ss.rdbuf() << std::endl;
    }

    // properties
    std::ostream& os;
    std::stringstream ss;
};

#ifdef DEBUG
class Debug : public Log
{
public:

    explicit Debug(const void* object, const char* function, const char* file, int line) :
    Log(std::cout)
    {
        //  Output the main info
        if (object)
        {
            ss << function << " [" << file << ':' << line << " " << object << "] ";
        }
        else
        {
            ss << function << " [" << file << ':' << line << "] ";
        }
    }

    template<typename T> Debug& operator<<(T const& t)
    {
        //   Accumulate into a non-shared stringstream, no threading issues
        ss << t;
        return *this;
    }

    Debug(const Debug& other) = delete;
};
#endif

class Info : public Log
{
public:

    explicit Info() :
    Log(std::cout)
    {
    }

    template<typename T> Info& operator<<(T const& t)
    {
        ss << t;
        return *this;
    }

    Info(const Info& other) = delete;
};

class Warn : public Log
{
public:

    explicit Warn() :
    Log(std::cout)
    {
    }

    template<typename T> Warn& operator<<(T const& t)
    {
        ss << t;
        return *this;
    }

    Warn(const Info& Warn) = delete;
};

class Error : public Log
{
public:

    explicit Error() :
    Log(std::cout)
    {
    }

    template<typename T> Error& operator<<(T const& t)
    {
        ss << t;
        return *this;
    }

    Error(const Error& other) = delete;
};
}


#endif	/* LOG_HPP */

