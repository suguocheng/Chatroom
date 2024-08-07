#pragma once

#include "Account_UI.h"
#include <iostream>
#include <string>
#include <limits>

extern std::unordered_map<std::string, int> notice_map;

void home_UI(int connecting_sockfd, std::string UID);
void message_UI(int connecting_sockfd, std::string UID);
void contacts_UI(int connecting_sockfd, std::string UID);
void friends_UI(int connecting_sockfd, std::string UID);
void friend_details(int connecting_sockfd, std::string UID);
void groups_UI(int connecting_sockfd, std::string UID);
void add_friends_groups_UI(int connecting_sockfd, std::string UID);
void add_friend_UI(int connecting_sockfd, std::string UID);
void add_group_UI(int connecting_sockfd, std::string UID);
void friends_request_UI(int connecting_sockfd, std::string UID);
void groups_request_UI(int connecting_sockfd, std::string UID);
bool user_UI(int connecting_sockfd, std::string UID);
void information_UI(int connecting_sockfd, std::string UID);
void username_UI(int connecting_sockfd, std::string UID);
void security_question_UI(int connecting_sockfd, std::string UID);
void waiting_for_input();