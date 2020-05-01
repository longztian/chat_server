/*
 * File:   SocketDataHandler.cpp
 * Author: Longzhang Tian
 *
 * Created on February 20, 2015, 11:25 PM
 */

#include <sys/epoll.h>
#include <sys/socket.h>

#include "ClientSocketHandler.hpp"
#include "ServerApp.hpp"
#include "Log.hpp"
#include "HandshakeHandler.hpp"
#include "DataFrameHandler.hpp"

namespace websocket {
   
void ClientSocketHandler::Stream::init()
{
    state = State::CONNECTING;
    handler = std::unique_ptr<MessageHandler>(dynamic_cast<MessageHandler*> (new HandshakeHandler()));
}

void ClientSocketHandler::Stream::open(ServerApp* pServerApp, int socket)
{
    // OPENING: handshake finished
    state = State::OPEN;
    handler = std::unique_ptr<MessageHandler>(dynamic_cast<MessageHandler*> (new DataFrameHandler(pServerApp, socket)));
}

void ClientSocketHandler::Stream::close()
{
    if (state != State::CLOSED)
    {
        state = State::CLOSED;
        handler = nullptr;
        in.clear();
        out.clear();
    }
}

ClientSocketHandler::ClientSocketHandler(ServerApp* pServerApp) :
pServerApp(pServerApp)
{
#ifdef DEBUG
    LOG_DEBUG << "created";
#endif
}

ClientSocketHandler::ClientSocketHandler(ClientSocketHandler&& other) :
streams(std::move(other.streams))
{
#ifdef DEBUG
    LOG_DEBUG << "moved from " << &other;
#endif
}

ClientSocketHandler& ClientSocketHandler::operator=(ClientSocketHandler&& other)
{
#ifdef DEBUG
    LOG_DEBUG << "moved from " << &other;
#endif
    if (this != &other)
    {
        streams = std::move(other.streams);
    }
    return *this;
}

ClientSocketHandler::~ClientSocketHandler()
{
#ifdef DEBUG
    LOG_DEBUG << "destroyed";
#endif
}

bool ClientSocketHandler::add(int socket)
{
    if (socket < 0) return false;

    // fill streams till the socket element
    if (streams.size() < static_cast<unsigned int> (socket + 1))
    {
        streams.resize(socket + 1);
    }
    else
    {
        // close existing stream
        streams[socket].close();
    }

    Event dataEvent(socket, EPOLLIN | EPOLLOUT, [this](const Event & ev)
    {
        if (ev.isError())
        {
            this->onError(ev);
            return;
        }

        if (ev.isIn())
        {
            this->onRecv(ev);
        }

        if (ev.isOut())
        {
            this->onSend(ev);
        }
    });

    return pServerApp->myEventLoop.registerEvent(dataEvent);
}

void ClientSocketHandler::close(int socket)
{
    if (socket > 0)
    {
        pServerApp->myEventLoop.unregisterEvent(Event(socket, 0, Event::dummyEventHandler));
        if (static_cast<unsigned int> (socket) < streams.size()) streams[socket].close();
    }
}

void ClientSocketHandler::onError(const Event& ev)
{
    int socket = ev.getFd();
#ifdef DEBUG
    LOG_DEBUG << "socket=" << socket;
#endif

    // explicitly unregister for non error event
    if (!ev.isError())
    {
        pServerApp->myEventLoop.unregisterEvent(ev);
    }

    if(streams[socket].state == Stream::State::OPEN) pServerApp->onClose(socket);
   
    streams[socket].close();   
}

void ClientSocketHandler::onRecv(const Event& ev)
{
    int socket = ev.getFd();
#ifdef DEBUG
    LOG_DEBUG << "socket=" << socket;
#endif

    auto& stream = streams[socket];

    if (stream.state == Stream::State::CLOSING)
    {
        // closing, do not process any further message, just close connection
        onError(ev);
        return;
    }

    // read data
    ssize_t count = stream.in.recv(socket);

    if (count > 0)
    {
        // stream.process(pServerApp, socket);
        // process
        if (stream.state == Stream::State::CLOSED)
        {
            stream.init();
        }

        // process message for CONNECTING and OPEN state
        switch (stream.handler->process(stream.in, stream.out))
        {
        case MessageHandler::Status::SUCCESS:
            if (stream.state == Stream::State::CONNECTING)
            {
                stream.open(pServerApp, socket);
                pServerApp->onOpen(socket);
                // just finished handshake, connection is OPEN
                // there should be no data received further before OPEN, otherwise clear
                if (!stream.in.empty()) stream.in.clear();

                // there should be a HTTP UPGRADE response, otherwise close
                if (stream.out.empty()) onError(ev);
            }
            break;

        case MessageHandler::Status::PARSING:
            break;

        case MessageHandler::Status::ERROR:
            if (stream.state == Stream::State::OPEN)
            {
                // CONNECTED: got a closing frame or an invalid frame
                // response a close frame

                // first need to check whether we get a pending response
                // pServerApp->sendPendingMessage(socket);

                if (stream.out.empty())
                {
                    // just close if nothing to send
                    onError(ev);
                }
                else
                {
                    // will call close() after send()
                    stream.state = Stream::State::CLOSING;
                }
            }
            break;
        }

        // have something to send, try sending NOW
        if (!stream.out.empty())
        {
            if (stream.out.send(socket) < 0)
            {
                onError(ev);
            }

            // close CLOSING stream, if flush the out stream
            if (stream.out.empty() && stream.state == Stream::State::CLOSING)
            {
                onError(ev);
            }
        }
    }
    else if (count < 0)
    {
        onError(ev);
    }
}

void ClientSocketHandler::onSend(const Event& ev)
{
    int socket = ev.getFd();
#ifdef DEBUG
    LOG_DEBUG << "socket=" << socket;
#endif

    auto& stream = streams[socket];
    if (!stream.out.empty())
    {
        if (stream.out.send(socket) < 0)
        {
            onError(ev);
        }

        // close CLOSING stream, if flush the out stream
        if (stream.out.empty() && stream.state == Stream::State::CLOSING)
        {
            onError(ev);
        }
    }
}

} // namespace websocket