/*
 * File:   User.hpp
 * Author: Longzhang Tian
 *
 * Created on April 3, 2015, 10:22 PM
 */

#ifndef USER_HPP
#define	USER_HPP

#include <memory>

#include "Channel.hpp"

namespace chat {

class User
{
public:
    explicit User();

    User(const User& other) = delete;
    User& operator=(const User& other) = delete;

    User(User&& other);
    User& operator=(User&& other);

    ~User() = default;
   
    void reset()
    {
        id = 0;
        name.clear();
        channels.clear();
    }

private:
    std::vector<std::shared_ptr<Channel>> channels;
   
    unsigned int id;
    std::string name;
};

} // namespace chat

#endif	/* USER_HPP */

