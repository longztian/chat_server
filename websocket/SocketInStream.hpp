/*
 * File:   SocketInStream.hpp
 * Author: Longzhang Tian
 *
 * Created on March 1, 2015, 12:24 PM
 */

#ifndef SOCKETINSTREAM_HPP
#define	SOCKETINSTREAM_HPP

#include <deque>
#include <memory>

namespace websocket {

class SocketInStream
{
public:

    SocketInStream() :
    mySize(0)
    {
    }

    SocketInStream(const SocketInStream& other) = delete;
    SocketInStream& operator=(const SocketInStream& other) = delete;

    SocketInStream(SocketInStream&& other) :
    mySize(other.mySize),
    buffers(std::move(other.buffers))
    {
        other.mySize = 0;
    }

    SocketInStream& operator=(SocketInStream&& other)
    {
        if (this != &other)
        {
            mySize = other.mySize;
            other.mySize = 0;
            buffers = std::move(other.buffers);
        }
        return *this;
    }

    ~SocketInStream() = default;

    size_t size()
    {
        return mySize;
    }

    bool empty()
    {
        return mySize == 0;
    }

    void clear()
    {
        buffers.clear();
        mySize = 0;
    }

    ssize_t recv(const int socket);
    size_t getData(const unsigned char** ppBuffer);
    void pop_front(size_t count);
    bool extract(unsigned char* buf, size_t count);
    bool maskExtract(unsigned char* buf, size_t count, const unsigned char* mask, size_t maskCount, size_t maskStart);

private:

    class Buffer
    {
    public:

        Buffer();
        Buffer(const Buffer& other) = delete;
        Buffer& operator=(const Buffer& other) = delete;
        Buffer(Buffer&& other);
        Buffer& operator=(Buffer&& other);
        ~Buffer();

        size_t getDataSize() const;
        size_t getFreeCapacity() const;
        void pop_front(size_t count);
        void push_back(size_t count);
        size_t getReadableBuffer(const unsigned char** ppBuf) const;
        size_t getWritableBuffer(unsigned char** ppBuf) const;
        void clear();

    private:
        static std::deque<std::unique_ptr<unsigned char[]>> pool;

        unsigned char* pData;
        unsigned char* pBegin;
        unsigned char* pEnd;
        // use Int32 position instead
        // int32_t  dataOffset;
        // int32_t  spaceOffset;
    };

    size_t mySize;
    std::deque<Buffer> buffers;
};

} // namespace websocket

#endif	/* SOCKETINSTREAM_HPP */

