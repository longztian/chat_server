/*
 * File:   ServerApp.hpp
 * Author: Longzhang Tian
 *
 * Created on February 20, 2015, 8:50 PM
 */

#ifndef SERVERAPP_HPP
#define	SERVERAPP_HPP

#include <vector>
#include <deque>
#include <string>
#include <mutex>

#include "Event.hpp"
#include "EventLoop.hpp"
#include "ClientSocketHandler.hpp"

namespace websocket {

class ServerApp
{
    friend class ClientSocketHandler;
public:

    ServerApp();

    ServerApp(const ServerApp& other) = delete;
    ServerApp& operator=(const ServerApp& other) = delete;

    ServerApp(ServerApp&& other) = delete;
    ServerApp& operator=(ServerApp&& other) = delete;

    ~ServerApp() = default;

    // event handler
    virtual void onOpen(int clientID) = 0;
    virtual void onClose(int clientID) = 0;
    virtual void onMessage(std::string msg, int clientID) = 0;
    virtual void onMessage(std::vector<unsigned char> msg, int clientID) = 0;

    // copy message
    virtual void send(std::string msg, int clientID) final;
    virtual void send(std::vector<unsigned char> msg, int clientID) final;
    virtual void send(std::string msg, std::vector<int> clientIDs) final;
    virtual void send(std::vector<unsigned char> msg, std::vector<int> clientIDs) final;

    // close client
    virtual void close(int clientID) final;

    // start server
    virtual void run() final;

private:

    void myMessageQueueEventHandler(const Event & ev);
    void mySend(std::string&& msg, std::vector<int>&& clientIDs);
    void mySend(std::vector<unsigned char>&& msg, std::vector<int>&& clientIDs);

    EventLoop myEventLoop;
    ClientSocketHandler myClientHandler;
    int myMessageQueueEventFd;

    class Message
    {
    public:
        SocketOutStream::Data data;
        std::vector<int> clients;
    };
    std::deque<Message> myMessageQueue;

    std::mutex myMessageQueueMutex;
};

} // namespace websocket

#endif	/* WEBSOCKETSERVERAPP_HPP */

