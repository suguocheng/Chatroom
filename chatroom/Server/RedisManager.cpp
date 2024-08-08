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
    // initializeCounter();
}

RedisManager::~RedisManager() {
    if (redisContext_) {
        redisFree(redisContext_);
    }
}

std::string RedisManager::generateUniqueId() {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "INCR user_id_counter");

    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return " ";
    }

    std::string UID = std::to_string(reply->integer);
    // LogInfo("UID1 = {}", UID);
    freeReplyObject(reply);
    return UID;
}

void RedisManager::initializeCounter() {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SETNX user_id_counter 0");
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
    }
    freeReplyObject(reply);
}

bool RedisManager::add_user(const std::string& username, const std::string& password, const std::string& security_question, const std::string& security_answer) {
    std::string UID = generateUniqueId();
    if (UID.empty()) {
        return 0;
    }

    //检查用户名是否存在
    if (get_UID(username) != "") {
        std::cout << "该用户名已被注册！" << std::endl;
        return 0;
    }

    //用户名映射UID
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HSET username_to_id %s %s", username.c_str(), UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    //UID映射用户名
    redisReply* reply1 = (redisReply*)redisCommand(redisContext_, "HSET id_to_username %s %s", UID.c_str(), username.c_str());
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

    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HGET username_to_id %s", username.c_str());
    
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

    redisReply* reply = (redisReply*)redisCommand(redisContext_, "HGET id_to_username %s", UID.c_str());
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

    redisReply* del_reply = (redisReply*)redisCommand(redisContext_, "HDEL username_to_id %s", old_username.c_str());
    if (del_reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* del_reply2 = (redisReply*)redisCommand(redisContext_, "HDEL id_to_username %s", UID.c_str());
    if (del_reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* set_reply = (redisReply*)redisCommand(redisContext_, "HSET username_to_id %s %s", new_username.c_str(), UID.c_str());
    if (set_reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* set_reply2 = (redisReply*)redisCommand(redisContext_, "HSET id_to_username %s %s", UID.c_str(), new_username.c_str());
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
    redisReply* reply1 = (redisReply*)redisCommand(redisContext_, "DEL user:%s", UID.c_str());
    if (reply1 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* reply2 = (redisReply*)redisCommand(redisContext_, "HDEL username_to_id %s", UID.c_str());
    if (reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

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
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SADD friends:%s %s", UID.c_str(), request_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* reply2 = (redisReply*)redisCommand(redisContext_, "SADD friends:%s %s", request_UID.c_str(), UID.c_str());
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
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SMEMBERS friends:%s", UID.c_str());
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
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SREM friends:%s %s", UID.c_str(), friend_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply* reply2 = (redisReply*)redisCommand(redisContext_, "SREM friends:%s %s", friend_UID.c_str(), UID.c_str());
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
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SADD block_friends:%s %s", UID.c_str(), friend_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }
    // LogInfo("屏蔽完成");

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::check_block_friend(const std::string& UID, const std::string& friend_UID) {
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SMEMBERS block_friends:%s", UID.c_str());
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
    redisReply* reply = (redisReply*)redisCommand(redisContext_, "SREM block_friends:%s %s", UID.c_str(), friend_UID.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }
    // LogInfo("解除屏蔽完成");

    freeReplyObject(reply);
    return 1;
}

bool RedisManager::add_chat_message(const std::string& UID, const std::string& friend_UID, const std::string& message) {
    std::string full_message = get_username(UID) + ":" + message;
    redisReply *reply = (redisReply*) redisCommand(redisContext_, "RPUSH chat:%s:%s %s", UID.c_str(), friend_UID.c_str(), full_message.c_str());
    if (reply == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    redisReply *reply2 = (redisReply*) redisCommand(redisContext_, "RPUSH chat:%s:%s %s", friend_UID.c_str(), UID.c_str(), full_message.c_str());
    if (reply2 == NULL) {
        std::cerr << "Redis 命令执行失败！" << std::endl;
        return 0;
    }

    freeReplyObject(reply);
    freeReplyObject(reply2);
    return 1;
}

bool RedisManager::get_chat_messages(const std::string& UID, const std::string& friend_UID, std::vector<std::string>& messages) {
    redisReply *reply = (redisReply*) redisCommand(redisContext_, "LRANGE chat:%s:%s 0 -1", UID.c_str(), friend_UID.c_str());
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