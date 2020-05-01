/*
 * File:   HandshakeHandler.cpp
 * Author: Longzhang Tian
 *
 * Created on March 12, 2015, 9:27 PM
 */

#include "HandshakeHandler.hpp"
#include "Log.hpp"
#include "sha1/sha1.hpp"
#include "base64/base64.hpp"

namespace websocket {

MessageHandler::Status HandshakeHandler::process(SocketInStream& in, SocketOutStream& out)
{
#ifdef DEBUG
    LOG_DEBUG << "processing";
#endif
    HandshakeParser::Status status = myParser.parse(in);
    switch (status)
    {
    case HandshakeParser::Status::SWITCHING_PROTOCOLS:
        myHandleSwitchingProtocols(out);
        return Status::SUCCESS;
        break;

    case HandshakeParser::Status::BAD_REQUEST:
        myHandleBadRequest(out);
        break;

    case HandshakeParser::Status::METHOD_NOT_ALLOWED:
        myHandleMethodNotAllowed(out);
        break;

    case HandshakeParser::Status::UPGRADE_REQUIRED:
        myHandleUpgradeRequired(out);
        break;

    case HandshakeParser::Status::HTTP_VERSION_NOT_SUPPORTED:
        myHandleHttpVersionNotSupported(out);
        break;

    case HandshakeParser::Status::PARSING:
        return Status::PARSING;
        break;

    default:
        LOG_ERROR << "unsupported status";
    }
   
    return Status::ERROR;
}

void HandshakeHandler::myHandleSwitchingProtocols(SocketOutStream& out)
{
    std::string key = myParser.getWebSocketKey();
    key.append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    std::string response = std::string("HTTP/1.1 101 Switching Protocols\r\n"
                                        "Upgrade: websocket\r\n"
                                        "Connection: Upgrade\r\n"
                                        "Sec-WebSocket-Accept: ");

#define SHA1_LEN 20
#define BASE64_LEN 28

    unsigned char sha1sum[SHA1_LEN];
    sha1::hash((unsigned char*) key.c_str(), key.size(), sha1sum);

    char base64[BASE64_LEN];
    size_t count = base64::encode((const char*) sha1sum, 20, base64);
    if (count != BASE64_LEN)
    {
        LOG_ERROR << "expecting base64-encoding Sec-WebSocket-Accept hash string has length " << BASE64_LEN << " get " << count;
    }
    response.append(base64, BASE64_LEN).append("\r\n\r\n");
    out.add(std::move(response));
}

void HandshakeHandler::myHandleBadRequest(SocketOutStream& out)
{
    out.add("HTTP/1.1 400 Bad Request\r\n\r\n");
}

void HandshakeHandler::myHandleMethodNotAllowed(SocketOutStream& out)
{
    out.add("HTTP/1.1 405 Method Not Allowed\r\n\r\n");
}

void HandshakeHandler::myHandleUpgradeRequired(SocketOutStream& out)
{
    out.add("HTTP/1.1 426 Upgrade Required\r\n\r\n");
}

void HandshakeHandler::myHandleHttpVersionNotSupported(SocketOutStream& out)
{
    out.add("HTTP/1.1 505 HTTP Version Not Supported\r\n\r\n");
}

} // namespace websocket