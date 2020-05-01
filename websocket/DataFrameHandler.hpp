/*
 * File:   DataFrameHandler.hpp
 * Author: Longzhang Tian
 *
 * Created on March 11, 2015, 7:53 PM
 */

#ifndef DATAFRAMEHANDLER_HPP
#define	DATAFRAMEHANDLER_HPP

#include <vector>

#include "MessageHandler.hpp"
#include "DataFrameParser.hpp"
#include "ServerApp.hpp"

namespace websocket {

class DataFrameHandler : public MessageHandler
{
public:

    explicit DataFrameHandler(ServerApp* pServerApp, int clientID) :
    pServerApp(pServerApp),
    myClientID(clientID),
    myMessageType(Type::NONE)
    {
    }

    DataFrameHandler(const DataFrameHandler& other) = delete;
    DataFrameHandler& operator=(const DataFrameHandler& other) = delete;

    DataFrameHandler(DataFrameHandler&& other) = delete;
    DataFrameHandler& operator=(DataFrameHandler&& other) = delete;

    virtual ~DataFrameHandler() = default;

    virtual Status process(SocketInStream& in, SocketOutStream& out) override;

private:
    bool myHandleMessage(unsigned int type);
    bool myHandleFragmentFrame(SocketOutStream& out);
    void myHandleBadMaskBit(SocketOutStream& out);
    void myHandleBadPayloadLength(SocketOutStream& out);

    void mySendCloseFrame(SocketOutStream& out);
    void mySendPongFrame(SocketOutStream& out);

    enum class Type
    {
        NONE,
        TEXT,
        BINARY
    };

    ServerApp* pServerApp;
    int myClientID;

    DataFrameParser myParser;
    Type myMessageType;
    std::string myMessageText;
    std::vector<unsigned char> myMessageBinary;


};

} // namespace websocket

#endif	/* DATAFRAMEHANDLER_HPP */

