#pragma once
#include"TcpSocket.h"

class TcpServer{
public:
    TcpServer();
    ~TcpServer();
    // 服务器设置监听
	int setListen(unsigned short port);
	// 等待并接受客户端连接请求, 默认连接超时时间为10000s
	TcpSocket* acceptConn(int timeout1= 10000);
	void closefd();
private:
	int m_listenfd;
};