/*
 * File:   ServerSocket.cpp
 * Author: Longzhang Tian
 *
 * Created on February 19, 2015, 12:00 AM
 */
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

#include "ServerSocket.hpp"
#include "Log.hpp"
#include "Exception.hpp"

namespace
{
#define BACKLOG 120

    void myClose(const int socket)
    {
        if (socket != -1)
        {
            // get socket address
            sockaddr_storage addr = {0};
            socklen_t addrLen = sizeof (addr);
            if (::getsockname(socket, (sockaddr *) & addr, &addrLen) == 0)
            {
                if (addr.ss_family == AF_UNIX)
                {
                    // close socket
                    ::close(socket);
                    // remove socket file
                    ::unlink(((sockaddr_un*) & addr)->sun_path);
                }
                else
                {
                    // close socket
                    ::close(socket);
                }
            }
            else
            {
                // just close socket anyway
                ::close(socket);
            }
        }
    }

    int myCreate(const int port)
    {
        int socket = -1;

        addrinfo * pAI = NULL, * pAIL = NULL, hints = {0};
        hints.ai_family = AF_UNSPEC; /* Return IPv4 and IPv6 choices */
        hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
        hints.ai_flags = AI_PASSIVE; /* All interfaces */

        int status = ::getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &pAIL);

        if (status != 0)
        {
            // free linked list
            ::freeaddrinfo(pAIL);
            throw Exception(::gai_strerror(status));
        }

        int i = 0;
        std::string error;

        for (pAI = pAIL; pAI != NULL; pAI = pAI->ai_next)
        {
            ++i;

            socket = ::socket(pAI->ai_family, pAI->ai_socktype | SOCK_NONBLOCK, pAI->ai_protocol);
            if (socket == -1)
            {
                continue;
            }

            int reuseaddr = 1;
            if (::setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const void *) &reuseaddr, sizeof reuseaddr) == 0)
            {
                if (::bind(socket, pAI->ai_addr, pAI->ai_addrlen) == 0)
                {
                    if (::listen(socket, BACKLOG) == 0)
                    {
                        break;
                    }
                    else
                    {
                        error.append("listen: ").append(std::strerror(errno));
                    }
                }
                else
                {
                    error.append("bind: ").append(std::strerror(errno));
                }
            }
            else
            {
                error.append("reuse address: ").append(std::strerror(errno));
            }
            error.append(1, '\n');

            // close failed socket
            ::close(socket);
        }

        // free linked list
        ::freeaddrinfo(pAIL);

        if (pAI == NULL)
        {
            throw Exception(std::string("tried ").append(std::to_string(i)).append(" addresses. error:\n").append(error));
        }

        return socket;
    }

    int myCreate(const std::string& file)
    {
        int socket = -1;

        // if empty
        if (file.empty())
        {
            throw Exception("Unix socket filename is empty");
        }

        // if too long
        if (file.size() >= 108)
        {
            throw Exception(std::string("Unix socket filename is too long. name=").append(file));
        }

        // if not absolute path
        if (file.at(0) != '/')
        {
            throw Exception(std::string("UNIX socket file need to be an absolute path. file=").append(file));
        }

        sockaddr_un addrUnix = {0};
        addrUnix.sun_family = AF_UNIX;
        std::strcpy(addrUnix.sun_path, file.c_str());
        socklen_t addrLen = sizeof (addrUnix.sun_family) + file.size();

        socket = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (socket == -1)
        {
            throw Exception(std::strerror(errno));
        }

        int reuseaddr = 1;
        if (::setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const void *) &reuseaddr, sizeof reuseaddr) != 0)
        {
            std::string error = std::strerror(errno);
            ::close(socket);
            ::unlink(file.c_str());
            throw Exception(error);
        }

        // server listen state
        if (::bind(socket, (sockaddr *) & addrUnix, addrLen) != 0)
        {
            std::string error = std::strerror(errno);
            ::close(socket);
            ::unlink(file.c_str());
            throw Exception(error);
        }

        // start listening to socket
        if (::listen(socket, BACKLOG) != 0)
        {
            std::string error = std::strerror(errno);
            ::close(socket);
            ::unlink(file.c_str());
            throw Exception(error);
        }

        return socket;
    }
}

