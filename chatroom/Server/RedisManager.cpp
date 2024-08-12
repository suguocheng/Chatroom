#include "RedisManager.h"
#include "../log/mars_logger.h"

RedisManager::RedisManager() {
    redisContext_ = redisConnect("127.0.0.1", 6379);
    if (redisContext_ == NULL || redisContext_->err) {
        if (redisContext_) {
            std::cerr << "连接 Redis 失败: " << redisContext_->errstr << std::endl;
            redisFree(redisContext_);
        } else {
            std::cerr << "无法分配 Redis 上下文" << std::endl;
        }
    }
    LogInfo("redis已启动!");

    //初始化id计数器
    initialize_UID_counter();
    initialize_GID_counter();
}

RedisManager::~RedisManager() {
    if (redisContext_) {
        redisFree(redisContext_);
    }
}

std::string RedisManager::generate_UID() {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "INCR UID_counter");

    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return " ";
    }

    std::string UID = std::to_string(reply->integer);
    // LogInfo("UID1 = {}", UID);
    freeReplyObject(reply);
    return UID;
}

std::string RedisManager::generate_GID() {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "INCR GID_counter");

    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return " ";
    }

    std::string GID = std::to_string(reply->integer);
    // LogInfo("UID1 = {}", UID);
    freeReplyObject(reply);
    return GID;
}

void RedisManager::initialize_UID_counter() {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SETNX UID_counter 0");
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
    }
    freeReplyObject(reply);
}

void RedisManager::initialize_GID_counter() {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SETNX GID_counter 0");
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
    }
    freeReplyObject(reply);
}

bool RedisManager::add_user(const std::string& username, const std::string& password, const std::string& security_question, const std::string& security_answer) {
    std::string UID = generate_UID();
    if (UID.empty()) {
        return 0;
    }

    //检查用户名是否存在
    if (get_UID(username) != "") {
        std::cout << "该用户名已被注册！" << std::endl;
        return 0;
    }

    //用户名映射UID
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HSET username_to_UID %s %s", username.c_str(), UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    //UID映射用户名
    redisReply* reply1 = (redisReply*)redisCommand(redisContext_, "HSET UID_to_username %s %s", UID.c_str(), username.c_str());
    if (reply1 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    //UID映射用户信息
    redisReply* reply2 = (redisReply*)redisCommand(redisContext_, "HMSET user:%s password %s security_question %s security_answer %s", UID.c_str(), password.c_str(), security_question.c_str(), security_answer.c_str());
    if (reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);
    freeReplyObject(reply1);
    freeReplyObject(reply2);
    
    std::cout << "注册成功！" << std::endl;
    return 1;
}

std::string RedisManager::get_UID(const std::string& username) {
    std::string UID;
    // LogInfo("username = {}", username.c_str());

    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HGET username_to_UID %s", username.c_str());
    
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return "";
    }
    
    if (reply->type != REDIS_REPLY_STRING) {
        std::cout << "用户名不存在！" << std::endl;
        return "";
    } else {
        UID = reply->str;
    }

    // LogInfo("UID = {}", UID);
    freeReplyObject(reply);

    return UID;
}

std::string RedisManager::get_username(const std::string& UID) {
    std::string username;

    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HGET UID_to_username %s", UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return "";
    }

    if (reply->type != REDIS_REPLY_STRING) {
        std::cout << "用户不存在！" << std::endl;
        return "";

    } else {
        username = reply->str;
    }

    freeReplyObject(reply);

    return username;
}

std::string RedisManager::get_password(const std::string& username) {
    std::string password;

    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HGET user:%s password", get_UID(username).c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return "";
    }

    if (reply->type != REDIS_REPLY_STRING) {
        std::cout << "用户名不存在！" << std::endl;
        return "";

    } else {
        password = reply->str;
    }

    freeReplyObject(reply);

    return password;
}

std::string RedisManager::get_security_question(const std::string& username) {
    std::string security_question;

    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HGET user:%s security_question", get_UID(username).c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return "";
    }

    if (reply->type != REDIS_REPLY_STRING) {
        std::cout << "用户名不存在！" << std::endl;
        return "";

    } else {
        security_question = reply->str;
    }

    freeReplyObject(reply);

    return security_question;
}

std::string RedisManager::get_security_answer(const std::string& username) {
    std::string security_answer;

    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HGET user:%s security_answer", get_UID(username).c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return "";
    }

    if (reply->type != REDIS_REPLY_STRING) {
        std::cout << "用户名不存在！" << std::endl;
        return "";

    } else {
        security_answer = reply->str;
    }

    freeReplyObject(reply);

    return security_answer;
}

