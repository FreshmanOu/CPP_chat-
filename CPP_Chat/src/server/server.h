#ifndef CPP_CHAT_SERVER_H
#define CPP_CHAT_SERVER_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include "../common/message.h"
#include "database.h"

namespace cpp_chat {

// 前向声明
class WorkerThread;

class Server {
public:
 
    Server(const std::string& ip, int port, int threadCount);
    ~Server();
    bool start();
    void stop();
    Database* getDatabase() { return &database_; }

    // 新增：根据fd获取对应的Worker（供跨线程发送使用）
    WorkerThread* getWorkerForFd(evutil_socket_t fd);

    // 新增：注销fd到worker的映射（客户端断开时调用）
    void unregisterClientFd(evutil_socket_t fd);

private:
    // 服务器配置
    std::string ip_;
    int port_;
    int threadCount_;
    
    // 服务器状态
    std::atomic<bool> running_;
    
    // 数据库接口
    Database database_;
    
    // libevent相关
    struct event_base* base_;
    struct evconnlistener* listener_;
    
    // 工作线程管理
    std::vector<std::unique_ptr<WorkerThread>> workers_;
    std::mutex workerMutex_;
    int nextWorkerIndex_;
    
    // 全局fd到worker的映射
    std::unordered_map<evutil_socket_t, WorkerThread*> fdToWorker_;
    std::mutex fdToWorkerMutex_;
    
    // 静态回调函数
    static void acceptCallback(struct evconnlistener* listener, evutil_socket_t fd,
                              struct sockaddr* addr, int socklen, void* arg);
    static void acceptErrorCallback(struct evconnlistener* listener, void* arg);
    
    // 分发客户端连接到工作线程
    void dispatchClient(evutil_socket_t clientFd);
    
    // 获取负载最小的工作线程
    WorkerThread* getLeastLoadedWorker();
};


class WorkerThread {
public:
    // @param id 线程ID
    
    WorkerThread(Server* server, int id);
    
    ~WorkerThread();

    void start();

    void stop();
    
    void addClient(evutil_socket_t clientFd);
    
    int getClientCount() const { return clientCount_; }

    int getId() const { return id_; }

private:
    // 所属服务器
    Server* server_;
    
    // 线程ID
    int id_;
    
    // 线程状态
    std::atomic<bool> running_;
    std::thread thread_;
    
    // 客户端计数
    std::atomic<int> clientCount_;
    
    // libevent相关
    struct event_base* base_;
    struct event* notifyEvent_;
    evutil_socket_t notifyReceiveFd_;
    evutil_socket_t notifySendFd_;
    
    // 客户端连接管理
    std::unordered_map<evutil_socket_t, struct bufferevent*> clients_;
    std::unordered_map<evutil_socket_t, std::string> clientUsers_;
    std::mutex clientsMutex_;
    
    // 线程主函数
    void threadMain();
    
    // 处理通知事件
    void handleNotify();
    
    // 处理客户端消息
    void processMessage(evutil_socket_t clientFd, const std::string& jsonStr);
    
    // 处理注册消息
    void handleRegister(evutil_socket_t clientFd, const RegisterMessage& msg);
    
    // 处理登录消息
    void handleLogin(evutil_socket_t clientFd, const LoginMessage& msg);
    
    // 处理获取好友列表消息
    void handleGetFriends(evutil_socket_t clientFd, const GetFriendsMessage& msg);
    
    // 处理私聊消息
    void handlePrivateChat(evutil_socket_t clientFd, const PrivateChatMessage& msg);
    
    // 处理群聊消息
    void handleGroupChat(evutil_socket_t clientFd, const GroupChatMessage& msg);
    
    // 处理登出消息
    void handleLogout(evutil_socket_t clientFd, const LogoutMessage& msg);
    
    // 发送消息给客户端（支持跨worker）
    void sendToClient(evutil_socket_t clientFd, const Message& msg);

    // 用于接收其他worker发送通知并向本worker客户端发送消息
    void notifySendMessage(evutil_socket_t targetFd, const std::string& jsonStr);
    
    // 移除客户端连接
    void removeClient(evutil_socket_t clientFd);
    

    static void notifyCallback(evutil_socket_t fd, short events, void* arg);
    static void readCallback(struct bufferevent* bev, void* arg);
    static void eventCallback(struct bufferevent* bev, short events, void* arg);
};

} // namespace cpp_chat

#endif // CPP_CHAT_SERVER_H