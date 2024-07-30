#include <iostream>
#include <unordered_map>
#include <string>
#include <hiredis/hiredis.h>

void main_menu_UI();
void log_in_UI();
void sign_up_UI();
void change_usename_UI(std::string oldUsername);
void change_emaill_UI(std::string username);
void change_password_UI(std::string username);
void log_out_UI(std::string username);


class AccountManager {
public:
    AccountManager(const std::string& host, int port);
    ~AccountManager();
    
    bool log_in(const std::string& username, const std::string& password);
    bool sign_up(const std::string& username, const std::string& password, const std::string& email);
    std::string get_UID(const std::string& username);
    bool set_UID(const std::string& username, const std::string& UID);
    std::string get_email(const std::string& username);
    bool change_username(const std::string& oldUsername, const std::string& username);
    bool change_email(const std::string& username, const std::string& password, const std::string& email);
    bool change_password(const std::string& username, const std::string& oldPassword, const std::string& newPassword);
    bool log_out(const std::string& password1, const std::string& password2);

private:
    void initializeUserIdCounter();
    std::string generateUniqueId();
    
    redisContext* redisContext_;
};