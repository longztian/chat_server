/*
 * File:   DataFrameParser.cpp
 * Author: Longzhang Tian
 *
 * Created on March 12, 2015, 9:27 PM
 */

#include <endian.h>
#include <cstring>

#include "DataFrameParser.hpp"
#include "Log.hpp"

namespace websocket {

namespace
{

    template<typename T>
    void myExtract(T& dest, const unsigned char** ppData, size_t& nLeft, SocketInStream& in, size_t& nPop, size_t& nTotal)
    {
        // load a new buffer?
        if (nLeft == 0)
        {
            // pop till current unread position
            if (nPop)
            {
                in.pop_front(nPop);
                nPop = 0;
            }
            // load a new buffer
            nLeft = in.getData(ppData);
            nTotal = in.size();
        }

        size_t count = sizeof (T);
        if (nLeft >= count)
        {
            dest = **((T**) ppData);
            *ppData += count;
            nLeft -= count;
            nPop += count;
            nTotal -= count;
        }
        else
        {
            // current buffer doesn't have enough bytes
            // pop till current unread position
            if (nPop)
            {
                in.pop_front(nPop);
                nPop = 0;
            }

            // extract directly from inStream
            in.extract((unsigned char*) &dest, count);
            // get updated steam size
            nTotal = in.size();
            // invalidate current buffer
            nLeft = 0;
        }
    }

