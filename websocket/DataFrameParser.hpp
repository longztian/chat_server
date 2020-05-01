/*
 * File:   DataFrameParser.hpp
 * Author: Longzhang Tian
 *
 * Created on March 10, 2015, 10:02 PM
 */

#ifndef DATAFRAMEPARSER_HPP
#define	DATAFRAMEPARSER_HPP

#include <memory>
#include "SocketInStream.hpp"

#define WS_OPCODE_CONTINUATION 0
#define WS_OPCODE_TEXT         1
#define WS_OPCODE_BINARY       2
#define WS_OPCODE_CLOSE        8
#define WS_OPCODE_PING         9
#define WS_OPCODE_PONG         10

namespace websocket {

class DataFrameParser
{
public:

    struct Header
    {
        unsigned int fin : 1;
        unsigned int rsv1 : 1;
        unsigned int rsv2 : 1;
        unsigned int rsv3 : 1;
        unsigned int opcode : 4;
        unsigned int mask : 1;
        unsigned int payload : 7;
    };

    enum class Status
    {
        PARSING,
        SUCCESS,
        UNRELEASED_DATA,
        BAD_MASK_BIT,
        BAD_PAYLOAD_LENGTH
    };

    explicit DataFrameParser();

    DataFrameParser(const DataFrameParser& other) = delete;
    DataFrameParser& operator=(const DataFrameParser& other) = delete;

    DataFrameParser(DataFrameParser&& other);
    DataFrameParser& operator=(DataFrameParser&& other);

    ~DataFrameParser() = default;

    Status parse(SocketInStream& in);
   
    size_t getData(std::unique_ptr<unsigned char[]>& pData);
   
    Header& getHeader()
    {
        return myHeader;
    }


private:

    enum class State
    {
        HEADER,
        PAYLOAD,
        MASK,
        DATA,
        END
    };
   
    State myState;
    Header myHeader;
    size_t myPayloadLength;
    unsigned char myMask[4];
    std::unique_ptr<unsigned char[] > myData;
    size_t myDataLength;
};

} // namespace websocket

#endif	/* DATAFRAMEPARSER_HPP */

