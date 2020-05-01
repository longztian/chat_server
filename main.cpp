/*
 * File:   main.cpp
 * Author: Longzhang Tian
 *
 * Created on February 16, 2015, 10:04 PM
 */

#include <string>
#include <algorithm>

#include "chat/Server.hpp"

int main(int argc, char** argv)
{
    chat::Server server;
    server.init();
    server.run();
    return 0;
}