    void myExtract(unsigned char* pDest, size_t count, const unsigned char** ppData, size_t& nLeft, SocketInStream& in, size_t& nPop, size_t& nTotal)
    {
        // load a new buffer?
        if (nLeft == 0)
        {
            // pop till current unread position
            if (nPop)
            {
                in.pop_front(nPop);
                nPop = 0;
            }
            // load a new buffer
            nLeft = in.getData(ppData);
            nTotal = in.size();
        }

        if (nLeft >= count)
        {
            std::memcpy(pDest, *ppData, count);
            *ppData += count;
            nLeft -= count;
            nPop += count;
            nTotal -= count;
        }
        else
        {
            // current buffer doesn't have enough bytes
            // pop till current unread position
            if (nPop)
            {
                in.pop_front(nPop);
                nPop = 0;
            }

            // extract directly from inStream
            in.extract(pDest, count);
            // get updated steam size
            nTotal = in.size();
            // invalidate current buffer
            nLeft = 0;
        }
    }
}

DataFrameParser::DataFrameParser() :
myState(State::HEADER),
myData(nullptr),
myDataLength(0)
{
}

DataFrameParser::DataFrameParser(DataFrameParser&& other) :
myState(other.myState),
myHeader(other.myHeader),
myPayloadLength(other.myPayloadLength),
myData(std::move(other.myData)),
myDataLength(other.myDataLength)
{
    std::memcpy(myMask, other.myMask, sizeof (unsigned char)*4);
}

DataFrameParser& DataFrameParser::operator=(DataFrameParser&& other)
{
    if (this != &other)
    {
        myState = other.myState;
        myHeader = other.myHeader;
        myPayloadLength = other.myPayloadLength;
        std::memcpy(myMask, other.myMask, sizeof (unsigned char)*4);
        myData = std::move(other.myData);
        myDataLength = other.myDataLength;
    }
    return *this;
}

DataFrameParser::Status DataFrameParser::parse(SocketInStream& in)
{
    // start over from previous data frame
    if (myState == State::END)
    {
        if (myDataLength == 0)
        {
            // start over again for a new data frame
            myState = State::HEADER;
        }
        else
        {
            // we get unreleased data
            LOG_ERROR << "cannot further parse IN stream before releasing existing data (" << myDataLength << " bytes)";
            return Status::UNRELEASED_DATA;
        }
    }

    // nothing to parse
    if (in.empty())
    {
        return Status::PARSING;
    }

    size_t nLeft = 0, nPop = 0;
    const unsigned char *pData = nullptr;

    size_t nTotal = in.size();

    if (myState == State::HEADER)
    {
        // we need at least 2 bytes to process
        if (nTotal >= 2)
        {
            unsigned char header[2];
            myExtract(header, 2, &pData, nLeft, in, nPop, nTotal);
            // parse header
            myHeader.fin = (header[0] & 0x80) >> 7;
            myHeader.rsv1 = (header[0] & 0x40) >> 6;
            myHeader.rsv1 = (header[0] & 0x20) >> 5;
            myHeader.rsv1 = (header[0] & 0x10) >> 4;
            myHeader.opcode = (header[0] & 0x0F);
            myHeader.mask = (header[1] & 0x80) >> 7;
            myHeader.payload = (header[1] & 0x7F);

            // validate mask bit
            if (!myHeader.mask)
            {
                LOG_ERROR << "missing mask bit";
                myState = State::END;
                in.clear();
                return Status::BAD_MASK_BIT;
            }
        }
        else
        {
            return Status::PARSING;
        }
        myState = State::PAYLOAD;
    }

    if (myState == State::PAYLOAD)
    {
        // check payload;
        if (myHeader.payload <= 125)
        {
            // <= 125
            // 7 bits
            myPayloadLength = myHeader.payload;
        }
        else if (myHeader.payload == 126)
        {
            // 126
            // 7 + 16 bits, need to check the next 2 bytes
            if (nTotal >= 2)
            {
                uint16_t length = 0;
                myExtract(length, &pData, nLeft, in, nPop, nTotal);
                myPayloadLength = be16toh(length);

                if (myPayloadLength < 126)
                {
                    LOG_ERROR << "bad payload length: expect 0x7E [0x007E, 0xFFFF], get " << myPayloadLength;
                    myState = State::END;
                    in.clear();
                    return Status::BAD_PAYLOAD_LENGTH;
                }
            }
            else
            {
                return Status::PARSING;
            }
        }
        else
        {
            // 127
            // 7 + 64 bits, need to check the next 8 bytes
            if (nTotal >= 8)
            {
                uint64_t length;
                myExtract(length, &pData, nLeft, in, nPop, nTotal);
                myPayloadLength = be64toh(length);

                if (myPayloadLength < 0x0000000000010000 || myPayloadLength > 0x7FFFFFFFFFFFFFFF)
                {
                    LOG_ERROR << "bad payload length: expect 0x7F [0x0000000000010000, 0x7FFFFFFFFFFFFFFF], get " << myPayloadLength;
                    myState = State::END;
                    in.clear();
                    return Status::BAD_PAYLOAD_LENGTH;
                }
            }
            else
            {
                return Status::PARSING;
            }
        }
        myState = State::MASK;
    }

    if (myState == State::MASK)
    {
        // we need 4 bytes to process
        if (nTotal >= 4)
        {
            myExtract(myMask, 4, &pData, nLeft, in, nPop, nTotal);
        }
        else
        {
            return Status::PARSING;
        }
        myState = State::DATA;
    }

    // reading data
    if (myState == State::DATA)
    {
        // pop parsed headers
        if (nPop > 0)
        {
            in.pop_front(nPop);
            nTotal = in.size();
        }

        // no application in data frame
        if (myPayloadLength == 0)
        {
            myState = State::END;
            return Status::SUCCESS;
        }

        // first time to parse data
        if (myDataLength == 0)
        {
            myData = std::unique_ptr<unsigned char[]>(new unsigned char[myPayloadLength]);
        }

        size_t nToRead = myPayloadLength - myDataLength;
        if (nTotal >= nToRead)
        {
            in.maskExtract(myData.get() + myDataLength, nToRead, myMask, 4, myDataLength % 4);
            myDataLength += nToRead;
            myState = State::END;
            return Status::SUCCESS;
        }
        else
        {
            in.maskExtract(myData.get() + myDataLength, nTotal, myMask, 4, myDataLength % 4);
            myDataLength += nTotal;
            return Status::PARSING;
        }
    }
   
    return Status::PARSING;
}

size_t DataFrameParser::getData(std::unique_ptr<unsigned char[]>& pData)
{
    if (myState == State::END && myDataLength > 0)
    {
        pData = std::move(myData);
        size_t count = myDataLength;
        myDataLength = 0;

        return count;
    }
    else
    {
        pData = nullptr;
        return 0;
    }
}

} // namespace websocket