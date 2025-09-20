#include "client.h"
#include <iostream>
#include <cstring>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif

namespace cpp_chat {

Client::Client() : connected_(false), serverPort_(0), socketFd_(-1), running_(false) {
#ifdef _WIN32
    // 初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
    }
#endif
}

Client::~Client() {
    disconnect();
    
#ifdef _WIN32
    // 清理Winsock
    WSACleanup();
#endif
}

bool Client::connect(const std::string& ip, int port) {
    // 如果已连接，先断开
    if (connected_) {
        disconnect();
    }
    
    serverIp_ = ip;
    serverPort_ = port;
    
    // 创建socket
#ifdef _WIN32
    socketFd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFd_ == INVALID_SOCKET) {
        triggerConnectionCallback(false, "Failed to create socket: " + std::to_string(WSAGetLastError()));
        return false;
    }
#else
    socketFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd_ == -1) {
        triggerConnectionCallback(false, "Failed to create socket: " + std::string(strerror(errno)));
        return false;
    }
#endif
    
    // 设置服务器地址
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
#ifdef _WIN32
    inet_pton(AF_INET, ip.c_str(), &(serverAddr.sin_addr));
    
    // 连接服务器
    if (::connect(socketFd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        triggerConnectionCallback(false, "Failed to connect to server: " + std::to_string(WSAGetLastError()));
        closesocket(socketFd_);
        socketFd_ = INVALID_SOCKET;
        return false;
    }
#else
    inet_pton(AF_INET, ip.c_str(), &(serverAddr.sin_addr));
    
    // 连接服务器
    if (::connect(socketFd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        triggerConnectionCallback(false, "Failed to connect to server: " + std::string(strerror(errno)));
        close(socketFd_);
        socketFd_ = -1;
        return false;
    }
#endif
    
    // 设置连接状态
    connected_ = true;
    running_ = true;
    
    // 启动接收线程
    receiveThread_ = std::thread(&Client::receiveThreadFunc, this);
    
    // 触发连接回调
    triggerConnectionCallback(true, "Connected to server");
    
    return true;
}

void Client::disconnect() {
    if (!connected_) {
        return;
    }
    
    // 停止接收线程
    running_ = false;
    
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }
    
    // 关闭socket
#ifdef _WIN32
    if (socketFd_ != INVALID_SOCKET) {
        closesocket(socketFd_);
        socketFd_ = INVALID_SOCKET;
    }
#else
    if (socketFd_ != -1) {
        close(socketFd_);
        socketFd_ = -1;
    }
#endif
    
    // 设置连接状态
    connected_ = false;
    
    // 触发连接回调
    triggerConnectionCallback(false, "Disconnected from server");
}

bool Client::sendMessage(const Message& msg) {
    if (!connected_) {
        return false;
    }
    
    // 序列化消息
    std::string jsonStr = msg.toJsonString();
    
    // 发送消息长度前缀（4字节）
    uint32_t len = jsonStr.length();
    // 转换主机字节序到网络字节序
    uint32_t lenNetwork = htonl(len);
    
#ifdef _WIN32
    if (send(socketFd_, reinterpret_cast<const char*>(&lenNetwork), sizeof(lenNetwork), 0) != sizeof(lenNetwork)) {
        std::cerr << "Failed to send message length: " << WSAGetLastError() << std::endl;
        return false;
    }
    
    // 发送消息内容
    if (send(socketFd_, jsonStr.c_str(), jsonStr.length(), 0) != jsonStr.length()) {
        std::cerr << "Failed to send message content: " << WSAGetLastError() << std::endl;
        return false;
    }
#else
    if (send(socketFd_, &lenNetwork, sizeof(lenNetwork), 0) != sizeof(lenNetwork)) {
        std::cerr << "Failed to send message length: " << strerror(errno) << std::endl;
        return false;
    }
    
    // 发送消息内容
    if (send(socketFd_, jsonStr.c_str(), jsonStr.length(), 0) != static_cast<ssize_t>(jsonStr.length())) {
        std::cerr << "Failed to send message content: " << strerror(errno) << std::endl;
        return false;
    }
#endif
    
    return true;
}

void Client::setMessageCallback(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    messageCallback_ = callback;
}

void Client::setConnectionCallback(ConnectionCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    connectionCallback_ = callback;
}

bool Client::isConnected() const {
    return connected_;
}

