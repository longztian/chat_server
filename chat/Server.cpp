/*
 * File:   Server.cpp
 * Author: Longzhang Tian
 *
 * Created on March 31, 2015, 10:10 PM
 */

#include <algorithm>

#include "Server.hpp"
#include "Log.hpp"
#include "json/json.h"
#include "Channel.hpp"

namespace chat {

void Server::onOpen(int clientID)
{
    LOG_INFO << "client connected " << clientID;
}

void Server::onClose(int clientID)
{
    LOG_INFO << "client disconnected " << clientID;
    if (clientID > 0 && clientID < (int) users.size())
    {
        // notify channel;
        for (int cid : users[clientID].channelIDs)
        {
            // remove user from channel
            auto& channelClientIDs = channelUsers[cid];
            auto i = std::find(channelClientIDs.begin(), channelClientIDs.end(), clientID);
            if (i != channelClientIDs.end())
            {
                channelClientIDs.erase(i);
            }

            // notify channel subscribers
            // let's no do this, because this is useless info
            /*
            Json::Value msg(Json::objectValue), data(Json::objectValue);
            data["userID"] = users[clientID].id;
            data["channelID"] = pChannel->id;
            msg["userOffline"] = data;
             */

            // close single-user channel
            if (channelClientIDs.size() < 2)
            {
                channelUsers.erase(cid);
            }
        }

        // delete user;
        users[clientID].reset();
    }
}

void Server::onMessage(std::string msg, int clientID)
{
    Json::Value json;
    Json::Reader reader;
    LOG_INFO << "raw: " << msg;
    if (reader.parse(msg, json, false))
    {
        if (json.isObject())
        {
            // can be multiple functions in a message
            // {"login":{"username":<username>,"password":<password>}}
            std::vector<std::string> unhandledEvents;
            for (auto& func : json.getMemberNames())
            {
                auto data = json[func];
                auto iHandlers = eventHandlers.find(func);
                if (iHandlers != eventHandlers.end())
                {
                    for (auto& handler : iHandlers->second)
                    {
                        handler(data);
                    }
                }
                else
                {
                    unhandledEvents.push_back(func);
                }
            }

            if (!unhandledEvents.empty())
            {
                std::string error("unknown event [");
                for (auto& func : unhandledEvents)
                {
                    error.append(func).append(", ");
                }
                error.erase(error.size() - 2).push_back(']');

                Json::Value msg(Json::objectValue);
                msg["error"] = error;
                Json::FastWriter fwriter;
                send(fwriter.write(msg), clientID);
            }
        }
        else
        {
            send(std::string("{\"error\":\"invalid message format\"}"), clientID);
        }
    }
    else
    {
        Json::Value msg(Json::objectValue);
        msg["error"] = reader.getFormattedErrorMessages();
        Json::FastWriter fwriter;
        send(fwriter.write(msg), clientID);
    }
    /*
    std::vector<int>::iterator i = std::find(clients.begin(), clients.end(), clientID);
    if (i == clients.end())
    {
        // first message to get the username
        clients.push_back(clientID);
        names.push_back(msg);
        std::string outMsg("user connected: ");
        outMsg.append(msg);
        send(outMsg, clients);
    }
    else
    {
        // chatting messages
        int index = std::distance(clients.begin(), i);
        std::string outMsg(names[index]);
        outMsg.append(" says: ");
        outMsg.append(msg);

        std::vector<int> toClients;
        toClients.insert(toClients.begin(), clients.begin(), i);
        toClients.insert(toClients.begin() + index, ++i, clients.end());

        if (!toClients.empty()) send(outMsg, toClients);
    }
     */
}

void Server::onMessage(std::vector<unsigned char> msg, int clientID)
{
    LOG_ERROR << "not implemented";
}

void Server::init()
{
    bind("login", [this](Json::Value & data)
    {
        LOG_INFO << "login " << data.asString();
          users.push_back(User());
          users.back().id = users.size() - 1;
          users.back().channelIDs.push_back(0);
          channelUsers[0].push_back(users.back().id);
    });

    bind("message", [](Json::Value & data)
    {
        LOG_INFO << "message " << data.asString();
    });

    // create a common channel
    channelUsers[0] = std::vector<int>();
}

void Server::bind(std::string event, handler_t handler)
{
    LOG_INFO << "bind " << event;
    eventHandlers[event].push_back(std::move(handler));
}


} // namespace chat