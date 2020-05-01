/*
 * File:   ServerApp.cpp
 * Author: Longzhang Tian
 *
 * Created on February 20, 2015, 8:50 PM
 */

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <cstring>

#include "ServerApp.hpp"
#include "ServerSocket.hpp"
#include "EventLoop.hpp"
#include "Log.hpp"

namespace websocket {
   
ServerApp::ServerApp() :
myClientHandler(ClientSocketHandler(this)),
myMessageQueueEventFd(::eventfd(0, EFD_NONBLOCK))
{
}

void ServerApp::run()
{
    ServerSocket s(8080, &myEventLoop, &myClientHandler);

    myEventLoop.registerEvent(Event(myMessageQueueEventFd, EPOLLIN, [this](const Event & ev)
    {
        this->myMessageQueueEventHandler(ev);
    }));

    myEventLoop.run();
}

void ServerApp::myMessageQueueEventHandler(const Event & ev)
{
    // lock for out
    std::lock_guard<std::mutex> lock(myMessageQueueMutex);

    uint64_t count = 0;
    ssize_t s = ::read(ev.getFd(), &count, sizeof (count));
    if (s != sizeof (count))
    {
        LOG_ERROR << "failed to read message queue event: " << std::strerror(errno);
    }

    if (count != myMessageQueue.size())
    {
        LOG_ERROR << "message queue size error: expecting " << count << ", actual has " << myMessageQueue.size() << " messages";
    }

#ifdef DEBUG
    LOG_DEBUG << "flushing message queue";
#endif
    int i = 0, n = 0;
    int clientIDMax = myClientHandler.streams.size();
    size_t headerLength = 0;
    unsigned char header[10];

    while (!myMessageQueue.empty())
    {
        Message& msg = myMessageQueue.front();

        // build response header
        // text frame 0x81, binary frame 0x82
        header[0] = (msg.data.getType() == SocketOutStream::Data::Type::TEXT ? 0x81 : 0x82);
        size_t dataLength = msg.data.size();
        if (dataLength < 0x7E)
        {
            headerLength = 2;
            header[1] = dataLength;
        }
        else if (dataLength <= 0xFFFF)
        {
            headerLength = 4;
            header[1] = 0x7E;
            *((uint16_t*) (header + 2)) = htobe16(dataLength);
        }
        else if (dataLength <= 0x7FFFFFFFFFFFFFFF)
        {
            headerLength = 10;
            header[1] = 0x7F;
            *((uint64_t*) (header + 2)) = htobe64(dataLength);
        }
        else
        {
            // too big, not handling
            LOG_ERROR << "message data size is too big: " << dataLength;
        }

        // copy data to send out
        int clientID = 0;
        for (i = 1, n = msg.clients.size(); i < n; ++i)
        {
            clientID = msg.clients[i];
            if (clientID > 0 && clientID < clientIDMax && myClientHandler.streams[clientID].state == ClientSocketHandler::Stream::State::OPEN)
            {
                myClientHandler.streams[clientID].out.add(std::vector<unsigned char>(header, header + headerLength));
                myClientHandler.streams[clientID].out.add(SocketOutStream::Data(msg.data));
                // try send now
                myClientHandler.onSend(Event(clientID, 0, Event::dummyEventHandler));
            }
        }
        // move data to first client
        clientID = msg.clients[0];
        if (clientID > 0 && clientID < clientIDMax && myClientHandler.streams[clientID].state == ClientSocketHandler::Stream::State::OPEN)
        {
            myClientHandler.streams[clientID].out.add(std::vector<unsigned char>(header, header + headerLength));
            myClientHandler.streams[clientID].out.add(std::move(msg.data));
            // try send now
            myClientHandler.onSend(Event(clientID, 0, Event::dummyEventHandler));
        }

        // remove front
        myMessageQueue.pop_front();
    }

}

void ServerApp::close(int clientID)
{
    myClientHandler.close(clientID);
}

// copy message

void ServerApp::send(std::string msg, int clientID)
{
    mySend(std::move(msg), std::vector<int>(1, clientID));
}

void ServerApp::send(std::vector<unsigned char> msg, int clientID)
{
    mySend(std::move(msg), std::vector<int>(1, clientID));
}

void ServerApp::send(std::string msg, std::vector<int> clientIDs)
{
    if (!clientIDs.empty()) mySend(std::move(msg), std::move(clientIDs));
}

void ServerApp::send(std::vector<unsigned char> msg, std::vector<int> clientIDs)
{
    if (!clientIDs.empty()) mySend(std::move(msg), std::move(clientIDs));
}

void ServerApp::mySend(std::string&& msg, std::vector<int>&& clientIDs)
{
    std::lock_guard<std::mutex> lock(myMessageQueueMutex);
#ifdef DEBUG
    LOG_DEBUG << "sending-message appended to queue";
#endif
    myMessageQueue.push_back(Message{SocketOutStream::Data(std::move(msg)), std::move(clientIDs)});
    uint64_t count = 1;
    ssize_t s = ::write(myMessageQueueEventFd, &count, sizeof (count));
    if (s != sizeof (count))
    {
        LOG_ERROR << "failed to write message queue event: " << std::strerror(errno);
    }
}

void ServerApp::mySend(std::vector<unsigned char>&& msg, std::vector<int>&& clientIDs)
{
    std::lock_guard<std::mutex> lock(myMessageQueueMutex);
#ifdef DEBUG
    LOG_DEBUG << "sending-message appended to queue";
#endif
    myMessageQueue.push_back(Message{SocketOutStream::Data(std::move(msg)), std::move(clientIDs)});
    uint64_t count = 1;
    ssize_t s = ::write(myMessageQueueEventFd, &count, sizeof (count));
    if (s != sizeof (count))
    {
        LOG_ERROR << "failed to write message queue event: " << std::strerror(errno);
    }
}

} // namespace websocket