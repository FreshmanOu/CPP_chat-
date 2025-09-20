#include "message.h"
#include <iostream>

namespace cpp_chat {

// Message基类实现
Message::Message(MessageType type) : type_(type) {}

MessageType Message::getType() const {
    return type_;
}

std::unique_ptr<Message> Message::createMessageFromJson(const std::string& jsonStr) {
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(jsonStr, root)) {
        std::cerr << "Failed to parse JSON: " << jsonStr << std::endl;
        return nullptr;
    }
    
    if (!root.isMember("type")) {
        std::cerr << "Missing 'type' field in JSON: " << jsonStr << std::endl;
        return nullptr;
    }
    
    MessageType type = static_cast<MessageType>(root["type"].asInt());
    std::unique_ptr<Message> msg;
    
    switch (type) {
        case MessageType::REGISTER: {
            auto regMsg = std::unique_ptr<RegisterMessage>(new RegisterMessage());
            if (regMsg->fromJsonString(jsonStr)) {
                msg = std::move(regMsg);
            }
            break;
        }
        case MessageType::LOGIN: {
            auto loginMsg = std::unique_ptr<LoginMessage>(new LoginMessage());
            if (loginMsg->fromJsonString(jsonStr)) {
                msg = std::move(loginMsg);
            }
            break;
        }
        case MessageType::GET_FRIENDS: {
            auto friendsMsg = std::unique_ptr<GetFriendsMessage>(new GetFriendsMessage());
            if (friendsMsg->fromJsonString(jsonStr)) {
                msg = std::move(friendsMsg);
            }
            break;
        }
        case MessageType::PRIVATE_CHAT: {
            auto privateMsg = std::unique_ptr<PrivateChatMessage>(new PrivateChatMessage());
            if (privateMsg->fromJsonString(jsonStr)) {
                msg = std::move(privateMsg);
            }
            break;
        }
        case MessageType::GROUP_CHAT: {
            auto groupMsg = std::unique_ptr<GroupChatMessage>(new GroupChatMessage());
            if (groupMsg->fromJsonString(jsonStr)) {
                msg = std::move(groupMsg);
            }
            break;
        }
        case MessageType::LOGOUT: {
            auto logoutMsg = std::unique_ptr<LogoutMessage>(new LogoutMessage());
            if (logoutMsg->fromJsonString(jsonStr)) {
                msg = std::move(logoutMsg);
            }
            break;
        }
        default:
            std::cerr << "Unknown message type: " << static_cast<int>(type) << std::endl;
            break;
    }
    
    return msg;
}

// RegisterMessage实现
RegisterMessage::RegisterMessage() : Message(MessageType::REGISTER) {}

RegisterMessage::RegisterMessage(const std::string& username, const std::string& password)
    : Message(MessageType::REGISTER), username_(username), password_(password) {}

std::string RegisterMessage::toJsonString() const {
    Json::Value root;
    root["type"] = static_cast<int>(MessageType::REGISTER);
    root["username"] = username_;
    root["password"] = password_;
    root["success"] = success_;  // 添加响应字段
    root["message"] = message_;  // 添加响应消息
    
    Json::FastWriter writer;
    return writer.write(root);
}

bool RegisterMessage::fromJsonString(const std::string& jsonStr) {
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(jsonStr, root)) {
        std::cerr << "Failed to parse JSON: " << jsonStr << std::endl;
        return false;
    }
    
    if (root.isMember("username") && root.isMember("password")) {
        username_ = root["username"].asString();
        password_ = root["password"].asString();
        
        // 解析响应字段（如果存在）
        if (root.isMember("success")) {
            success_ = root["success"].asBool();
        }
        if (root.isMember("message")) {
            message_ = root["message"].asString();
        }
        
        return true;
    }
    
    return false;
}

std::string RegisterMessage::getUsername() const {
    return username_;
}

std::string RegisterMessage::getPassword() const {
    return password_;
}

void RegisterMessage::setSuccess(bool success) {
    success_ = success;
}

