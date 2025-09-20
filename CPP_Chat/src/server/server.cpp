#include "server.h"
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <event2/buffer.h>
#include <event2/util.h>

namespace cpp_chat {

Server::Server(const std::string& ip, int port, int threadCount)
    : ip_(ip), port_(port), threadCount_(threadCount), running_(false),
      base_(nullptr), listener_(nullptr), nextWorkerIndex_(0) {
}

Server::~Server() {
    stop();
}

bool Server::start() {
    if (running_) return true;
    if (!database_.connect("localhost", "root", "123", "cpp_chat", 33060)) {
        std::cerr << "Failed to connect to database" << std::endl;
        return false;
    }
    base_ = event_base_new();
    if (!base_) {
        std::cerr << "Failed to create event_base" << std::endl;
        return false;
    }
    struct sockaddr_in sin{};
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(static_cast<uint16_t>(port_));
    if (evutil_inet_pton(AF_INET, ip_.c_str(), &sin.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << ip_ << std::endl;
        event_base_free(base_); base_ = nullptr;
        return false;
    }
    listener_ = evconnlistener_new_bind(base_, acceptCallback, this,
                                       LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,
                                       -1, (struct sockaddr*)&sin, sizeof(sin));
    if (!listener_) {
        int err = EVUTIL_SOCKET_ERROR();
        std::cerr << "Failed to create listener: " << evutil_socket_error_to_string(err) << std::endl;
        std::cerr << "  Trying to bind to: " << ip_ << ":" << port_ << std::endl;
        event_base_free(base_); base_ = nullptr;
        return false;
    }
    evconnlistener_set_error_cb(listener_, acceptErrorCallback);
    for (int i = 0; i < threadCount_; ++i) {
        workers_.emplace_back(new WorkerThread(this, i));
        workers_[i]->start();
    }
    running_ = true;
    std::cout << "Server started on " << ip_ << ":" << port_
              << " with " << threadCount_ << " worker threads" << std::endl;
    event_base_dispatch(base_);
    return true;
}

void Server::stop() {
    if (!running_) return;
    running_ = false;
    for (auto& w : workers_) w->stop();
    workers_.clear();
    if (listener_) { evconnlistener_free(listener_); listener_ = nullptr; }
    if (base_)     { event_base_free(base_);         base_     = nullptr; }
    database_.disconnect();
    std::cout << "Server stopped" << std::endl;
}

void Server::acceptCallback(struct evconnlistener*, evutil_socket_t fd,
                           struct sockaddr*, int, void* arg) {
    Server* s = static_cast<Server*>(arg);
    evutil_make_socket_nonblocking(fd);
    s->dispatchClient(fd);
}

void Server::acceptErrorCallback(struct evconnlistener*, void* arg) {
    Server* s = static_cast<Server*>(arg);
    int err = EVUTIL_SOCKET_ERROR();
    std::cerr << "Accept error: " << evutil_socket_error_to_string(err) << std::endl;
    s->stop();
}

void Server::dispatchClient(evutil_socket_t clientFd) {
    WorkerThread* w = getLeastLoadedWorker();
    if (w) {
        w->addClient(clientFd);
        std::lock_guard<std::mutex> lock(fdToWorkerMutex_);
        fdToWorker_[clientFd] = w;
    } else {
        std::cerr << "No worker available, closing client connection" << std::endl;
        evutil_closesocket(clientFd);
    }
}

WorkerThread* Server::getLeastLoadedWorker() {
    std::lock_guard<std::mutex> lock(workerMutex_);
    if (workers_.empty()) return nullptr;
    WorkerThread* w = workers_[nextWorkerIndex_].get();
    nextWorkerIndex_ = (nextWorkerIndex_ + 1) % workers_.size();
    return w;
}

WorkerThread* Server::getWorkerForFd(evutil_socket_t fd) {
    std::lock_guard<std::mutex> lock(fdToWorkerMutex_);
    auto it = fdToWorker_.find(fd);
    return it != fdToWorker_.end() ? it->second : nullptr;
}

void Server::unregisterClientFd(evutil_socket_t fd) {
    std::lock_guard<std::mutex> lock(fdToWorkerMutex_);
    fdToWorker_.erase(fd);
}


WorkerThread::WorkerThread(Server* server, int id)
    : server_(server), id_(id), running_(false), clientCount_(0),
      base_(nullptr), notifyEvent_(nullptr), notifyReceiveFd_(-1), notifySendFd_(-1) {}

WorkerThread::~WorkerThread() { stop(); }

void WorkerThread::start() {
    if (running_) return;
    evutil_socket_t fds[2];
    if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        std::cerr << "Failed to create socketpair: " << evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()) << std::endl;
        return;
    }
    notifyReceiveFd_ = fds[0];
    notifySendFd_    = fds[1];
    evutil_make_socket_nonblocking(notifyReceiveFd_);
    evutil_make_socket_nonblocking(notifySendFd_);
    running_ = true;
    thread_ = std::thread(&WorkerThread::threadMain, this);
    std::cout << "Worker thread " << id_ << " started" << std::endl;
}

