// clientmain.cpp 顶部
#include "client.h"
#include <iostream>
#include <string>
#include <sstream>

#ifdef _WIN32
#include <windows.h>   // Windows下设置控制台代码页
#else
#include <locale>      // Linux/macOS下用locale
#endif

// 统一初始化终端UTF-8
void initConsoleUTF8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#else
    // 保证C++流和C流都用UTF-8
    std::locale::global(std::locale("en_US.UTF-8"));
    std::ios::sync_with_stdio(false);
    std::wcin.imbue(std::locale());
    std::wcout.imbue(std::locale());
    // 若系统locale未安装en_US.UTF-8，可改为"C.UTF-8"或"zh_CN.UTF-8"
#endif
}
std::string messageTypeToString(cpp_chat::MessageType type) {
    switch (type) {
        case cpp_chat::MessageType::REGISTER: return "REGISTER";
        case cpp_chat::MessageType::LOGIN: return "LOGIN";
        case cpp_chat::MessageType::GET_FRIENDS: return "GET_FRIENDS";
        case cpp_chat::MessageType::PRIVATE_CHAT: return "PRIVATE_CHAT";
        case cpp_chat::MessageType::GROUP_CHAT: return "GROUP_CHAT";
        case cpp_chat::MessageType::LOGOUT: return "LOGOUT";
        case cpp_chat::MessageType::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

void messageCallback(const cpp_chat::Message& msg) {
    // Handle different message types with friendly output
    if (auto regMsg = dynamic_cast<const cpp_chat::RegisterMessage*>(&msg)) {
        std::cout << "注册结果: " << (regMsg->getSuccess() ? "成功" : "失败") << " - " << regMsg->getMessage() << std::endl;
    } else if (auto loginMsg = dynamic_cast<const cpp_chat::LoginMessage*>(&msg)) {
        std::cout << "登录结果: " << (loginMsg->getSuccess() ? "成功" : "失败") << " - " << loginMsg->getMessage() << std::endl;
        auto offlineMsgs = loginMsg->getOfflineMessages();
        if (!offlineMsgs.empty()) {
            std::cout << "离线消息:" << std::endl;
            for (const auto& offMsg : offlineMsgs) {
                std::cout << offMsg << std::endl;
            }
        }
    } else if (auto friendsMsg = dynamic_cast<const cpp_chat::GetFriendsMessage*>(&msg)) {
        if (friendsMsg->getSuccess()) {
            std::cout << "所有用户:" << std::endl;
            for (const auto& user : friendsMsg->getAllUsers()) {
                std::cout << "- " << user << std::endl;
            }
            std::cout << "在线用户:" << std::endl;
            for (const auto& user : friendsMsg->getOnlineUsers()) {
                std::cout << "- " << user << std::endl;
            }
        } else {
            std::cout << "获取好友列表失败: " << friendsMsg->getMessage() << std::endl;
        }
    } else if (auto privateMsg = dynamic_cast<const cpp_chat::PrivateChatMessage*>(&msg)) {
        if (privateMsg->getFromUser().empty()) { // Response from server
            std::cout << "私聊发送结果: " << (privateMsg->getSuccess() ? "成功" : "失败") << " - " << privateMsg->getMessage() << std::endl;
        } else {
            std::cout << "收到私聊 [" << privateMsg->getFromUser() << "]: " << privateMsg->getContent() << std::endl;
        }
    } else if (auto groupMsg = dynamic_cast<const cpp_chat::GroupChatMessage*>(&msg)) {
        if (groupMsg->getFromUser().empty()) { // Response from server
            std::cout << "群聊发送结果: " << (groupMsg->getContent().empty() ? "成功" : "失败") << " - " << groupMsg->getContent() << std::endl;
        } else {
            std::cout << "收到群聊 [" << groupMsg->getFromUser() << "]: " << groupMsg->getContent() << std::endl;
        }
    } else if (auto logoutMsg = dynamic_cast<const cpp_chat::LogoutMessage*>(&msg)) {
        std::cout << "退出登录结果: " << (logoutMsg->getSuccess() ? "成功" : "失败") << " - " << logoutMsg->getMessage() << std::endl;
    } else {
        std::cout << "收到未知消息: Type=" << messageTypeToString(msg.getType()) << ", Content=" << msg.toJsonString() << std::endl;
    }
}

void connectionCallback(bool connected, const std::string& errorMsg) {
    if (connected) {
        std::cout << "连接成功" << std::endl;
    } else {
        std::cout << "连接状态: " << errorMsg << std::endl;
    }
}

int main() {
    // Set console to UTF-8 for Chinese characters
    initConsoleUTF8();
    
    cpp_chat::Client client;
    client.setMessageCallback(messageCallback);
    client.setConnectionCallback(connectionCallback);
    
    if (!client.connect("192.168.149.129", 8888)) {
        std::cerr << "无法连接到服务器" << std::endl;
        return 1;
    }
    
    std::cout << "客户端已启动。输入命令：" << std::endl;
    std::cout << "- register <username> <password>" << std::endl;
    std::cout << "- login <username> <password>" << std::endl;
    std::cout << "- friends" << std::endl;
    std::cout << "- private <toUser> <message>" << std::endl;
    std::cout << "- group <message>" << std::endl;
    std::cout << "- logout" << std::endl;
    std::cout << "- exit" << std::endl;
    
    std::string input;
    while (std::getline(std::cin, input)) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;
        
        if (command == "register") {
            std::string username, password;
            iss >> username >> password;
            client.registerUser(username, password);
        } else if (command == "login") {
            std::string username, password;
            iss >> username >> password;
            client.login(username, password);
        } else if (command == "friends") {
            client.getFriends();
        } else if (command == "private") {
            std::string toUser;
            iss >> toUser;
            std::string content;
            std::getline(iss, content); // 获取剩余作为消息，可能有空格
            if (!content.empty() && content[0] == ' ') content = content.substr(1); // 去除前导空格
            client.sendPrivateMessage(toUser, content);
        } else if (command == "group") {
            std::string content;
            std::getline(iss, content);
            if (!content.empty() && content[0] == ' ') content = content.substr(1);
            client.sendGroupMessage(content);
        } else if (command == "logout") {
            client.logout();
        } else if (command == "exit") {
            break;
        } else {
            std::cout << "未知命令。可用命令: register, login, friends, private, group, logout, exit" << std::endl;
        }
    }
    
    client.disconnect();
    return 0;
}