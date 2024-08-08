#pragma once

#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <vector>


class RedisManager {
public:
    RedisManager();
    ~RedisManager();

    std::string generateUniqueId();
    void initializeCounter();

    bool add_user(const std::string& username, const std::string& password, const std::string& security_question, const std::string& security_answer);
    std::string get_UID(const std::string& username);
    std::string get_username(const std::string& UID);
    std::string get_password(const std::string& username);
    std::string get_security_question(const std::string& username);
    std::string get_security_answer(const std::string& username);
    bool modify_username(const std::string& UID, const std::string& new_username);
    bool modify_password(const std::string& UID, const std::string& new_password);
    bool modify_security_question(const std::string& UID, const std::string& new_security_question, const std::string& new_security_answer);
    bool delete_user(const std::string& UID);

    bool add_notification(const std::string& UID, const std::string& notification_type, const std::string& notification);
    bool get_notification(const std::string& UID, const std::string& notification_type, std::vector<std::string>& notifications);
    bool delete_notification(const std::string& UID, const std::string& notification_type, const std::string& notification);

    bool add_friend(const std::string& UID, const std::string& request_UID);
    bool get_friends(const std::string& UID, std::vector<std::string>& friends_UID);
    bool modify_friend();
    bool delete_friend(const std::string& UID, const std::string& friend_UID);

    bool add_block_friend(const std::string& UID, const std::string& friend_UID);
    bool check_block_friend(const std::string& UID, const std::string& friend_UID);
    bool delete_block_friend(const std::string& UID, const std::string& friend_UID);

    bool add_group(const std::string& userId, const std::string& groupId);
    bool get_group();
    bool modify_group();
    bool delete_group();

    bool add_chat_message(const std::string& userId, const std::string& friendId, const std::string& message);
    bool get_chat_message();

    bool add_group_chat_message(const std::string& groupId, const std::string& message);
    bool get_group_chat_message();

private:
    redisContext* redisContext_;
};