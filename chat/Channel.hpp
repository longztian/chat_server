/*
 * File:   Channel.hpp
 * Author: Longzhang Tian
 *
 * Created on April 3, 2015, 10:18 PM
 */

#ifndef CHANNEL_HPP
#define	CHANNEL_HPP

#include <memory>

namespace chat {

class Channel
{
public:
    explicit Channel();

    Channel(const Channel& other) = delete;
    Channel& operator=(const Channel& other) = delete;

    Channel(Channel&& other);
    Channel& operator=(Channel&& other);

    ~Channel();
   
    void removeUser(int socket)
    {
       
    }

private:
    int id;
    std::vector<int> users;
};

} // namespace chat

#endif	/* CHANNEL_HPP */