void RegisterMessage::setMessage(const std::string& message) {
    message_ = message;
}

bool RegisterMessage::getSuccess() const {
    return success_;
}

std::string RegisterMessage::getMessage() const {
    return message_;
}

// LoginMessage实现
LoginMessage::LoginMessage() : Message(MessageType::LOGIN) {}

LoginMessage::LoginMessage(const std::string& username, const std::string& password)
    : Message(MessageType::LOGIN), username_(username), password_(password) {}

std::string LoginMessage::toJsonString() const {
    Json::Value root;
    root["type"] = static_cast<int>(MessageType::LOGIN);
    root["username"] = username_;
    root["password"] = password_;
    root["success"] = success_;  // 添加响应字段
    root["message"] = message_;  // 添加响应消息
    
    // 如果有离线消息，也要序列化
    if (!offlinemessages_.empty()) {
        Json::Value offlineArray(Json::arrayValue);
        for (const auto& msg : offlinemessages_) {
            offlineArray.append(msg);
        }
        root["offline_messages"] = offlineArray;
    }
    
    Json::FastWriter writer;
    return writer.write(root);
}

bool LoginMessage::fromJsonString(const std::string& jsonStr) {
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(jsonStr, root)) {
        std::cerr << "Failed to parse JSON: " << jsonStr << std::endl;
        return false;
    }
    
    if (root.isMember("username") && root.isMember("password")) {
        username_ = root["username"].asString();
        password_ = root["password"].asString();
        
        // 解析响应字段（如果存在）
        if (root.isMember("success")) {
            success_ = root["success"].asBool();
        }
        if (root.isMember("message")) {
            message_ = root["message"].asString();
        }
        
        // 解析离线消息（如果存在）
        if (root.isMember("offline_messages") && root["offline_messages"].isArray()) {
            offlinemessages_.clear();
            const Json::Value& offlineArray = root["offline_messages"];
            for (Json::ArrayIndex i = 0; i < offlineArray.size(); ++i) {
                offlinemessages_.push_back(offlineArray[i].asString());
            }
        }
        
        return true;
    }
    
    return false;
}

std::string LoginMessage::getUsername() const {
    return username_;
}

std::string LoginMessage::getPassword() const {
    return password_;
}

void LoginMessage::setSuccess(bool success) {
    success_ = success;
}

void LoginMessage::setMessage(const std::string& message) {
    message_ = message;
}

void LoginMessage::setOfflineMessages(const std::vector<std::string>& offlineMessages) {
    offlinemessages_ = offlineMessages;
}

bool LoginMessage::getSuccess() const {
    return success_;
}

std::string LoginMessage::getMessage() const {
    return message_;
}

std::vector<std::string> LoginMessage::getOfflineMessages() const {
    return offlinemessages_;
}

// GetFriendsMessage实现
GetFriendsMessage::GetFriendsMessage() : Message(MessageType::GET_FRIENDS) {}

GetFriendsMessage::GetFriendsMessage(const std::string& username)
    : Message(MessageType::GET_FRIENDS), username_(username) {}

std::string GetFriendsMessage::toJsonString() const {
    Json::Value root;
    root["type"] = static_cast<int>(MessageType::GET_FRIENDS);
    root["username"] = username_;
    root["success"] = success_;  // 添加响应字段
    root["message"] = message_;  // 添加响应消息
    
    // 序列化用户列表
    if (!allUsers_.empty()) {
        Json::Value allUsersArray(Json::arrayValue);
        for (const auto& user : allUsers_) {
            allUsersArray.append(user);
        }
        root["all_users"] = allUsersArray;
    }
    
    if (!onlineUsers_.empty()) {
        Json::Value onlineUsersArray(Json::arrayValue);
        for (const auto& user : onlineUsers_) {
            onlineUsersArray.append(user);
        }
        root["online_users"] = onlineUsersArray;
    }
    
    Json::FastWriter writer;
    return writer.write(root);
}

