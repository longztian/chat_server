/*
 * File:   SocketOutStream.cpp
 * Author: Longzhang Tian
 *
 * Created on March 1, 2015, 12:24 PM
 */

#include <sys/socket.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <cstring>
#include <memory>

#include "SocketOutStream.hpp"
#include "Log.hpp"

namespace websocket {

SocketOutStream::Buffer::Buffer(std::string&& str) :
data(Data(std::move(str))),
offset(0)
{
#ifdef DEBUG
    LOG_DEBUG << "buffer created with string";
#endif
}

SocketOutStream::Buffer::Buffer(std::vector<unsigned char>&& binary) :
data(Data(std::move(binary))),
offset(0)
{
#ifdef DEBUG
    LOG_DEBUG << "buffer created with binary";
#endif
}

SocketOutStream::Buffer::Buffer(Data&& data) :
data(std::move(data)),
offset(0)
{
#ifdef DEBUG
    LOG_DEBUG << "buffer created with data";
#endif
}

SocketOutStream::Buffer::Buffer(SocketOutStream::Buffer&& other) :
data(std::move(other.data)),
offset(other.offset)
{
#ifdef DEBUG
    LOG_DEBUG << "moved from " << &other;
#endif
    other.offset = 0;
    if (other.data.type == Data::Type::FILE) other.data.file = {0};

}

SocketOutStream::Buffer& SocketOutStream::Buffer::operator=(SocketOutStream::Buffer&& other)
{
#ifdef DEBUG
    LOG_DEBUG << "moved from " << &other;
#endif
    if (this != &other)
    {
        data = std::move(other.data);
        offset = other.offset;

        other.offset = 0;
        if (other.data.type == Data::Type::FILE) other.data.file = {0};
    }

    return *this;
}

SocketOutStream::Buffer::~Buffer()
{
    if (data.type == Data::Type::FILE) ::close(data.file.fd);
}

ssize_t SocketOutStream::send(const int socket)
{
    ssize_t nSend = 0, count = 0;
    size_t nLeft = 0;
    const unsigned char* pData = nullptr;

    while (!buffers.empty())
    {
        int flags = buffers.size() == 1 ? 0 : MSG_MORE;
        Buffer& buf = buffers.front();

        if (buf.data.type != Data::Type::FILE)
        {
            // process string buffer
            if (buf.data.type == Data::Type::TEXT)
            {
                pData = (const unsigned char*) buf.data.str.data() + buf.offset;
                nLeft = buf.data.str.size() - buf.offset;
            }
            else
            {
                pData = buf.data.binary.data() + buf.offset;
                nLeft = buf.data.binary.size() - buf.offset;
            }
            nSend += nLeft;
            while (nLeft > 0)
            {
                // send the string buffer
                count = ::send(socket, pData, nLeft, flags);
                if (count > 0)
                {
                    pData += count;
                    buf.offset += count;
                    nLeft -= count;
                    continue;
                }
                else
                {
                    if (count == -1)
                    {
                        if (errno == EAGAIN)
                        {
                            // no more send
                            break;
                        }
                        else
                        {
                            // send error
                            LOG_ERROR << "socket " << socket << " closed (" << strerror(errno) << ")";
                            return -1;
                        }
                    }
                    else
                    {
                        // this should not happen
                        LOG_ERROR << "socket " << socket << " send 0 bytes unexpectedly";
                        return -1;
                    }
                }
            }
        }
        else
        {
            // process file buffer
            nLeft = buf.data.file.size - buf.offset;
            nSend += nLeft;
            while (nLeft > 0)
            {
                count = ::sendfile(socket, buf.data.file.fd, (off_t*) & buf.offset, nLeft);
                if (count > 0)
                {
                    buf.offset += count;
                    nLeft -= count;
                    continue;
                }
                else
                {
                    if (count == -1)
                    {
                        if (errno == EAGAIN)
                        {
                            // no more send
                            break;
                        }
                        else
                        {
                            // send error
                            LOG_ERROR << "socket " << socket << " closed (" << strerror(errno) << ")";
                            return -1;
                        }
                    }
                    else
                    {
                        // this should not happen
                        LOG_ERROR << "socket " << socket << " send 0 bytes unexpectedly";
                        return -1;
                    }
                }
            }
        }

        if (nLeft == 0)
        {
            buffers.pop_front();
        }
        else
        {
            nSend -= nLeft;
            break;
        }
    }

#ifdef DEBUG
    LOG_DEBUG << "send " << nSend << " bytes";
#endif
    return nSend;
}

} // namespace websocket