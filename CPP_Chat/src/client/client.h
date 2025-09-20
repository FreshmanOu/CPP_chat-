#ifndef CPP_CHAT_CLIENT_H
#define CPP_CHAT_CLIENT_H

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <memory>
#include "../common/message.h"

namespace cpp_chat {
class Client {
public:
    /**
     消息回调函数类型
    */
    using MessageCallback = std::function<void(const Message& msg)>;
    
    /**
     * 连接状态回调函数类型
     * connected 是否已连接
     * errorMsg 错误信息（如果有）
     */
    using ConnectionCallback = std::function<void(bool connected, const std::string& errorMsg)>;
    
    Client();
    ~Client();
    
    bool connect(const std::string& ip, int port);
    void disconnect();
    bool sendMessage(const Message& msg);
    /**
     *  设置消息回调
     *  callback 回调函数
     */
    void setMessageCallback(MessageCallback callback);
    /**
      设置连接状态回调
      callback 回调函数
     */
    void setConnectionCallback(ConnectionCallback callback);
    bool isConnected() const;
    
    void registerUser(const std::string& username, const std::string& password);
    void login(const std::string& username, const std::string& password);
    void getFriends();
    void sendPrivateMessage(const std::string& toUser, const std::string& content);
    void sendGroupMessage(const std::string& content);
    void logout();//退出登录

private:
    // 连接状态
    std::atomic<bool> connected_;
    std::string serverIp_;
    int serverPort_;
    int socketFd_;
     // 当前登录用户信息
    std::string currentUsername_;
    // 线程相关
    std::thread receiveThread_;
    std::atomic<bool> running_;
    // 回调函数
    MessageCallback messageCallback_;
    ConnectionCallback connectionCallback_;
    std::mutex callbackMutex_;
    // 接收线程函数
    void receiveThreadFunc();
    // 处理接收到的消息
    void handleMessage(const std::string& jsonStr);
    // 触发消息回调
    void triggerMessageCallback(const Message& msg);
    // 触发连接状态回调
    void triggerConnectionCallback(bool connected, const std::string& errorMsg);
};

} // namespace cpp_chat

#endif // CPP_CHAT_CLIENT_H