void Client::registerUser(const std::string& username, const std::string& password) {
    RegisterMessage msg(username, password);
    sendMessage(msg);
}

void Client::login(const std::string& username, const std::string& password) {
    // 保存当前登录用户名
    currentUsername_ = username;
    
    LoginMessage msg(username, password);
    sendMessage(msg);
}

void Client::getFriends() {
    // 使用保存的当前登录用户名
    GetFriendsMessage msg(currentUsername_);
    sendMessage(msg);
}

void Client::sendPrivateMessage(const std::string& toUser, const std::string& content) {
    // 使用保存的当前登录用户名作为发送者
    PrivateChatMessage msg(currentUsername_, toUser, content);
    sendMessage(msg);
}

void Client::sendGroupMessage(const std::string& content) {
    // 使用保存的当前登录用户名作为发送者
    // 这里toUsers参数仍使用空字符串，实际应用中应该有选择群组的功能
    GroupChatMessage msg(currentUsername_, "", content);
    sendMessage(msg);
}

void Client::logout() {
    // 使用保存的当前登录用户名
    LogoutMessage msg(currentUsername_);
    sendMessage(msg);
}

void Client::receiveThreadFunc() {
    const int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    std::string messageBuffer;
    uint32_t expectedLength = 0;
    bool readingHeader = true;
    
    while (running_ && connected_) {
        // 接收数据
#ifdef _WIN32
        int bytesRead = recv(socketFd_, buffer, BUFFER_SIZE, 0);
        if (bytesRead == SOCKET_ERROR || bytesRead == 0) {
            if (bytesRead == SOCKET_ERROR) {
                std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
            }
            break;
        }
#else
        ssize_t bytesRead = recv(socketFd_, buffer, BUFFER_SIZE, 0);
        if (bytesRead <= 0) {
            if (bytesRead < 0) {
                std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
            }
            break;
        }
#endif
        
        // 处理接收到的数据
        for (size_t i = 0; i < static_cast<size_t>(bytesRead);) {
            if (readingHeader) {
                // 读取消息长度前缀
                if (i + sizeof(uint32_t) <= static_cast<size_t>(bytesRead)) {
                    // 确保memcpy操作不会超出缓冲区边界
                    if (i + sizeof(uint32_t) <= static_cast<size_t>(BUFFER_SIZE)) {
                        memcpy(&expectedLength, buffer + i, sizeof(uint32_t));
                        expectedLength = ntohl(expectedLength);  // 转换网络字节序到主机字节序
                        i += sizeof(uint32_t);
                        readingHeader = false;
                        messageBuffer.clear();
                    } else {
                        // 不完整的头部，等待更多数据
                        break;
                    }
                } else {
                    // 不完整的头部，等待更多数据
                    break;
                }
            } else {
                // 读取消息内容
                int remainingBytes = bytesRead - i;
                int bytesToCopy = std::min(remainingBytes, static_cast<int>(expectedLength - messageBuffer.size()));
                
                messageBuffer.append(buffer + i, bytesToCopy);
                i += bytesToCopy;
                
                // 检查是否已读取完整消息
                if (messageBuffer.size() == expectedLength) {
                    // 处理完整消息
                    handleMessage(messageBuffer);
                    
                    // 重置状态，准备读取下一条消息
                    readingHeader = true;
                }
            }
        }
        
        // 短暂休眠，避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 如果线程退出但客户端仍认为已连接，则断开连接
    if (connected_) {
        connected_ = false;
        triggerConnectionCallback(false, "Connection lost");
    }
}

void Client::handleMessage(const std::string& jsonStr) {
    try {
        // 解析消息
        std::unique_ptr<Message> msg = Message::createMessageFromJson(jsonStr);
        if (!msg) {
            std::cerr << "Failed to parse message: " << jsonStr << std::endl;
            return;
        }
        
        // 触发消息回调
        triggerMessageCallback(*msg);
    } catch (const std::exception& e) {
        std::cerr << "Exception in handleMessage: " << e.what() << std::endl;
    }
}

void Client::triggerMessageCallback(const Message& msg) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (messageCallback_) {
        messageCallback_(msg);
    }
}

void Client::triggerConnectionCallback(bool connected, const std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (connectionCallback_) {
        connectionCallback_(connected, errorMsg);
    }
}

} // namespace cpp_chat