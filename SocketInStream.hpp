/* 
 * File:   SocketInStream.hpp
 * Author: ikki
 *
 * Created on March 1, 2015, 12:24 PM
 */

#ifndef SOCKETINSTREAM_HPP
#define	SOCKETINSTREAM_HPP

#include <deque>
#include <string>

class SocketInStream
{
public:
    explicit SocketInStream();

    SocketInStream(const SocketInStream& other) = delete;
    SocketInStream& operator=(const SocketInStream& other) = delete;

    SocketInStream(SocketInStream&& other);
    SocketInStream& operator=(SocketInStream&& other);

    ~SocketInStream();
    
    ssize_t recv(const int socket);
    
    std::string get();
    
private:
    class Buffer;
    std::deque<Buffer> buffers;
};

#endif	/* SOCKETINSTREAM_HPP */
