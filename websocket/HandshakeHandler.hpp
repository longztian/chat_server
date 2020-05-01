/*
 * File:   HandshakeHandler.hpp
 * Author: Longzhang Tian
 *
 * Created on March 11, 2015, 7:49 PM
 */

#ifndef HANDSHAKEHANDLER_HPP
#define	HANDSHAKEHANDLER_HPP

#include "MessageHandler.hpp"
#include "HandshakeParser.hpp"

namespace websocket {

class HandshakeHandler : public MessageHandler
{
public:
    explicit HandshakeHandler() = default;

    HandshakeHandler(const HandshakeHandler& other) = delete;
    HandshakeHandler& operator=(const HandshakeHandler& other) = delete;

    HandshakeHandler(HandshakeHandler&& other) = delete;
    HandshakeHandler& operator=(HandshakeHandler&& other) = delete;

    virtual ~HandshakeHandler() = default;

    virtual Status process(SocketInStream& in, SocketOutStream& out) override;

private:
    void myHandleSwitchingProtocols(SocketOutStream& out);
    void myHandleBadRequest(SocketOutStream& out);
    void myHandleMethodNotAllowed(SocketOutStream& out);
    void myHandleUpgradeRequired(SocketOutStream& out);
    void myHandleHttpVersionNotSupported(SocketOutStream& out);

    HandshakeParser myParser;
};

} // namespace websocket

#endif	/* HANDSHAKEHANDLER_HPP */