void WorkerThread::stop() {
    if (!running_) return;
    running_ = false;
    if (notifySendFd_ != -1) {
        char q = 'q';
        send(notifySendFd_, &q, 1, 0);
    }
    if (thread_.joinable()) thread_.join();
    if (notifyReceiveFd_ != -1) {
        char buf = 'q';
        send(notifyReceiveFd_, &buf, 1, 0);
    }
    if (notifySendFd_    != -1) { evutil_closesocket(notifySendFd_);    notifySendFd_    = -1; }
    if (notifyEvent_)   { event_free(notifyEvent_); notifyEvent_ = nullptr; }
    if (base_)          { event_base_free(base_);   base_        = nullptr; }
    std::cout << "Worker thread " << id_ << " stopped" << std::endl;
}

void WorkerThread::addClient(evutil_socket_t clientFd) {
    if (notifySendFd_ == -1) return;
    char buf[sizeof(evutil_socket_t)+1];
    buf[0] = 'c';
    memcpy(buf+1, &clientFd, sizeof(clientFd));
    send(notifySendFd_, buf, sizeof(buf), 0);
}

void WorkerThread::threadMain() {
    base_ = event_base_new();
    if (!base_) {
        std::cerr << "Failed to create event_base for worker thread " << id_ << std::endl;
        return;
    }
    notifyEvent_ = event_new(base_, notifyReceiveFd_, EV_READ|EV_PERSIST,
                            &WorkerThread::notifyCallback, this);
    if (!notifyEvent_) {
        std::cerr << "Failed to create notify event for worker thread " << id_ << std::endl;
        event_base_free(base_); base_ = nullptr;
        return;
    }
    event_add(notifyEvent_, nullptr);
    while (running_) {
        event_base_loop(base_, EVLOOP_NONBLOCK);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void WorkerThread::handleNotify() {
    char buf[1024];
    ssize_t n = recv(notifyReceiveFd_, buf, sizeof(buf), 0);
    if (n <= 0) return;
    size_t i = 0;
    while (i < static_cast<size_t>(n)) {
        char type = buf[i++];
        if (type == 'q') { running_ = false; break; }
        if (type == 'c' && i + sizeof(evutil_socket_t) <= static_cast<size_t>(n)) {
            evutil_socket_t clientFd;
            memcpy(&clientFd, buf+i, sizeof(clientFd));
            i += sizeof(clientFd);
            struct bufferevent* bev = bufferevent_socket_new(base_, clientFd, BEV_OPT_CLOSE_ON_FREE);
            if (!bev) {
                std::cerr << "Failed to create bufferevent for client " << clientFd << std::endl;
                evutil_closesocket(clientFd);
                continue;
            }
            bufferevent_setcb(bev, &WorkerThread::readCallback, nullptr,
                              &WorkerThread::eventCallback, this);
            bufferevent_enable(bev, EV_READ|EV_WRITE);
            {
                std::lock_guard<std::mutex> lock(clientsMutex_);
                clients_[clientFd] = bev;
            }
            ++clientCount_;
            std::cout << "Worker thread " << id_ << " added client " << clientFd
                      << ", total clients: " << clientCount_ << std::endl;
        } else if (type == 's' && i + sizeof(evutil_socket_t) + sizeof(uint32_t) <= static_cast<size_t>(n)) {
            evutil_socket_t targetFd; uint32_t len;
            memcpy(&targetFd, buf+i, sizeof(targetFd)); i += sizeof(targetFd);
            memcpy(&len,      buf+i, sizeof(len));      i += sizeof(len);
            if (i + len > static_cast<size_t>(n)) { std::cerr << "Incomplete send notification" << std::endl; break; }
            std::string jsonStr(buf+i, len); i += len;
            uint32_t lenNetwork = htonl(len);  // 转换为网络字节序
            std::lock_guard<std::mutex> lock(clientsMutex_);
            auto it = clients_.find(targetFd);
            if (it != clients_.end()) {
                struct evbuffer* out = bufferevent_get_output(it->second);
                evbuffer_add(out, &lenNetwork, sizeof(lenNetwork));
                evbuffer_add(out, jsonStr.c_str(), len);
            } else {
                std::cerr << "Client " << targetFd << " not found in worker " << id_ << std::endl;
            }
        }
    }
}

void WorkerThread::processMessage(evutil_socket_t clientFd, const std::string& jsonStr) {
    try {
        auto msg = Message::createMessageFromJson(jsonStr);
        if (!msg) { std::cerr << "Failed to parse message: " << jsonStr << std::endl; return; }
        switch (msg->getType()) {
        case MessageType::REGISTER:
            handleRegister(clientFd, static_cast<const RegisterMessage&>(*msg)); break;
        case MessageType::LOGIN:
            handleLogin(clientFd, static_cast<const LoginMessage&>(*msg)); break;
        case MessageType::GET_FRIENDS:
            handleGetFriends(clientFd, static_cast<const GetFriendsMessage&>(*msg)); break;
        case MessageType::PRIVATE_CHAT:
            handlePrivateChat(clientFd, static_cast<const PrivateChatMessage&>(*msg)); break;
        case MessageType::GROUP_CHAT:
            handleGroupChat(clientFd, static_cast<const GroupChatMessage&>(*msg)); break;
        case MessageType::LOGOUT:
            handleLogout(clientFd, static_cast<const LogoutMessage&>(*msg)); break;
        default:
            std::cerr << "Unknown message type: " << static_cast<int>(msg->getType()) << std::endl; break;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in processMessage: " << e.what() << std::endl;
    }
}

void WorkerThread::handleRegister(evutil_socket_t clientFd, const RegisterMessage& msg) {
    Database* db = server_->getDatabase();
    bool ok = db->registerUser(msg.getUsername(), msg.getPassword());
    RegisterMessage resp(msg.getUsername(), msg.getPassword());
    resp.setSuccess(ok);
    resp.setMessage(ok ? "注册成功" : "注册失败，用户名已存在");
    sendToClient(clientFd, resp);
}

void WorkerThread::handleLogin(evutil_socket_t clientFd, const LoginMessage& msg) {
    Database* db = server_->getDatabase();
    std::cout << "[DEBUG] 收到登录请求: username=" << msg.getUsername() << ", password=" << msg.getPassword() << std::endl;
    
    bool ok = db->verifyLogin(msg.getUsername(), msg.getPassword());
    std::cout << "[DEBUG] 登录验证结果: " << (ok ? "成功" : "失败") << std::endl;
    
    LoginMessage resp(msg.getUsername(), msg.getPassword());
    resp.setSuccess(ok);
    if (ok) {
        db->setUserOnline(msg.getUsername(), clientFd);
        { std::lock_guard<std::mutex> lock(clientsMutex_); clientUsers_[clientFd] = msg.getUsername(); }
        resp.setMessage("登录成功");
        auto off = db->getOfflineMessages(msg.getUsername());
        if (!off.empty()) { resp.setOfflineMessages(off); db->clearOfflineMessages(msg.getUsername()); }
        std::cout << "[DEBUG] 用户 " << msg.getUsername() << " 登录成功，socket_fd=" << clientFd << std::endl;
    } else {
        resp.setMessage("登录失败，用户名或密码错误");
        std::cout << "[DEBUG] 用户 " << msg.getUsername() << " 登录失败" << std::endl;
    }
    
    std::cout << "[DEBUG] 发送登录响应: success=" << resp.getSuccess() << ", message=" << resp.getMessage() << std::endl;
    sendToClient(clientFd, resp);
}

void WorkerThread::handleGetFriends(evutil_socket_t clientFd, const GetFriendsMessage&) {
    Database* db = server_->getDatabase();
    GetFriendsMessage resp;
    resp.setAllUsers(db->getAllUsers());
    resp.setOnlineUsers(db->getOnlineUsers());
    resp.setSuccess(true);
    resp.setMessage("获取好友列表成功");
    sendToClient(clientFd, resp);
}

void WorkerThread::handlePrivateChat(evutil_socket_t clientFd, const PrivateChatMessage& msg) {
    Database* db = server_->getDatabase();
    std::string fromUser;
    { std::lock_guard<std::mutex> lock(clientsMutex_); auto it = clientUsers_.find(clientFd); if (it != clientUsers_.end()) fromUser = it->second; }
    if (fromUser.empty()) {
        PrivateChatMessage resp("", msg.getToUser());
        resp.setSuccess(false); resp.setMessage("发送失败，请先登录");
        sendToClient(clientFd, resp); return;
    }
    PrivateChatMessage forwardMsg(fromUser, msg.getToUser(), msg.getContent());
    forwardMsg.setSuccess(true); forwardMsg.setMessage("消息已发送");
    int toFd = db->getUserSocketFd(msg.getToUser());
    // 检查目标客户端是否在当前工作线程中
    { 
        std::lock_guard<std::mutex> lock(clientsMutex_);
        if (clients_.find(toFd) != clients_.end()) {
            // 在当前工作线程中，直接发送
            sendToClient(toFd, forwardMsg);
        } else {
            // 检查是否在其他工作线程中
            WorkerThread* w = server_->getWorkerForFd(toFd);
            if (w) {
                // 在其他工作线程中，通过notifySendMessage转发
                w->notifySendMessage(toFd, forwardMsg.toJsonString());
            } else {
                // 目标用户不在线，存储离线消息
                db->storeOfflineMessage(msg.getToUser(), forwardMsg.toJsonString());
            }
        }
    }
    // 发送给发送者的确认消息，不包含消息内容避免回显
    PrivateChatMessage resp("", msg.getToUser(), "");  // 不包含消息内容
    resp.setSuccess(true); resp.setMessage("消息已发送");
    sendToClient(clientFd, resp);
}

void WorkerThread::handleGroupChat(evutil_socket_t clientFd, const GroupChatMessage& msg) {
    Database* db = server_->getDatabase();
    std::string fromUser;
    { std::lock_guard<std::mutex> lock(clientsMutex_); auto it = clientUsers_.find(clientFd); if (it != clientUsers_.end()) fromUser = it->second; }
    if (fromUser.empty()) {
        GroupChatMessage resp("", "", msg.getContent());
        resp.setSuccess(false); resp.setMessage("发送失败，请先登录");
        sendToClient(clientFd, resp); return;
    }
    GroupChatMessage forwardMsg(fromUser, "", msg.getContent());
    forwardMsg.setSuccess(true); forwardMsg.setMessage("群消息已发送");
    for (const auto& user : db->getOnlineUsers()) {
        if (user == fromUser) continue;
        int toFd = db->getUserSocketFd(user);
        if (toFd != -1) sendToClient(toFd, forwardMsg);
    }
    // 发送给发送者的确认消息，不包含消息内容避免回显
    GroupChatMessage resp("", "", "");  // 不包含消息内容
    resp.setSuccess(true); resp.setMessage("群消息已发送");
    sendToClient(clientFd, resp);
}

void WorkerThread::handleLogout(evutil_socket_t clientFd, const LogoutMessage&) {
    Database* db = server_->getDatabase();
    std::string username;
    { std::lock_guard<std::mutex> lock(clientsMutex_); auto it = clientUsers_.find(clientFd); if (it != clientUsers_.end()) { username = it->second; clientUsers_.erase(it); } }
    if (!username.empty()) db->setUserOffline(username);
    LogoutMessage resp(""); resp.setSuccess(true); resp.setMessage("已成功退出登录");
    sendToClient(clientFd, resp);
}

void WorkerThread::notifySendMessage(evutil_socket_t targetFd, const std::string& jsonStr) {
    if (notifySendFd_ == -1) { std::cerr << "Notify send fd not available" << std::endl; return; }
    uint32_t len = jsonStr.length();
    size_t total = 1 + sizeof(targetFd) + sizeof(len) + len;
    std::unique_ptr<char[]> buf(new char[total]);
    size_t p = 0;
    buf[p++] = 's';
    memcpy(buf.get()+p, &targetFd, sizeof(targetFd)); p += sizeof(targetFd);
    memcpy(buf.get()+p, &len,      sizeof(len));      p += sizeof(len);
    memcpy(buf.get()+p, jsonStr.c_str(), len);
    send(notifySendFd_, buf.get(), total, 0);
}

void WorkerThread::sendToClient(evutil_socket_t clientFd, const Message& msg) {
    std::string jsonStr = msg.toJsonString();
    uint32_t len = jsonStr.length();
    uint32_t lenNetwork = htonl(len);  // 转换为网络字节序
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = clients_.find(clientFd);
        if (it != clients_.end()) {
            struct evbuffer* out = bufferevent_get_output(it->second);
            evbuffer_add(out, &lenNetwork, sizeof(lenNetwork));
            evbuffer_add(out, jsonStr.c_str(), len);
            return;
        }
    }
    WorkerThread* w = server_->getWorkerForFd(clientFd);
    if (w && w != this) w->notifySendMessage(clientFd, jsonStr);
    else std::cerr << "Cannot send to client " << clientFd << ": not found" << std::endl;
}

// 在命名空间内补上缺失实现（C++11 兼容）
void WorkerThread::removeClient(evutil_socket_t fd) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(fd);
    if (it != clients_.end()) {
        bufferevent_free(it->second);
        clients_.erase(it);
        --clientCount_;
    }
}

void WorkerThread::notifyCallback(evutil_socket_t, short, void* arg) {
    static_cast<WorkerThread*>(arg)->handleNotify();
}

void WorkerThread::readCallback(struct bufferevent* bev, void* arg) {
    auto* self = static_cast<WorkerThread*>(arg);
    struct evbuffer* in = bufferevent_get_input(bev);
    while (evbuffer_get_length(in) >= sizeof(uint32_t)) {
        uint32_t msgLen;
        evbuffer_copyout(in, &msgLen, sizeof(msgLen));
        // 转换网络字节序到主机字节序
        msgLen = ntohl(msgLen);
        if (evbuffer_get_length(in) < sizeof(uint32_t) + msgLen) break;
        evbuffer_drain(in, sizeof(uint32_t));
        std::string jsonStr(msgLen, '\0');
        evbuffer_remove(in, &jsonStr[0], msgLen);
        self->processMessage(bufferevent_getfd(bev), jsonStr);
    }
}

void WorkerThread::eventCallback(struct bufferevent* bev, short events, void* arg) {
    auto* worker = static_cast<WorkerThread*>(arg);
    evutil_socket_t fd = bufferevent_getfd(bev);
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        if (events & BEV_EVENT_ERROR) {
            int err = EVUTIL_SOCKET_ERROR();
            std::cerr << "Client " << fd << " error: "
                      << evutil_socket_error_to_string(err) << std::endl;
        }
        worker->removeClient(fd);
        bufferevent_free(bev);
    }
}
}