#pragma once

#include "Account_UI.h"
#include <iostream>
#include <string>
#include <limits>
#include <sys/sendfile.h> //发送文件
#include <sys/stat.h> //查看路径状态
#include <fcntl.h> //操作文件

extern std::unordered_map<std::string, int> notice_map;
extern bool confirmed_as_friend;
extern bool confirmed_as_block_friend;
extern bool confirm_in_group;

struct len_name {
	unsigned long len;
	char name[1024];
};

void home_UI(int connecting_sockfd, std::string UID);
void message_UI(int connecting_sockfd, std::string UID);
void recv_file_UI(int connecting_sockfd, std::string UID);
void recv_friend_file(int connecting_sockfd, std::string UID, std::string friend_UID, std::string file_name);
void recv_group_file(int connecting_sockfd, std::string UID, std::string GID, std::string file_name);
void contacts_UI(int connecting_sockfd, std::string UID);
void friends_UI(int connecting_sockfd, std::string UID);
void friend_details_UI(int connecting_sockfd, std::string UID, std::string friend_UID);
void private_chat(int connecting_sockfd, std::string UID, std::string friend_UID);
void send_friend_file(int connecting_sockfd, std::string UID, std::string friend_UID);
void groups_UI(int connecting_sockfd, std::string UID);
void group_details_UI(int connecting_sockfd, std::string UID, std::string GID);
void group_chat(int connecting_sockfd, std::string UID, std::string GID);
void send_group_file(int connecting_sockfd, std::string UID, std::string GID);
void add_friends_groups_UI(int connecting_sockfd, std::string UID);
void add_friend_UI(int connecting_sockfd, std::string UID);
void add_group_UI(int connecting_sockfd, std::string UID);
void friends_request_UI(int connecting_sockfd, std::string UID);
void groups_request_UI(int connecting_sockfd, std::string UID);
void create_group_UI(int connecting_sockfd, std::string UID);
bool user_UI(int connecting_sockfd, std::string UID);
void information_UI(int connecting_sockfd, std::string UID);
void username_UI(int connecting_sockfd, std::string UID);
void security_question_UI(int connecting_sockfd, std::string UID);
void waiting_for_input();