bool GetFriendsMessage::fromJsonString(const std::string& jsonStr) {
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(jsonStr, root)) {
        std::cerr << "Failed to parse JSON: " << jsonStr << std::endl;
        return false;
    }
    
    if (root.isMember("username")) {
        username_ = root["username"].asString();
        
        // 解析响应字段（如果存在）
        if (root.isMember("success")) {
            success_ = root["success"].asBool();
        }
        if (root.isMember("message")) {
            message_ = root["message"].asString();
        }
        
        // 解析用户列表（如果存在）
        if (root.isMember("all_users") && root["all_users"].isArray()) {
            allUsers_.clear();
            const Json::Value& allUsersArray = root["all_users"];
            for (Json::ArrayIndex i = 0; i < allUsersArray.size(); ++i) {
                allUsers_.push_back(allUsersArray[i].asString());
            }
        }
        
        if (root.isMember("online_users") && root["online_users"].isArray()) {
            onlineUsers_.clear();
            const Json::Value& onlineUsersArray = root["online_users"];
            for (Json::ArrayIndex i = 0; i < onlineUsersArray.size(); ++i) {
                onlineUsers_.push_back(onlineUsersArray[i].asString());
            }
        }
        
        return true;
    }
    
    return false;
}

std::string GetFriendsMessage::getUsername() const {
    return username_;
}

void GetFriendsMessage::setAllUsers(const std::vector<std::string>& users) {
    allUsers_ = users;
}

void GetFriendsMessage::setOnlineUsers(const std::vector<std::string>& users) {
    onlineUsers_ = users;
}

void GetFriendsMessage::setSuccess(bool success) {
    success_ = success;
}

void GetFriendsMessage::setMessage(const std::string& message) {
    message_ = message;
}

bool GetFriendsMessage::getSuccess() const {
    return success_;
}

std::string GetFriendsMessage::getMessage() const {
    return message_;
}

std::vector<std::string> GetFriendsMessage::getAllUsers() const {
    return allUsers_;
}

std::vector<std::string> GetFriendsMessage::getOnlineUsers() const {
    return onlineUsers_;
}

// PrivateChatMessage实现
PrivateChatMessage::PrivateChatMessage() : Message(MessageType::PRIVATE_CHAT) {}

PrivateChatMessage::PrivateChatMessage(const std::string& fromUser, const std::string& toUser, const std::string& content)
    : Message(MessageType::PRIVATE_CHAT), fromUser_(fromUser), toUser_(toUser), content_(content) {}

PrivateChatMessage::PrivateChatMessage(const std::string& fromUser, const std::string& toUser)
    : Message(MessageType::PRIVATE_CHAT), fromUser_(fromUser), toUser_(toUser), content_("") {}

std::string PrivateChatMessage::toJsonString() const {
    Json::Value root;
    root["type"] = static_cast<int>(MessageType::PRIVATE_CHAT);
    root["from_user"] = fromUser_;
    root["to_user"] = toUser_;
    root["content"] = content_;
    root["success"] = success_;  // 添加响应字段
    root["message"] = message_;  // 添加响应消息
    
    Json::FastWriter writer;
    return writer.write(root);
}

bool PrivateChatMessage::fromJsonString(const std::string& jsonStr) {
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(jsonStr, root)) {
        std::cerr << "Failed to parse JSON: " << jsonStr << std::endl;
        return false;
    }
    
    if (root.isMember("from_user") && root.isMember("to_user") && root.isMember("content")) {
        fromUser_ = root["from_user"].asString();
        toUser_ = root["to_user"].asString();
        content_ = root["content"].asString();
        
        // 解析响应字段（如果存在）
        if (root.isMember("success")) {
            success_ = root["success"].asBool();
        }
        if (root.isMember("message")) {
            message_ = root["message"].asString();
        }
        
        return true;
    }
    
    return false;
}

std::string PrivateChatMessage::getFromUser() const {
    return fromUser_;
}

std::string PrivateChatMessage::getToUser() const {
    return toUser_;
}

std::string PrivateChatMessage::getContent() const {
    return content_;
}

void PrivateChatMessage::setSuccess(bool success) {
    success_ = success;
}

