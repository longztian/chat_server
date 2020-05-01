/*
 * File:   HandshakeParser.cpp
 * Author: Longzhang Tian
 *
 * Created on March 12, 2015, 9:18 PM
 */

#include <cctype>
#include <algorithm>

#include "HandshakeParser.hpp"
#include "Log.hpp"

namespace websocket {

HandshakeParser::HandshakeParser() :
myState(State::METHOD),
myStatus(Status::PARSING),
myMethod(Method::UNKNOWN)
{
}

HandshakeParser::Status HandshakeParser::parse(SocketInStream& in)
{
    size_t nLeft = 0, count = 0;
    const char *pData = nullptr, *pBegin = nullptr, *pEnd = nullptr;
    char* pUpper = nullptr;

    while (!in.empty() && myStatus == Status::PARSING)
    {
        nLeft = count = in.getData((const unsigned char**) &pData);
#ifdef DEBUG
        LOG_DEBUG << "in.getData = " << std::string(pData, count);
#endif

        while (nLeft > 0 && myState < State::END)
        {
            switch (myState)
            {
            case State::METHOD:
                // still unknown? find method
                if (myMethod == Method::UNKNOWN)
                {
                    // GET, case-sensitive
                    if (*pData++ == 'G')
                    {
                        myMethod = Method::GET;
                    }
                    else
                    {
                        // 400 bad request
                        LOG_ERROR << "method not allowed";
                        myStatus = Status::METHOD_NOT_ALLOWED;
                        in.clear();
                        nLeft = 0;
                    }
                    --nLeft;
                }

                // move to uri, find SPACE
                for (; nLeft > 0 && *pData != ' '; ++pData, --nLeft)
                {
                    if (*pData == '\r' || *pData == '\n')
                    {
                        // 400 bad request
                        LOG_ERROR << "bad request: unexpected end of line";
                        myStatus = Status::BAD_REQUEST;
                        in.clear();
                        nLeft = 0;
                    }
                }
                if (nLeft > 0)
                {
                    ++pData;
                    --nLeft;
                    myState = State::URI;
                }
                break;

            case State::URI:
                pBegin = pData;
                // find SPACE
                for (; nLeft > 0 && *pData != ' '; ++pData, --nLeft)
                {
                    if (*pData == '\r' || *pData == '\n')
                    {
                        // 400 bad request
                        LOG_ERROR << "bad request: unexpected end of line";
                        myStatus = Status::BAD_REQUEST;
                        in.clear();
                        nLeft = 0;
                        break;
                    }
                }

                if (myStatus == Status::PARSING)
                {
                    if (pData != pBegin)
                    {
                        myUri.append(pBegin, pData - pBegin);
                    }
                    else
                    {
                        // empty URI
                        // 400 bad request
                        LOG_ERROR << "bad request: empty URI";
                        myStatus = Status::BAD_REQUEST;
                        in.clear();
                        nLeft = 0;
                    }
                }

                if (nLeft > 0)
                {
                    ++pData;
                    --nLeft;
                    myState = State::HTTP_VERSION;
                }
                break;

            case State::HTTP_VERSION:
                pBegin = pData;
                // find CRLF
                for (; nLeft > 0 && *pData != '\r'; ++pData, --nLeft)
                {
                }
                myHttpVersion.append(pBegin, pData - pBegin);

                if (nLeft > 0)
                {
                    if (myHttpVersion == "HTTP/1.1")
                    {
                        if (nLeft >= 2)
                        {
                            pData += 2;
                            nLeft -= 2;
                            myState = State::HEADER_NAME;
                        }
                        else
                        {
                            // '\r' is at buffer end
                            myState = State::CRLF;
                            nLeft = 0;
                        }
                    }
                    else
                    {
                        // 400 bad request
                        // log bad HTTP version
                        LOG_ERROR << "HTTP version not supported";
                        myStatus = Status::HTTP_VERSION_NOT_SUPPORTED;
                        in.clear();
                        nLeft = 0;
                    }
                }
                break;

            case State::CRLF:
                // skip '\n'
                ++pData;
                --nLeft;

                if (nLeft > 0)
                {
                    if (*pData != '\r')
                    {
                        myState = State::HEADER_NAME;
                    }
                    else
                    {
                        // headers end
                        if (nLeft >= 2)
                        {
                            // pData += 2;
                            nLeft -= 2;
                            myState = State::END;
                        }
                        else
                        {
                            // '\r' is at buffer end
                            myState = State::ALMOST_END;
                            nLeft = 0;
                        }
                    }
                }
                else
                {
                    myState = State::HEADER_NAME;
                }
                break;

            case State::HEADER_NAME:
                if (*pData != '\r')
                {
                    pBegin = pData;
                    // find COLON
                    pUpper = const_cast<char*> (pData);
                    for (; nLeft > 0 && *pData != ':'; ++pData, ++pUpper, --nLeft)
                    {
                        if (*pData != '\r' && *pData != '\n')
                        {
                            // convert to upper case
                            *pUpper = ::toupper(*pData);
                        }
                        else
                        {
                            // 400 bad request
                            LOG_ERROR << "bad request: unexpected end of line";
                            myStatus = Status::BAD_REQUEST;
                            in.clear();
                            nLeft = 0;
                        }
                    }

                    if (myStatus == Status::PARSING)
                    {
                        myCurrentHeaderName.append(pBegin, pData - pBegin);
                    }

                    if (nLeft > 0)
                    {
                        ++pData;
                        --nLeft;
                        myState = State::HEADER_VALUE;
                    }
                }
                else
                {
                    // headers end
                    if (nLeft >= 2)
                    {
                        // pData += 2;
                        nLeft -= 2;
                        myState = State::END;
                    }
                    else
                    {
                        // '\r' is at buffer end
                        myState = State::ALMOST_END;
                        nLeft = 0;
                    }
                }

                break;

            case State::HEADER_VALUE:
                // skip optional white spaces
                for (; nLeft > 0 && *pData == ' '; ++pData, --nLeft)
                {
                    if (*pData == '\r' || *pData == '\n')
                    {
                        // 400 bad request
                        LOG_ERROR << "bad request: unexpected end of line";
                        myStatus = Status::BAD_REQUEST;
                        in.clear();
                        nLeft = 0;
                    }
                }

                if (myStatus == Status::PARSING)
                {
                    pBegin = pData;
                    // find CRLF
                    for (; nLeft > 0 && *pData != '\r'; ++pData, --nLeft)
                    {
                    }
                    // found value end
                    if (nLeft > 0)
                    {
                        // remove trailing white spaces
                        for (pEnd = pData; *pEnd == ' ' && pEnd > pBegin; --pEnd)
                        {
                        }

                        if (pEnd > pBegin)
                        {
                            // get some value data
                            myCurrentHeaderValue.append(pBegin, pEnd - pBegin);
                        }
                        else
                        {
                            // all white spaces, further trailing right white spaces
                            size_t length = myCurrentHeaderValue.size();
                            if (length > 0)
                            {
                                size_t found = myCurrentHeaderValue.find_last_not_of(" \t");
                                if (found != std::string::npos)
                                {
                                    if (found < length - 1)
                                    {
                                        myCurrentHeaderValue.erase(found + 1);
                                    }
                                }
                                else
                                {
                                    myCurrentHeaderValue.clear(); // all white spaces
                                }
                            }
                        }

                        if (!myCurrentHeaderValue.empty())
                        {
                            myHeaders.emplace(myCurrentHeaderName, myCurrentHeaderValue);
                        }
                        myCurrentHeaderName.clear();
                        myCurrentHeaderValue.clear();
                        if (nLeft >= 2)
                        {
                            pData += 2;
                            nLeft -= 2;
                            myState = State::HEADER_NAME;
                        }
                        else
                        {
                            // '\r' is at buffer end
                            myState = State::CRLF;
                            nLeft = 0;
                        }
                    }
                    else
                    {
                        // not found value end, just append current data
                        myCurrentHeaderValue.append(pBegin, pData - pBegin);
                    }
                }
                break;

            case State::ALMOST_END:
                // skip '\n'
                --nLeft;
                myState = State::END;
                break;

            case State::END:
                break;

            default:
                LOG_ERROR << "unsupported parsing state";
            }
        }

        if (myState == State::END)
        {
#ifdef DEBUG
            LOG_DEBUG << "in.pop_front = " << count - nLeft;
#endif
            in.pop_front(count - nLeft);
            nLeft = 0;
        }
        else
        {
            // continue to load next buffer
#ifdef DEBUG
            LOG_DEBUG << "in.pop_front = " << count;
#endif
            in.pop_front(count);
        }
    }

    if (myState == State::END)
    {
        myStatus = myValidateHeaders();
    }

    return myStatus;
}

HandshakeParser::Status HandshakeParser::myValidateHeaders()
{
#ifdef DEBUG
    for (auto iter = myHeaders.begin(), iterE = myHeaders.end(); iter != iterE; ++iter)
    {
        LOG_DEBUG << "HDR: " << iter->first << " : " << iter->second;
    }
#endif

    // validate |Host| header
    auto iter = myHeaders.find("HOST");
    if (iter == myHeaders.end())
    {
        LOG_ERROR << "|Host| header not found";
        return Status::BAD_REQUEST;
    }

    // validate |Upgrade| header
    iter = myHeaders.find("UPGRADE");
    if (iter == myHeaders.end())
    {
        LOG_ERROR << "|Upgrade| header not found";
        return Status::BAD_REQUEST;
    }
    else
    {
        // UPGRADE case-insensitive value, convert to lower case
        auto& value = iter->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        if (value.find("websocket") == value.npos)
        {
            LOG_ERROR << "|Upgrade| header does not contains \"websocket\"";
            return Status::BAD_REQUEST;
        }
    }

    // validate |Connection| header
    iter = myHeaders.find("CONNECTION");
    if (iter == myHeaders.end())
    {
        LOG_ERROR << "|Connection| header not found";
        return Status::BAD_REQUEST;
    }
    else
    {
        // UPGRADE case-insensitive value, convert to lower case
        auto& value = iter->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        if (value.find("upgrade") == value.npos)
        {
            LOG_ERROR << "|Connection| header does not contains \"Upgrade\"";
            return Status::BAD_REQUEST;
        }
    }

    // validate optional |Origin| header

    // validate |Sec-WebSocket-Version| header
    iter = myHeaders.find("SEC-WEBSOCKET-VERSION");
    if (iter == myHeaders.end())
    {
        LOG_ERROR << "|Sec-WebSocket-Version| header not found";
        return Status::BAD_REQUEST;
    }
    else
    {
        if (iter->second != "13")
        {
            LOG_ERROR << "|Sec-WebSocket-Version| header is not equal to 13";
            return Status::BAD_REQUEST;
        }
    }

    // validate |Sec-WebSocket-Key| header
    iter = myHeaders.find("SEC-WEBSOCKET-KEY");
    if (iter == myHeaders.end())
    {
        LOG_ERROR << "|Sec-WebSocket-Key| header not found";
        return Status::BAD_REQUEST;
    }
    else
    {
        // 16 bytes source base64 encoded to 24 bytes
        if (iter->second.length() != 24)
        {
            LOG_ERROR << "|Sec-WebSocket-Key| header is not 16 bytes";
            return Status::BAD_REQUEST;
        }
        else
        {
            myWebSocketKey = iter->second;
        }
    }

    // check optional |Sec-WebSocket-Protocol| header

    // check optional |Sec-WebSocket-Extensions| header

    return Status::SWITCHING_PROTOCOLS;
}

} // namespace websocket