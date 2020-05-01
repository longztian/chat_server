/*
 * File:   HandshakeParser.hpp
 * Author: Longzhang Tian
 *
 * Created on March 10, 2015, 10:02 PM
 */

#ifndef HANDSHAKEPARSER_HPP
#define	HANDSHAKEPARSER_HPP

#include <string>
#include <unordered_map>
#include <utility>

#include "SocketInStream.hpp"

namespace websocket {

class HandshakeParser
{
    using Headers = std::unordered_map<std::string, std::string>;
   
public:
    // HTTP response status
    enum class Status
    {
        PARSING = 0,
        SWITCHING_PROTOCOLS = 101,
        BAD_REQUEST = 400,
        METHOD_NOT_ALLOWED = 405,
        UPGRADE_REQUIRED = 426,
        HTTP_VERSION_NOT_SUPPORTED = 505
    };

    explicit HandshakeParser();

    HandshakeParser(const HandshakeParser& other) = delete;
    HandshakeParser& operator=(const HandshakeParser& other) = delete;

    HandshakeParser(HandshakeParser&& other) = default;
    HandshakeParser& operator=(HandshakeParser&& other) = default;

    ~HandshakeParser() = default;

    HandshakeParser::Status parse(SocketInStream& in);
   
    std::string& getUri()
    {
        return myUri;
    }
   
    std::string& getWebSocketKey()
    {
        return myWebSocketKey;
    }

private:

    enum class State
    {
        METHOD,
        URI,
        HTTP_VERSION,
        CRLF,
        HEADER_NAME,
        HEADER_VALUE,
        ALMOST_END,
        END
    };

    enum class Method
    {
        UNKNOWN,
        GET
    };
   
    Status myValidateHeaders();

    State myState;
    Status myStatus;
    Method myMethod;
    std::string myUri;
    std::string myHttpVersion;
    std::string myWebSocketKey;
    std::string myCurrentHeaderName;
    std::string myCurrentHeaderValue;
    Headers myHeaders;
};

} // namespace websocket

#endif	/* HANDSHAKEPARSER_HPP */

