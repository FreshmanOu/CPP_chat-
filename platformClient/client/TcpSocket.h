#pragma once
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

static const int TIMEOUT = 10000; //10s
class TcpSocket{
public:
    enum SocketError {
    ParamError = 3001,
    TimeoutError ,
    PeerCloseError,
    MallocError
};

    TcpSocket();
    TcpSocket(int sfd);
    ~TcpSocket();

    // 连接到服务器
    int connectToHost(std::string ip, unsigned short port,int timeout=TIMEOUT);
    // 发送消息
    int sendMsg(std::string msg,int timeout=TIMEOUT);
    // 接收消息
    std::string recvMsg(int timeout=TIMEOUT);
    
    void disconnect();

private:
    int checkSocketStatus();
    int sfd;
    // 设置I/O为非阻塞模式
	int setNonBlock(int fd);
	// 设置I/O为阻塞模式
	int setBlock(int fd);
	// 读超时检测函数，不含读操作
	int readTimeout(unsigned int wait_seconds);
	// 写超时检测函数, 不包含写操作
	int writeTimeout(unsigned int wait_seconds);
	// 带连接超时的connect函数
	int connectTimeout(struct sockaddr_in *addr, unsigned int wait_seconds);
	// 每次从缓冲区中读取n个字符
    int readN(void*buf,int count);
    // 每次向缓冲区中写入n个字符
    int writeN(const void*buf,int count);

};
