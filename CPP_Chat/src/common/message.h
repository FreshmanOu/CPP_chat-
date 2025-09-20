#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <memory>
#include <json/json.h>

namespace cpp_chat {

/* 消息类型枚举 */
enum class MessageType {
    REGISTER,       // 注册
    LOGIN,          // 登录
    GET_FRIENDS,    // 获取好友列表
    PRIVATE_CHAT,   // 私聊
    GROUP_CHAT,     // 群聊
    LOGOUT,         // 退出登录
    UNKNOWN         // 未知类型
};

/*消息基类*/
class Message {
public:
    explicit Message(MessageType type);
    virtual ~Message() = default;
    MessageType getType() const;
    
    //将消息序列化为JSON字符串
    virtual std::string toJsonString() const = 0;
    
    //从JSON字符串反序列化消息
    virtual bool fromJsonString(const std::string& jsonStr) = 0;
    
    /**
     * @brief 从JSON字符串创建消息对象
     * @param jsonStr JSON字符串
     * @return 消息对象指针
     */
    static std::unique_ptr<Message> createMessageFromJson(const std::string& jsonStr);

protected:
    MessageType type_; // 消息类型
};


class RegisterMessage : public Message {
public:
    RegisterMessage();
    RegisterMessage(const std::string& username, const std::string& password);
    std::string toJsonString() const override;
    bool fromJsonString(const std::string& jsonStr) override;
    std::string getUsername() const;
    std::string getPassword() const;
    void setSuccess(bool success);
    void setMessage(const std::string& message);
    bool getSuccess() const;
    std::string getMessage() const;
    
private:
    std::string username_; // 用户名
    std::vector<std::string> allUsers_; // 所有用户
    std::vector<std::string> onlineUsers_; // 在线用户
    bool success_ = false; // 是否成功
    std::string message_; // 消息内容
    std::string password_; // 密码
};


class LoginMessage : public Message {
public:

    LoginMessage();
    LoginMessage(const std::string& username, const std::string& password);
    std::string toJsonString() const override;
    bool fromJsonString(const std::string& jsonStr) override;
    std::string getUsername() const;
    std::string getPassword() const;
    void setSuccess(bool success);
    void setMessage(const std::string& message);//设置消息内容
    void setOfflineMessages(const std::vector<std::string>& messages);//设置离线消息
    bool getSuccess() const;//获取成功标志
    std::string getMessage() const;
    std::vector<std::string> getOfflineMessages() const;//获取离线消息
    
private:
    std::string username_; // 用户名
    std::string password_; // 密码
    bool success_ = false; // 是否成功
    std::string message_; // 消息内容
    std::vector<std::string> offlinemessages_; // 离线消息列表
};


/**
 获取好友列表消息类
 */
class GetFriendsMessage : public Message {
public:
    GetFriendsMessage();
    explicit GetFriendsMessage(const std::string& username);
    std::string toJsonString() const override;
    bool fromJsonString(const std::string& jsonStr) override;
    std::string getUsername() const;
    void setAllUsers(const std::vector<std::string>& users);//设置所有用户列表
    void setOnlineUsers(const std::vector<std::string>& users);//设置在线用户列表
    void setSuccess(bool success);
    void setMessage(const std::string& message);
    std::vector<std::string> getAllUsers() const;
    std::vector<std::string> getOnlineUsers() const;
    bool getSuccess() const;
    std::string getMessage() const;
    
private:
    std::string username_; // 用户名
    std::vector<std::string> allUsers_; // 所有用户
    std::vector<std::string> onlineUsers_; // 在线用户
    bool success_ = false; // 是否成功
    std::string message_; // 消息内容

};

/**
 私聊消息类
 */
class PrivateChatMessage : public Message {
public:
    PrivateChatMessage();
    PrivateChatMessage(const std::string& fromUser, const std::string& toUser, const std::string& content);
    //用于响应消息）
    PrivateChatMessage(const std::string& fromUser, const std::string& toUser);
    std::string toJsonString() const override;
    bool fromJsonString(const std::string& jsonStr) override;
    std::string getFromUser() const;
    std::string getToUser() const;
    std::string getContent() const;
    void setSuccess(bool success);
    void setMessage(const std::string& message);
    bool getSuccess() const;
    std::string getMessage() const;
    void setFromUser(const std::string& fromUser);//设置发送者
    
private:
    std::string fromUser_; // 发送者
    std::string toUser_;   // 接收者
    std::string content_;  // 消息内容
    bool success_ = false;  // 是否成功
    std::string message_;  // 消息内容
};

/**
 群聊消息类
 */
class GroupChatMessage : public Message {
public:
    GroupChatMessage();
    GroupChatMessage(const std::string& fromUser, const std::string& toUsers, const std::string& content);
    std::string toJsonString() const override;
    bool fromJsonString(const std::string& jsonStr) override;
    std::string getFromUser() const;
    std::string getToUsers() const;
    std::string getContent() const;
    explicit GroupChatMessage(const std::string& content);
    void setFromUser(const std::string& fromUser);//设置发送者
    void setSuccess(bool success);
    void setMessage(const std::string& message);
private:
    std::string fromUser_; // 发送者
    std::string toUsers_;  // 接收者列表（以分号分隔）
    std::string content_;  // 消息内容
    bool success_ = false;  // 是否成功
    std::string message_;  // 消息内容
};

/**
 退出登录消息类
 */
class LogoutMessage : public Message {
public:
    LogoutMessage();
    explicit LogoutMessage(const std::string& username);
    std::string toJsonString() const override;
    bool fromJsonString(const std::string& jsonStr) override;
    std::string getUsername() const;
    void setSuccess(bool success);
    void setMessage(const std::string& message);
    bool getSuccess() const;
    std::string getMessage() const;
private:
    std::string username_; // 用户名
    bool success_ = false; // 是否成功
    std::string message_; // 消息内容
};
} // namespace cpp_chat

#endif // MESSAGE_H