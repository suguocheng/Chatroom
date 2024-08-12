#pragma once

#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <vector>


class RedisManager {
public:
    RedisManager();
    ~RedisManager();

    std::string generate_UID();
    std::string generate_GID();
    void initialize_UID_counter();
    void initialize_GID_counter();

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

    bool add_private_chat_message(const std::string& UID, const std::string& friend_UID, const std::string& message);
    bool get_private_chat_messages(const std::string& UID, const std::string& friend_UID, std::vector<std::string>& messages);

    bool add_group(const std::string& group_name, const std::string& group_owner_UID);
    std::string get_group_name(const std::string& GID);
    std::string get_group_owner_UID(const std::string& GID);
    bool modify_group_name(const std::string& GID, const std::string& new_group_name);
    bool modify_group_owner_UID(const std::string& GID, const std::string& new_group_owner_UID);
    bool delete_group(const std::string& GID);

    bool add_group_member(const std::string& GID, const std::string& request_UID);
    bool get_groups(const std::string& UID, std::vector<std::string>& groups_GID);
    bool get_group_members(const std::string& GID, std::vector<std::string>& members_UID);
    bool delete_group_member(const std::string& GID, const std::string& member_UID);

    bool add_administrator(const std::string& GID, const std::string& member_UID);
    bool check_administrator(const std::string& GID, const std::string& member_UID);
    bool get_administrators(const std::string& GID, std::vector<std::string>& administrators_UID);
    bool delete_administrator(const std::string& GID, const std::string& member_UID);

    bool add_group_chat_message(const std::string& GID, const std::string& member_UID, const std::string& message);
    bool get_group_chat_messages(const std::string& GID, const std::string& member_UID, std::vector<std::string>& messages);

private:
    redisContext* redisContext_;
};