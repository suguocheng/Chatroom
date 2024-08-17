#pragma once

#include <iostream>
#include <string>
#include <nlohmann/json.hpp> //JSON库
#include <sys/socket.h> //套接字连接

using json = nlohmann::json;

int send_json(int connecting_sockfd, const json& j);
// int receive_json(int sockfd, json& j);
