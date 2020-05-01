/*
 * File:   ServerSocket.hpp
 * Author: Longzhang Tian
 *
 * Created on February 18, 2015, 11:59 PM
 */

#ifndef SERVERSOCKET_HPP
#define	SERVERSOCKET_HPP

#include <string>

#include "Event.hpp"
#include "ClientSocketHandler.hpp"
#include "EventLoop.hpp"

namespace websocket {

class ServerSocket
{
public:
    explicit ServerSocket(const int port, EventLoop* pEventLoop, ClientSocketHandler* pClientHander);
    explicit ServerSocket(std::string& unixSocketFile, EventLoop* pEventLoop, ClientSocketHandler* pClientHander);

    ServerSocket(const ServerSocket& other) = delete;
    ServerSocket& operator=(const ServerSocket& other) = delete;

    ServerSocket(ServerSocket&& other) = delete;
    ServerSocket& operator=(ServerSocket&& other) = delete;

    ~ServerSocket();

protected:
    void onConnect(const Event& ev);

private:
    int socket;
    EventLoop* pEventLoop;
    ClientSocketHandler* pClientHandler;
};

} // namespace websocket

#endif	/* SERVERSOCKET_HPP */

