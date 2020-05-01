/*
 * File:   Exception.hpp
 * Author: Longzhang Tian
 *
 * Created on February 16, 2015, 10:45 PM
 */

#ifndef EXCEPTION_HPP
#define	EXCEPTION_HPP

#include <exception>
#include <string>

class Exception : public std::exception
{
public:

    explicit Exception(const std::string& what) :
    reason(what)
    {
    }

    explicit Exception(std::string&& what) :
    reason(what)
    {
    }

    explicit Exception(const char* what) :
    reason(what)
    {
    }

    virtual const char* what() const noexcept
    {
        return reason.c_str();
    }

    Exception(const Exception& other) = delete;
    Exception& operator=(const Exception& other) = delete;

    Exception(Exception&& other) = default;
    Exception& operator=(Exception&& other) = default;

    ~Exception() = default;

private:
    std::string reason;
};

#endif	/* EXCEPTION_HPP */

