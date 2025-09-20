#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <mysql/mysql.h>

namespace cpp_chat {
class Database {
public:
    
    Database();
    ~Database();

    bool connect(const std::string& host, const std::string& user, const std::string& password, 
                 const std::string& database, unsigned int port = 3306);
    void disconnect();

    bool isUserExists(const std::string& username);
    
    bool registerUser(const std::string& username, const std::string& password);
    bool verifyLogin(const std::string& username, const std::string& password);
    bool setUserOnline(const std::string& username, int socketFd);
    bool setUserOffline(const std::string& username);
    std::vector<std::string> getAllUsers();
    std::vector<std::string> getOnlineUsers();
    
    int getUserSocketFd(const std::string& username);

    bool storeOfflineMessage(const std::string& toUser, const std::string& message);

    std::vector<std::string> getOfflineMessages(const std::string& username);
    
    bool clearOfflineMessages(const std::string& username);
    
private:
    MYSQL* conn_; // MySQL连接句柄
    bool connected_; // 是否已连接
    bool executeQuery(const std::string& sql);// 执行SQL查询
};

} // namespace cpp_chat

#endif // DATABASE_H