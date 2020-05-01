/*
 * File:   ClientSocketHandler.hpp
 * Author: Longzhang Tian
 *
 * Created on February 20, 2015, 11:19 PM
 */

#ifndef CLIENTSOCKETHANDLER_HPP
#define	CLIENTSOCKETHANDLER_HPP

#include <vector>
#include <memory>

#include "Event.hpp"
#include "EventLoop.hpp"
#include "SocketInStream.hpp"
#include "SocketOutStream.hpp"
#include "MessageHandler.hpp"

namespace websocket {

class ServerApp;

class ClientSocketHandler
{
    friend class ServerApp;
public:
    explicit ClientSocketHandler(ServerApp* pServerApp);

    ClientSocketHandler(const ClientSocketHandler& other) = delete;
    ClientSocketHandler& operator=(const ClientSocketHandler& other) = delete;

    ClientSocketHandler(ClientSocketHandler&& other);
    ClientSocketHandler& operator=(ClientSocketHandler&& other);

    ~ClientSocketHandler();

    bool add(int socket);
    void close(int socket);

protected:
    void onError(const Event& ev);
    void onRecv(const Event& ev);
    void onSend(const Event& ev);

private:

    class Stream
    {
    public:

        enum class State
        {
            CONNECTING,
            OPEN,
            CLOSING,
            CLOSED
        };

        Stream() :
        state(State::CLOSED),
        handler(nullptr)
        {
        }

        Stream(const Stream& other) = delete;
        Stream& operator=(const Stream& other) = delete;
        Stream(Stream&& other) = default;
        Stream& operator=(Stream&& other) = default;
        ~Stream() = default;

        void init();
        void open(ServerApp* pServerApp, int socket);
        void close();

        SocketInStream in;
        SocketOutStream out;
        State state;
        std::unique_ptr<MessageHandler> handler;
    };
   
    std::vector<Stream> streams;
    ServerApp* pServerApp;
};

} // namespace websocket

#endif	/* CLIENTSOCKETHANDLER_HPP */