bool RedisManager::modify_username(const std::string& UID, const std::string& new_username) {
    std::string old_username = get_username(UID);

    redisReply* del_reply = (redisReply*)redisCommand(redisContext_, "HDEL username_to_UID %s", old_username.c_str());
    if (del_reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* del_reply2 = (redisReply*)redisCommand(redisContext_, "HDEL UID_to_username %s", UID.c_str());
    if (del_reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* set_reply = (redisReply*)redisCommand(redisContext_, "HSET username_to_UID %s %s", new_username.c_str(), UID.c_str());
    if (set_reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* set_reply2 = (redisReply*)redisCommand(redisContext_, "HSET UID_to_username %s %s", UID.c_str(), new_username.c_str());
    if (set_reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(del_reply);
    freeReplyObject(del_reply2);
    freeReplyObject(set_reply);
    freeReplyObject(set_reply2);

    return 1;
}

bool RedisManager::modify_password(const std::string& UID, const std::string& new_password) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HSET user:%s password %s", UID.c_str(), new_password.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);

    return 1;
}

bool RedisManager::modify_security_question(const std::string& UID, const std::string& new_security_question, const std::string& new_security_answer) {
    redisReply* reply1 = (redisReply*)redisCommand(redisContext_, "HSET user:%s security_question %s", UID.c_str(), new_security_question.c_str());
    if (reply1 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* reply2 = (redisReply*)redisCommand(redisContext_, "HSET user:%s security_answer %s", UID.c_str(), new_security_answer.c_str());
    if (reply1 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply1);
    freeReplyObject(reply2);

    return 1;
}

bool RedisManager::delete_user(const std::string& UID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "DEL user:%s", UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* reply1 = (redisReply*)redisCommand(redisContext_, "HDEL username_to_UID %s", get_username(UID).c_str());
    if (reply1 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* reply2 = (redisReply*)redisCommand(redisContext_, "HDEL UID_to_username %s", UID.c_str());
    if (reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);
    freeReplyObject(reply1);
    freeReplyObject(reply2);

    return 1;
}

bool RedisManager::add_notification(const std::string& UID, const std::string& notification_type, const std::string& notification) {
    std::string key;

    if (notification_type == "friend_request") {
        key = "friend_request_notifications:" + UID;
    } else if (notification_type == "group_request") {
        key = "group_request_notifications:" + UID;
    } else if (notification_type == "message") {
        key = "message_notifications:" + UID;
    } else {
        std::cerr << "Unknown notification type" << std::endl;
        return 0;
    }

    redisReply* reply = (redisReply*) redisCommand(redisContext_, "LPUSH %s %s", key.c_str(), notification.c_str());

    if (reply == NULL) {
        std::cerr << "Error: " << redisContext_->errstr << std::endl;
        return 0;
    }
    freeReplyObject(reply);
    
    return 1;
    
}


bool RedisManager::get_notification(const std::string& UID, const std::string& notification_type, std::vector<std::string>& notifications) {
    std::string key;

    if (notification_type == "friend_request") {
        key = "friend_request_notifications:" + UID;
    } else if (notification_type == "group_request") {
        key = "group_request_notifications:" + UID;
    } else if (notification_type == "message") {
        key = "message_notifications:" + UID;
    } else {
        std::cerr << "Unknown notification type" << std::endl;
        return 0;
    }
    // LogInfo("exe reply");
    redisReply *reply = (redisReply*) redisCommand(redisContext_, "LRANGE %s 0 -1", key.c_str());
    // LogInfo("reply end");
    if (reply == NULL) {
        std::cerr << "Error: " << redisContext_->errstr << std::endl;
        return 0;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        std::cout << "用户不存在！" << std::endl;
        return 0;

    } else {
        //不设置长度就报错了！
        notifications.resize(reply->elements);

        for (size_t i = 0; i < reply->elements; ++i) {
            notifications[i] = reply->element[i]->str;
        }
    }

    // LogInfo("赋值结束");

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::delete_notification(const std::string& UID, const std::string& notification_type, const std::string& notification) {
    redisReply *reply;
    std::string key;

    if (notification_type == "friend_request") {
        key = "friend_request_notifications:" + UID;
    } else if (notification_type == "group_request") {
        key = "group_request_notifications:" + UID;
    } else if (notification_type == "message") {
        key = "message_notifications:" + UID;
    } else {
        std::cerr << "Unknown notification type" << std::endl;
        return 0;
    }

    reply = (redisReply*) redisCommand(redisContext_, "LREM %s 0 %s", key.c_str(), notification.c_str());

    // LogInfo("删除通知结束");
    if (reply == nullptr) {
        std::cerr << "Error: " << redisContext_->errstr << std::endl;
        return 0;
    }

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::add_friend(const std::string& UID, const std::string& request_UID) {
    //双向添加好友
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SADD user_friends:%s %s", UID.c_str(), request_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* reply2 = (redisReply*)redisCommand(redisContext_, "SADD user_friends:%s %s", request_UID.c_str(), UID.c_str());
    if (reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    // LogInfo("添加好友完成");
    freeReplyObject(reply);
    freeReplyObject(reply2);
    return 1;
}

bool RedisManager::get_friends(const std::string& UID, std::vector<std::string>& friends_UID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SMEMBERS user_friends:%s", UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        std::cerr << "Redis 返回的不是数组类型" << std::endl;
        freeReplyObject(reply);
        return 0;

    } else {
        friends_UID.resize(reply->elements);

        for (size_t i = 0; i < reply->elements; ++i) {
            friends_UID[i] = reply->element[i]->str;
        }
    }

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::delete_friend(const std::string& UID, const std::string& friend_UID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SREM user_friends:%s %s", UID.c_str(), friend_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* reply2 = (redisReply*)redisCommand(redisContext_, "SREM user_friends:%s %s", friend_UID.c_str(), UID.c_str());
    if (reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);
    freeReplyObject(reply2);
    return 1;
}

bool RedisManager::add_block_friend(const std::string& UID, const std::string& friend_UID) {

    // LogInfo("准备屏蔽");
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SADD user_block_friends:%s %s", UID.c_str(), friend_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }
    // LogInfo("屏蔽完成");

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::check_block_friend(const std::string& UID, const std::string& friend_UID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SMEMBERS user_block_friends:%s", UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    // LogInfo("reply->type = {}", reply->type);
    if (reply->type != REDIS_REPLY_ARRAY) {
        std::cerr << "Redis 返回的不是数组类型" << std::endl;
        freeReplyObject(reply);
        return 0;

    } else {
        // LogInfo("准备进入循环");
        // LogInfo("reply->elements = {}", reply->elements);
        for (size_t i = 0; i < reply->elements; ++i) {
            // LogInfo("friend_UID = {}", friend_UID);
            // LogInfo("reply->element[i]->str = {}", reply->element[i]->str);
            if (friend_UID == reply->element[i]->str) {
                return 1;
            }
        }
    }

    freeReplyObject(reply);
    return 0;
}

bool RedisManager::delete_block_friend(const std::string& UID, const std::string& friend_UID) {
    // LogInfo("准备解除屏蔽");
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SREM user_block_friends:%s %s", UID.c_str(), friend_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }
    // LogInfo("解除屏蔽完成");

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::add_private_chat_message(const std::string& UID, const std::string& friend_UID, const std::string& message) {
    std::string full_message = get_username(UID) + ": " + message;
    redisReply *reply = (redisReply*) redisCommand(redisContext_, "RPUSH private_chat:%s:%s %s", UID.c_str(), friend_UID.c_str(), full_message.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply *reply2 = (redisReply*) redisCommand(redisContext_, "RPUSH private_chat:%s:%s %s", friend_UID.c_str(), UID.c_str(), full_message.c_str());
    if (reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);
    freeReplyObject(reply2);
    return 1;
}

bool RedisManager::get_private_chat_messages(const std::string& UID, const std::string& friend_UID, std::vector<std::string>& messages) {
    redisReply *reply = (redisReply*) redisCommand(redisContext_, "LRANGE private_chat:%s:%s 0 -1", UID.c_str(), friend_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        std::cerr << "Redis 返回的不是数组类型" << std::endl;
        freeReplyObject(reply);
        return 0;

    } else {
        messages.resize(reply->elements);

        for (size_t i = 0; i < reply->elements; ++i) {
            messages[i] = reply->element[i]->str;
        }
    }

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::add_group(const std::string& group_name, const std::string& group_owner_UID) {
    std::string GID = generate_GID();
    if (GID.empty()) {
        return 0;
    }

    //UID映射用户信息
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HMSET group:%s group_name %s group_owner_UID %s", GID.c_str(), group_name.c_str(), group_owner_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* reply2 = (redisReply*)redisCommand(redisContext_, "SADD user_groups:%s %s", group_owner_UID.c_str(), GID.c_str());
    if (reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);
    freeReplyObject(reply2);
    
    std::cout << "创建成功！" << std::endl;
    return 1;
}

std::string RedisManager::get_group_name(const std::string& GID) {
    std::string group_name;

    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HGET group:%s group_name", GID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return "";
    }

    if (reply->type != REDIS_REPLY_STRING) {
        std::cout << "群不存在！" << std::endl;
        return "";
    } else {
        group_name = reply->str;
    }

    freeReplyObject(reply);

    return group_name;
}

std::string RedisManager::get_group_owner_UID(const std::string& GID) {
    std::string group_owner_UID;

    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HGET group:%s group_owner_UID", GID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return "";
    }

    if (reply->type != REDIS_REPLY_STRING) {
        std::cout << "群不存在！" << std::endl;
        return "";
    } else {
        group_owner_UID = reply->str;
    }

    freeReplyObject(reply);

    return group_owner_UID;
}

bool RedisManager::modify_group_name(const std::string& GID, const std::string& new_group_name) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HSET group:%s group_name %s", GID.c_str(), new_group_name.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);

    return 1;
}

bool RedisManager::modify_group_owner_UID(const std::string& GID, const std::string& new_group_owner_UID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HSET group:%s group_owner_UID %s", GID.c_str(), new_group_owner_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);

    return 1;
}

bool RedisManager::delete_group(const std::string& GID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "DEL group:%s", GID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);

    return 1;
}

bool RedisManager::add_group_member(const std::string& GID, const std::string& request_UID) {
    //双向添加群与成员
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SADD group_members:%s %s", GID.c_str(), request_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* reply2 = (redisReply*)redisCommand(redisContext_, "SADD user_groups:%s %s", request_UID.c_str(), GID.c_str());
    if (reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    // LogInfo("添加完成");
    freeReplyObject(reply);
    freeReplyObject(reply2);
    return 1;
}

bool RedisManager::get_groups(const std::string& UID, std::vector<std::string>& groups_GID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SMEMBERS user_groups:%s", UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        std::cerr << "Redis 返回的不是数组类型" << std::endl;
        freeReplyObject(reply);
        return 0;

    } else {
        for (size_t i = 0; i < reply->elements; ++i) {
            groups_GID.push_back(reply->element[i]->str);
        }
    }

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::get_group_members(const std::string& GID, std::vector<std::string>& members_UID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SMEMBERS group_members:%s", GID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        std::cerr << "Redis 返回的不是数组类型" << std::endl;
        freeReplyObject(reply);
        return 0;

    } else {
        for (size_t i = 0; i < reply->elements; ++i) {
            members_UID.push_back(reply->element[i]->str);
        }
    }

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::delete_group_member(const std::string& GID, const std::string& member_UID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SREM group_members:%s %s", GID.c_str(), member_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* reply2 = (redisReply*)redisCommand(redisContext_, "SREM user_groups:%s %s", member_UID.c_str(), GID.c_str());
    if (reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);
    freeReplyObject(reply2);
    return 1;
}

bool RedisManager::add_administrator(const std::string& GID, const std::string& member_UID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SADD group_administrators:%s %s", GID.c_str(), member_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* reply2 = (redisReply*)redisCommand(redisContext_, "SREM group_members:%s %s", GID.c_str(), member_UID.c_str());
    if (reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);
    freeReplyObject(reply2);
    return 1;
}

bool RedisManager::check_administrator(const std::string& GID, const std::string& member_UID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SMEMBERS group_administrators:%s", GID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    // LogInfo("reply->type = {}", reply->type);
    if (reply->type != REDIS_REPLY_ARRAY) {
        std::cerr << "Redis 返回的不是数组类型" << std::endl;
        freeReplyObject(reply);
        return 0;

    } else {
        // LogInfo("准备进入循环");
        // LogInfo("reply->elements = {}", reply->elements);
        for (size_t i = 0; i < reply->elements; ++i) {
            // LogInfo("friend_UID = {}", friend_UID);
            // LogInfo("reply->element[i]->str = {}", reply->element[i]->str);
            if (member_UID == reply->element[i]->str) {
                return 1;
            }
        }
    }

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::get_administrators(const std::string& GID, std::vector<std::string>& administrators_UID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SMEMBERS group_administrators:%s", GID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    // LogInfo("reply->type = {}", reply->type);
    if (reply->type != REDIS_REPLY_ARRAY) {
        std::cerr << "Redis 返回的不是数组类型" << std::endl;
        freeReplyObject(reply);
        return 0;

    } else {
        for (size_t i = 0; i < reply->elements; ++i) {
            administrators_UID.push_back(reply->element[i]->str);
        }
    }

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::delete_administrator(const std::string& GID, const std::string& member_UID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SREM group_administrators:%s %s", GID.c_str(), member_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::add_group_chat_message(const std::string& GID, const std::string& member_UID, const std::string& message) {
    std::string full_message = get_username(member_UID) + ": " + message;
    redisReply *reply = (redisReply*) redisCommand(redisContext_, "RPUSH group_chat:%s %s", GID.c_str(), full_message.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::get_group_chat_messages(const std::string& GID, const std::string& member_UID, std::vector<std::string>& messages) {
    redisReply *reply = (redisReply*) redisCommand(redisContext_, "LRANGE group_chat:%s 0 -1", GID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        std::cerr << "Redis 返回的不是数组类型" << std::endl;
        freeReplyObject(reply);
        return 0;

    } else {
        messages.resize(reply->elements);

        for (size_t i = 0; i < reply->elements; ++i) {
            messages[i] = reply->element[i]->str;
        }
    }

    freeReplyObject(reply);
    return 1;
}