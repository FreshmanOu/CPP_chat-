#include "TcpServer.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
TcpServer::TcpServer()
{
    m_listenfd = -1;
}

TcpServer::~TcpServer()
{
}

int TcpServer::setListen(unsigned short port)
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0)
    {
        perror("socket create failed");
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int on = 1;
    int ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, 4);
    if (ret == -1)
    {
        perror("setsockopt failed");
        return -1;
        /* code */
    }
    ret = bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        perror("bind failed");
        return -1;
    }

    ret = listen(sfd, 10);
    if (ret < 0)
    {
        perror("listen failed");
        return -1;
    }
    m_listenfd = sfd;//为了传递sfd，再下面的acceptConn里的FD_SET使用
    return ret;
}

TcpSocket *TcpServer::acceptConn(int timeout1)
{
    int ret;
	if (timeout1 > 0)
	{
		fd_set accept_fdset;
		struct timeval timeout;
		FD_ZERO(&accept_fdset);
		FD_SET(m_listenfd, &accept_fdset);
		timeout.tv_sec = timeout1;
		timeout.tv_usec = 0;
		do
		{
			// 检测读集合
			ret = select(m_listenfd + 1, &accept_fdset, NULL, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);	// 被信号中断, 再次进入循环
		if (ret <= 0)
		{
			return NULL;
		}
	}

	// 一但检测出 有select事件发生，表示对等方完成了三次握手，客户端有新连接建立
	// 此时再调用accept将不会堵塞
	struct sockaddr_in addrCli;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	int connfd = accept(m_listenfd, (struct sockaddr*)&addrCli, &addrlen); //返回已连接套接字
	if (connfd == -1)
	{
		perror("TCPServer :accept failed");
		return NULL;
	}

    return new TcpSocket(connfd);//返回已连接套接字de TcpSocket对象
}

void TcpServer::closefd()
{
    close(m_listenfd);
}
