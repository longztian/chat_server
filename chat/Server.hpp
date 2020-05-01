/*
 * File:   Server.hpp
 * Author: Longzhang Tian
 *
 * Created on March 31, 2015, 10:09 PM
 */

#ifndef SERVER_HPP
#define	SERVER_HPP

#include <unordered_map>
#include <string>
#include <memory>

#include "json/json.h"

#include "websocket/ServerApp.hpp"

namespace chat {

class Server : public websocket::ServerApp
{
    using handler_t = std::function<void(Json::Value&) >;
public:
    Server() = default;

    Server(const Server& other) = delete;
    Server& operator=(const Server& other) = delete;

    Server(Server&& other) = delete;
    Server& operator=(Server&& other) = delete;

    ~Server() = default;

    void init();
    void bind(std::string event, handler_t handler);

    virtual void onOpen(int clientID) override;
    virtual void onClose(int clientID) override;
    virtual void onMessage(std::string msg, int clientID) override;
    virtual void onMessage(std::vector<unsigned char> msg, int clientID) override;

private:
    using ChannelUserMap = std::unordered_map<int, std::vector<int>>;

    class User
    {
    public:
        int id;
        std::vector<int> channelIDs;

        void reset()
        {
            id = 0;
            channelIDs.clear();
        }
    };

    std::vector<User> users;
   
    ChannelUserMap channelUsers;
    std::unordered_map<std::string, std::vector<handler_t>> eventHandlers;
};

} // namespace chat

#endif	/* SERVER_HPP */