namespace websocket {

ServerSocket::ServerSocket(const int port, EventLoop* pEventLoop, ClientSocketHandler* pClientHander) :
pEventLoop(pEventLoop),
pClientHandler(pClientHander)
{
    socket = myCreate(port);
#ifdef DEBUG
    LOG_DEBUG << "socket " << socket << " [port=" << port << "]";
#endif

    if (socket == -1)
    {
        throw Exception(std::string("failed to bind to port: ").append(std::to_string(port)));
    }

    Event connectEvent(socket, EPOLLIN, [this](const Event & ev)
    {
        this->onConnect(ev);
    });

    if (!pEventLoop->registerEvent(connectEvent))
    {
        throw Exception("failed to register connect event handler for server socket");
    }
}

ServerSocket::ServerSocket(std::string& unixSocketFile, EventLoop* pEventLoop, ClientSocketHandler* pClientHander) :
pEventLoop(pEventLoop),
pClientHandler(pClientHander)
{
    socket = myCreate(unixSocketFile);
#ifdef DEBUG
    LOG_DEBUG << "socket " << socket << " [file=" << unixSocketFile << "]";
#endif

    if (socket == -1)
    {
        throw Exception(std::string("failed to bind to UNIX socket: ").append(unixSocketFile));
    }

    Event connectEvent(socket, EPOLLIN, [this](const Event & ev)
    {
        this->onConnect(ev);
    });

    if (!pEventLoop->registerEvent(connectEvent))
    {
        throw Exception("failed to register connect event handler for server socket");
    }
}

ServerSocket::~ServerSocket()
{
#ifdef DEBUG
    LOG_DEBUG << "destroyed, " << "close socket " << socket;
#endif
    myClose(socket);
    delete pClientHandler;
}

void ServerSocket::onConnect(const Event& ev)
{
    /* We have a notification on the listening socket, which
     * means one or more incoming connections.
     */
#define HOSTSIZE 40 // 39 charactors for IPv6
#define SERVSIZE 6 // unsigned short

    sockaddr addrClient = {0};
    socklen_t addrLen = sizeof addrClient;

    char host[HOSTSIZE] = {0}, service[SERVSIZE] = {0};
    int status = -1;

    while (true)
    {
        addrClient = {0};
        addrLen = sizeof addrClient;

        int client = ::accept4(socket, &addrClient, &addrLen, SOCK_NONBLOCK);
        if (client == -1)
        {
            if (errno == EAGAIN)
            {
                /* We have processed all incoming connections. */
            }
            else
            {
                LOG_WARN << std::strerror(errno);
            }

            // no clients connected, return
            break;
        }

#ifdef DEBUG
        LOG_DEBUG << "CONNECT: client @ " << client;
#endif

        if (addrClient.sa_family == AF_INET || addrClient.sa_family == AF_INET6)
        {
            int nodelay = 1;
            if (::setsockopt(client, SOL_TCP, TCP_NODELAY, (const void *) &nodelay, sizeof nodelay) != 0)
            {
                std::string error = std::strerror(errno);
                ::close(client);
                throw Exception(error);
            }

            std::memset(host, 0, HOSTSIZE);
            std::memset(service, 0, SERVSIZE);

            status = ::getnameinfo(&addrClient, addrLen,
                                    host, HOSTSIZE,
                                    service, SERVSIZE,
                                    NI_NUMERICHOST | NI_NUMERICSERV);
            if (status == 0)
            {
#ifdef DEBUG
                LOG_DEBUG << "client from " << host << ":" << service;
#endif
            }
            else
            {
                LOG_WARN << ::gai_strerror(status);
            }
        }

        if (!pClientHandler->add(client))
        {
#ifdef DEBUG
            LOG_DEBUG << "DISCONNECT: client @ " << socket;
#endif

            // log error here
            ::close(client);
        }
    }
}

} // namespace websocket