void PrivateChatMessage::setMessage(const std::string& message) {
    message_ = message;
}

bool PrivateChatMessage::getSuccess() const {
    return success_;
}

std::string PrivateChatMessage::getMessage() const {
    return message_;
}

void PrivateChatMessage::setFromUser(const std::string& fromUser) {
    fromUser_ = fromUser;
}

// GroupChatMessage实现
GroupChatMessage::GroupChatMessage() : Message(MessageType::GROUP_CHAT) {}

GroupChatMessage::GroupChatMessage(const std::string& fromUser, const std::string& toUsers, const std::string& content)
    : Message(MessageType::GROUP_CHAT), fromUser_(fromUser), toUsers_(toUsers), content_(content) {}

GroupChatMessage::GroupChatMessage(const std::string& content)
    : Message(MessageType::GROUP_CHAT), fromUser_(""), toUsers_(""), content_(content) {}

std::string GroupChatMessage::toJsonString() const {
    Json::Value root;
    root["type"] = static_cast<int>(MessageType::GROUP_CHAT);
    root["from_user"] = fromUser_;
    root["to_users"] = toUsers_;
    root["content"] = content_;
    root["success"] = success_;  // 添加响应字段
    root["message"] = message_;  // 添加响应消息
    
    Json::FastWriter writer;
    return writer.write(root);
}

bool GroupChatMessage::fromJsonString(const std::string& jsonStr) {
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(jsonStr, root)) {
        std::cerr << "Failed to parse JSON: " << jsonStr << std::endl;
        return false;
    }
    
    if (root.isMember("from_user") && root.isMember("to_users") && root.isMember("content")) {
        fromUser_ = root["from_user"].asString();
        toUsers_ = root["to_users"].asString();
        content_ = root["content"].asString();
        
        // 解析响应字段（如果存在）
        if (root.isMember("success")) {
            success_ = root["success"].asBool();
        }
        if (root.isMember("message")) {
            message_ = root["message"].asString();
        }
        
        return true;
    }
    
    return false;
}

std::string GroupChatMessage::getFromUser() const {
    return fromUser_;
}

std::string GroupChatMessage::getToUsers() const {
    return toUsers_;
}

std::string GroupChatMessage::getContent() const {
    return content_;
}

void GroupChatMessage::setFromUser(const std::string& fromUser) {
    fromUser_ = fromUser;
}

void GroupChatMessage::setSuccess(bool success) {
    success_ = success;
}

void GroupChatMessage::setMessage(const std::string& message) {
    message_ = message;
}

// LogoutMessage实现
LogoutMessage::LogoutMessage() : Message(MessageType::LOGOUT) {}

LogoutMessage::LogoutMessage(const std::string& username)
    : Message(MessageType::LOGOUT), username_(username) {}

std::string LogoutMessage::toJsonString() const {
    Json::Value root;
    root["type"] = static_cast<int>(MessageType::LOGOUT);
    root["username"] = username_;
    root["success"] = success_;  // 添加响应字段
    root["message"] = message_;  // 添加响应消息
    
    Json::FastWriter writer;
    return writer.write(root);
}

bool LogoutMessage::fromJsonString(const std::string& jsonStr) {
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(jsonStr, root)) {
        std::cerr << "Failed to parse JSON: " << jsonStr << std::endl;
        return false;
    }
    
    if (root.isMember("username")) {
        username_ = root["username"].asString();
        
        // 解析响应字段（如果存在）
        if (root.isMember("success")) {
            success_ = root["success"].asBool();
        }
        if (root.isMember("message")) {
            message_ = root["message"].asString();
        }
        
        return true;
    }
    
    return false;
}

std::string LogoutMessage::getUsername() const {
    return username_;
}

void LogoutMessage::setSuccess(bool success) {
    success_ = success;
}

void LogoutMessage::setMessage(const std::string& message) {
    message_ = message;
}

bool LogoutMessage::getSuccess() const {
    return success_;
}

std::string LogoutMessage::getMessage() const {
    return message_;
}



} // namespace cpp_chat