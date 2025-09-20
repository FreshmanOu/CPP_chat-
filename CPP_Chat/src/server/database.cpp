#include "database.h"
#include <iostream>

namespace cpp_chat {

Database::Database() : conn_(nullptr), connected_(false) {
    // 初始化MySQL库
    mysql_library_init(0, nullptr, nullptr);
}

Database::~Database() {
    // 断开连接
    disconnect();
    
    // 清理MySQL库
    mysql_library_end();
}

bool Database::connect(const std::string& host, const std::string& user, const std::string& password, 
                       const std::string& database, unsigned int port) {
    // 如果已连接，先断开
    if (connected_) {
        disconnect();
    }
    
    // 初始化连接句柄
    conn_ = mysql_init(nullptr);
    if (!conn_) {
        std::cerr << "Failed to initialize MySQL connection" << std::endl;
        return false;
    }
    
    // 连接数据库
    if (!mysql_real_connect(conn_, host.c_str(), user.c_str(), password.c_str(), 
                           database.c_str(), port, nullptr, 0)) {
        std::cerr << "Failed to connect to MySQL: " << mysql_error(conn_) << std::endl;
        mysql_close(conn_);
        conn_ = nullptr;
        return false;
    }
    
    connected_ = true;
    
    // 创建必要的表（如果不存在）
    executeQuery("CREATE TABLE IF NOT EXISTS users ("
                "name VARCHAR(50) PRIMARY KEY,"
                "password VARCHAR(50) NOT NULL"
                ")");
    
    executeQuery("CREATE TABLE IF NOT EXISTS online_users ("
                "name VARCHAR(50) PRIMARY KEY,"
                "socket_fd INT NOT NULL,"
                "FOREIGN KEY (name) REFERENCES users(name)"
                ")");
    
    executeQuery("CREATE TABLE IF NOT EXISTS offline_messages ("
                "id INT AUTO_INCREMENT PRIMARY KEY,"
                "to_user VARCHAR(50) NOT NULL,"
                "message TEXT NOT NULL,"
                "FOREIGN KEY (to_user) REFERENCES users(name)"
                ")");
    
    return true;
}

void Database::disconnect() {
    if (connected_ && conn_) {
        mysql_close(conn_);
        conn_ = nullptr;
        connected_ = false;
    }
}

bool Database::executeQuery(const std::string& sql) {
    if (!connected_ || !conn_) {
        std::cerr << "Not connected to database" << std::endl;
        return false;
    }
    
    if (mysql_query(conn_, sql.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn_) << std::endl;
        return false;
    }
    
    return true;
}

bool Database::isUserExists(const std::string& username) {
    if (!connected_ || !conn_) {
        return false;
    }
    
    std::string sql = "SELECT * FROM users WHERE name='" + username + "'";
    
    if (mysql_query(conn_, sql.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn_) << std::endl;
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(conn_);
    if (!result) {
        std::cerr << "MySQL store result error: " << mysql_error(conn_) << std::endl;
        return false;
    }
    
    bool exists = (mysql_num_rows(result) > 0);
    mysql_free_result(result);
    
    return exists;
}

bool Database::registerUser(const std::string& username, const std::string& password) {
    if (!connected_ || !conn_) {
        return false;
    }
    
    // 检查用户是否已存在
    if (isUserExists(username)) {
        std::cerr << "User already exists: " << username << std::endl;
        return false;
    }
    
    std::string sql = "INSERT INTO users (name, password) VALUES ('" + 
                     username + "', '" + password + "')";
    
    return executeQuery(sql);
}

bool Database::verifyLogin(const std::string& username, const std::string& password) {
    if (!connected_ || !conn_) {
        std::cerr << "[DEBUG] 数据库未连接" << std::endl;
        return false;
    }
    
    std::string sql = "SELECT * FROM users WHERE name='" + username + 
                     "' AND password='" + password + "'";
    
    std::cout << "[DEBUG] 执行SQL: " << sql << std::endl;
    
    if (mysql_query(conn_, sql.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn_) << std::endl;
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(conn_);
    if (!result) {
        std::cerr << "MySQL store result error: " << mysql_error(conn_) << std::endl;
        return false;
    }
    
    bool verified = (mysql_num_rows(result) > 0);
    std::cout << "[DEBUG] 查询结果: 找到 " << mysql_num_rows(result) << " 条记录, 验证结果: " << (verified ? "成功" : "失败") << std::endl;
    mysql_free_result(result);
    
    return verified;
}

bool Database::setUserOnline(const std::string& username, int socketFd) {
    if (!connected_ || !conn_) {
        return false;
    }
    
    // 先删除可能存在的记录
    std::string deleteSql = "DELETE FROM online_users WHERE name='" + username + "'";
    if (!executeQuery(deleteSql)) {
        return false;
    }
    
    // 插入新记录
    std::string insertSql = "INSERT INTO online_users (name, socket_fd) VALUES ('" + 
                           username + "', " + std::to_string(socketFd) + ")";
    
    return executeQuery(insertSql);
}

bool Database::setUserOffline(const std::string& username) {
    if (!connected_ || !conn_) {
        return false;
    }
    
    std::string sql = "DELETE FROM online_users WHERE name='" + username + "'";
    
    return executeQuery(sql);
}

std::vector<std::string> Database::getAllUsers() {
    std::vector<std::string> users;
    
    if (!connected_ || !conn_) {
        return users;
    }
    
    std::string sql = "SELECT name FROM users";
    
    if (mysql_query(conn_, sql.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn_) << std::endl;
        return users;
    }
    
    MYSQL_RES* result = mysql_store_result(conn_);
    if (!result) {
        std::cerr << "MySQL store result error: " << mysql_error(conn_) << std::endl;
        return users;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        users.push_back(row[0]);
    }
    
    mysql_free_result(result);
    
    return users;
}

std::vector<std::string> Database::getOnlineUsers() {
    std::vector<std::string> users;
    
    if (!connected_ || !conn_) {
        return users;
    }
    
    std::string sql = "SELECT name FROM online_users";
    
    if (mysql_query(conn_, sql.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn_) << std::endl;
        return users;
    }
    
    MYSQL_RES* result = mysql_store_result(conn_);
    if (!result) {
        std::cerr << "MySQL store result error: " << mysql_error(conn_) << std::endl;
        return users;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        users.push_back(row[0]);
    }
    
    mysql_free_result(result);
    
    return users;
}

int Database::getUserSocketFd(const std::string& username) {
    if (!connected_ || !conn_) {
        return -1;
    }
    
    std::string sql = "SELECT socket_fd FROM online_users WHERE name='" + username + "'";
    
    if (mysql_query(conn_, sql.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn_) << std::endl;
        return -1;
    }
    
    MYSQL_RES* result = mysql_store_result(conn_);
    if (!result) {
        std::cerr << "MySQL store result error: " << mysql_error(conn_) << std::endl;
        return -1;
    }
    
    if (mysql_num_rows(result) == 0) {
        mysql_free_result(result);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int socketFd = std::stoi(row[0]);
    
    mysql_free_result(result);
    
    return socketFd;
}

bool Database::storeOfflineMessage(const std::string& toUser, const std::string& message) {
    if (!connected_ || !conn_) {
        return false;
    }
    
    std::string sql = "INSERT INTO offline_messages (to_user, message) VALUES ('" + 
                     toUser + "', '" + message + "')";
    
    return executeQuery(sql);
}

std::vector<std::string> Database::getOfflineMessages(const std::string& username) {
    std::vector<std::string> messages;
    
    if (!connected_ || !conn_) {
        return messages;
    }
    
    std::string sql = "SELECT message FROM offline_messages WHERE to_user='" + username + "'";
    
    if (mysql_query(conn_, sql.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn_) << std::endl;
        return messages;
    }
    
    MYSQL_RES* result = mysql_store_result(conn_);
    if (!result) {
        std::cerr << "MySQL store result error: " << mysql_error(conn_) << std::endl;
        return messages;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        messages.push_back(row[0]);
    }
    
    mysql_free_result(result);
    
    return messages;
}

bool Database::clearOfflineMessages(const std::string& username) {
    if (!connected_ || !conn_) {
        return false;
    }
    
    std::string sql = "DELETE FROM offline_messages WHERE to_user='" + username + "'";
    
    return executeQuery(sql);
}

} // namespace cpp_chat