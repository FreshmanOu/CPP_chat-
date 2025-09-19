#pragma once
#include <string>
#include <fstream>
#include "TcpServer.h"
#include "../interface/SecKeyShm.h"
#include "mysqlOP.h"
#include "ThreadPool.h"
#include <iostream>
#include"message.pb.h"
#include"../interface/interface.h"
#include<pthread.h>
#include"SeckKeyNodeInfo.h"
#include <cstdio>
#include <locale>
#include <codecvt>
class ServerOP
{
public:
    // 构造函数
    ServerOP(std::string jsonFile);
    // 析构函数
    ~ServerOP();
    // 启动服务器
    void startServer();
    void stopServer();
private:

    // 线程处理函数
    static void working(void* arg, void* client);
    // 密钥协商服务器线程函数
    static void* seckeyServerThread(void* arg);
    // 密钥协商
    std::string seckeyAgree(std::string clientID, std::string serverID, std::string randStr, NodeSecKeyInfo& keyInfo);
    // 生成随机密钥
    std::string getRandKey();

private:
    // 服务器ID
    std::string m_serverID;
    // 数据库主机
    std::string m_dbHost;
    // 数据库端口
    int m_dbPort;
    // 数据库用户
    std::string m_dbUser;
    // 数据库密码
    std::string m_dbPasswd;
    // 数据库名称
    std::string m_dbName;
    // 密钥协商端口
    int m_seckeyPort;
    // 共享内存键值
    std::string m_shmKey;
    // 最大节点数
    int m_maxNode;
    // 共享内存
    SecKeyShm* m_shm;
    // MySQL数据库
    MySQLOP* m_mysql;
    // 密钥协商服务器
    TcpServer* m_seckeyServer;

    // 线程池
    ThreadPool* m_pool;
    // 线程同步机制
    pthread_mutex_t m_mutex;      // Mutex for data protection
    pthread_cond_t m_cond;        // Condition variable for thread signaling
    bool m_isShutdown;            // Flag to indicate server shutdown
    pthread_t m_seckeyThreadId;   // ID of the seckey server thread
};