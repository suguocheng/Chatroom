#include "SendJson.h"
#include "../log/mars_logger.h"


int send_json(int connecting_sockfd, const json& j) {
    std::string json_str = j.dump(); // 将 JSON 对象序列化为字符串

    struct msghdr msg = {0};
    struct iovec iov[1];
    char buf[1024];
    
    // 确保缓冲区大小足够
    if (json_str.size() >= sizeof(buf)) {
        std::cerr << "JSON object too large\n";
        return -1;
    }
    
    // LogInfo("buf = ",buf);

    // 准备数据缓冲区
    strncpy(buf, json_str.c_str(), sizeof(buf));

    iov[0].iov_base = buf;
    iov[0].iov_len = json_str.size();
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    ssize_t sent_len = sendmsg(connecting_sockfd, &msg, 0);
    if (sent_len < 0) {
        perror("sendmsg");
        return -1;
    }

    return 0;
}

int receive_json(int connecting_sockfd, json& j) {
    struct msghdr msg = {0};
    struct iovec iov[1];
    char buf[1024];

    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf) - 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    ssize_t received_len = recvmsg(connecting_sockfd, &msg, 0);
    if (received_len < 0) {
        perror("recvmsg");
        return -1;
    }
    buf[received_len] = '\0'; // 确保字符串结束符

    try {
        j = json::parse(buf); // 解析 JSON 字符串
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}