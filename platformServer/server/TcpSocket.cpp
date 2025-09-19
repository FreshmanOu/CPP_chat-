#include "TcpSocket.h"

TcpSocket::TcpSocket()
{
}

TcpSocket::TcpSocket(int sfd)
{
    this->sfd = sfd;
}

TcpSocket::~TcpSocket()
{
    
}

int TcpSocket::connectToHost(std::string ip, unsigned short port, int timeout)
{
    int ret = 0;
	if (port <= 0 || port > 65535 || timeout < 0)
	{
		ret = ParamError;
		return ret;
	}

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
	{
		ret = errno;
		printf("func socket() err:  %d\n", ret);
		return ret;
	}
		// 在连接前检查socket状态
	if (checkSocketStatus() != 0)
	{
		printf("connectToHost: Socket status check failed\n");
		close(sfd);
		sfd= -1;
		return -1;
	}
    struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = inet_addr(ip.data());

	ret = connectTimeout((struct sockaddr_in*) (&servaddr), (unsigned int)timeout);
	if (ret < 0)
	{
		// 超时
		if (ret == -1 && errno == ETIMEDOUT)
		{
			ret = TimeoutError;
			return ret;
		}
		else
		{
			printf("connectTimeout 调用异常, 错误号: %d\n", errno);
			return errno;
		}
	}

	return ret;
}

int TcpSocket::sendMsg(std::string sendmsg, int timeout)
{
     // 返回0->没超时, 返回-1->超时
		// 检查socket是否有效
	if (sfd < 0)
	{
		printf("sendMsg: Invalid socket descriptor: %d\n", sfd);
		return -1;
	}

	// 返回0->没超时, 返回-1->超时
	int ret = writeTimeout(timeout);
	if (ret == 0)
		{
				int writed = 0;
				int dataLen = sendmsg.size() + 4;
				// 添加的4字节作为数据头, 存储数据块长度
				unsigned char *netdata = (unsigned char *)malloc(dataLen);
				if (netdata == NULL)
				{
						ret = MallocError;
						printf("func sckClient_send() mlloc Err:%d\n ", ret);
						return ret;
				}
				// 转换为网络字节序
				int netlen = htonl(sendmsg.size());
				memcpy(netdata, &netlen, 4);
				memcpy(netdata + 4, sendmsg.data(), sendmsg.size());
                // 没问题返回发送的实际字节数, 应该 == 第二个参数: dataLen
				// 失败返回: -1
				writed = writeN(netdata, dataLen);//write natelen changdu de 
				// 释放内存
				if (netdata != NULL) 
				{
						free(netdata);
						netdata = NULL;
				}
				
				if (writed < dataLen) // 发送失败
				{
						printf("sendMsg: Failed to send complete data, writed=%d, expected=%d\n", writed, dataLen);
						return writed;
				}
				
				// 发送成功，返回实际发送的字节数
				return writed;
		}
		else
		{
				//失败返回-1，超时返回-1并且errno = ETIMEDOUT
				if (ret == -1 && errno == ETIMEDOUT)
				{
						ret = TimeoutError;
						printf("func sckClient_send() timeout error: %d\n ", ret);
				}
				else
				{
						printf("func sckClient_send() writeTimeout error: %d\n ", ret);
				}
		}
    
    return ret;
}

std::string TcpSocket::recvMsg(int timeout)
{
	
    // 返回0 -> 没超时就接收到了数据, -1, 超时或有异常
	int ret = readTimeout(timeout); 
	if (ret != 0)
	{
		return std::string();
	}


    //successful
    int netdatalen=0;

    ret=readN(&netdatalen,4);//读包头 4个字节
	std::cout<<ret<<std::endl;
    if (ret == -1)
	{
		printf("recvMsg:timeout");
		
		return std::string();
	}
	else if(ret<4)
	{
		printf("recvMsg: Peer closed connection while reading header, got %d of 4 bytes\n", ret);
		return std::string();
	}

    int n=ntohl(netdatalen);//将网络字节序转换为本地字节序
	if (n <= 0 || n > 1024*1024)  // 限制最大消息长度（根据需求调整）
    {
        printf("recvMsg: Invalid message length %d (invalid range)\n", n);
        return std::string();
    }
	// 根据包头中记录的数据大小申请内存, 接收数据
    char*buf=(char*)malloc(n+1);
    if(buf==nullptr){
		perror("recvMsg: malloc failed");
        return std::string();  // 返回空字符串而非 nullptr
    }

    ret=readN(buf,n);////根据长度读数据
    if (ret == -1)
	{
		 int saveErrno = errno;
        printf("recvMsg: Read data error: %s\n", strerror(saveErrno));
        free(buf);
        return std::string();
	}
	else if (ret < n)
	{
		printf("recvMsg: Peer closed while reading data (got %d of %d bytes)\n", ret, n);
        free(buf);
        return std::string();
	}

     // 构造字符串并返回
    buf[n] = '\0';  // 确保字符串终止
    std::string recvmsg(buf);
    free(buf);
    return recvmsg;
}
	

void TcpSocket::disconnect()
{
   	if (sfd >= 0)
	{
		// 在关闭连接前检查socket状态
		if (checkSocketStatus() != 0)
		{
			printf("disconnect: Socket status check failed, closing anyway\n");
		}
		close(sfd);
		sfd = -1;
	}
}

/////////////////////////////////////////////////
//////             子函数                   //////
/////////////////////////////////////////////////
/*
* checkSocketStatus - 检查socket状态
* 返回0表示socket正常，-1表示socket有问题
*/
int TcpSocket::checkSocketStatus()
{
	if (sfd< 0)
	{
		printf("checkSocketStatus: Invalid socket descriptor\n");
		return -1;
	}
	
	int error = 0;
	socklen_t len = sizeof(error);
	if (getsockopt(sfd, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
	{
		printf("checkSocketStatus: getsockopt failed: %s (errno: %d)\n", strerror(errno), errno);
		return -1;
	}
	
	if (error != 0)
	{
		printf("checkSocketStatus: Socket error: %s (errno: %d)\n", strerror(error), error);
		return -1;
	}
	
	return 0;
}

// 在TcpSocket中，select 不是用于并发处理,ershi超时检测
int TcpSocket::setNonBlock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags ==-1)
    {
        return flags;

    }
    flags|= O_NONBLOCK;
    int ret = fcntl(fd, F_SETFL, flags);
    return ret;
}

int TcpSocket::setBlock(int fd)
{
    int ret =0;
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags==-1){
        ret = flags;
    }
    flags &=~O_NONBLOCK;
    flags=fcntl(fd,F_SETFL,flags);
    return ret;
}
// readTimeout - 读超时检测函数，不含读操作
int TcpSocket::readTimeout(unsigned int wait_seconds)
{
	// 在超时检测前检查socket状态
	if (checkSocketStatus() != 0)
	{
		printf("readTimeout: Socket status check failed\n");
		return -1;
	}
    int ret = 0;
	if (wait_seconds > 0)
	{
		fd_set read_fdset;
		struct timeval timeout;

		FD_ZERO(&read_fdset);
		FD_SET(sfd, &read_fdset);

		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;

		//select返回值三态
		//1 若timeout时间到（超时），没有检测到读事件 ret返回=0
		//2 若ret返回<0 &&  errno == EINTR 说明select的过程中被别的信号中断（可中断睡眠原理）
		//2-1 若返回-1，select出错
		//3 若ret返回值>0 表示有read事件发生，返回事件发生的个数

		do
		{
			ret = select(sfd+ 1, &read_fdset, NULL, NULL, &timeout);

		} while (ret < 0 && errno == EINTR);

		if (ret == 0)
		{
			// 保存原始errno值
								int saveErrno = errno;
								ret = -1;
								errno = ETIMEDOUT;
								printf("readTimeout: timeout after %d seconds\n", wait_seconds);
		}
		else if (ret == 1)
		{
			ret = 0;
		}
	}

	return ret;
}

int TcpSocket::writeTimeout(unsigned int wait_seconds)
{
	// 在超时检测前检查socket状态
	if (checkSocketStatus() != 0)
	{
		printf("writeTimeout: Socket status check failed\n");
		return -1;
	}
    int ret = 0;
	if (wait_seconds > 0)
	{
		fd_set write_fdset;
		struct timeval timeout;

		FD_ZERO(&write_fdset);
		FD_SET(sfd, &write_fdset);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
            // 一但连接建立，则套接字就可读 所以connect_fdset放在了读集合中
			ret = select(sfd + 1, NULL, &write_fdset, NULL, &timeout);

		} while (ret < 0 && errno == EINTR);
		// 超时
		if (ret == 0)
		{
			ret = -1;
			errno = ETIMEDOUT;
		}
		else if (ret == 1)
		{
			ret = 0;	// 没超时
			errno = 0; // 清除errno，确保调用者不会看到旧的错误值
		}
	}

	return ret;
}

int TcpSocket::connectTimeout(sockaddr_in *addr, unsigned int wait_seconds)
{
	int ret;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	if (wait_seconds > 0)
	{
		setNonBlock(sfd);	// 设置非阻塞IO
		printf("connectTimeout: Set socket to non-blocking mode\n");
	}

	ret = connect(sfd, (struct sockaddr*)addr, addrlen);
	// 非阻塞模式连接, 返回-1, 并且errno为EINPROGRESS, 表示连接正在进行中
	if (ret < 0 && errno == EINPROGRESS)
	{
		printf("connectTimeout: 非阻塞模式连接, 返回-1, 并且errno为EINPROGRESS, 表示连接正在进行中\n");
	}else if (ret < 0)
	{
		// 其他连接错误
		printf("connectTimeout: 其他连接错误\n");
		return ret;
	}
	else if (ret == 0)
	{
		// 立即连接成功（通常在阻塞模式下）
		printf("connectTimeout: 立即连接成功（通常在阻塞模式下）\n");
		if (wait_seconds > 0)
		{
			setBlock(sfd); // 确保设置为阻塞模式
		}
		return ret;
	}

	// 处理非阻塞模式下的连接完成
	if (ret < 0 && errno == EINPROGRESS)
	{
		fd_set connect_fdset;
		struct timeval timeout;
		FD_ZERO(&connect_fdset);
		FD_SET(sfd, &connect_fdset);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			// 一但连接建立，则套接字就可写 所以connect_fdset放在了写集合中
			ret = select(sfd + 1, NULL, &connect_fdset, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);

		if (ret == 0)
		{
			// 超时
			printf("connectTimeout:// 超时 \n");
			ret = -1;
			errno = ETIMEDOUT;
		}
		else if (ret < 0)
		{
			printf("connectTimeout: select error: %s\n", strerror(errno));
			return -1;
		}
		else if (ret == 1)
		{
			/* ret返回为1（表示套接字可写），可能有两种情况，一种是连接建立成功，一种是套接字产生错误，*/
			/* 此时错误信息不会保存至errno变量中，因此，需要调用getsockopt来获取。 */
			int err;
			socklen_t sockLen = sizeof(err);
			int sockoptret = getsockopt(sfd, SOL_SOCKET, SO_ERROR, &err, &sockLen);
			if (sockoptret == -1)
			{
				printf("connectTimeout: getsockopt error: %s\n", strerror(errno));
				return -1;
			}
			if (err == 0)
			{
				printf("connectTimeout: Connection established successfully\n");
				ret = 0;	// 成功建立连接
				// 连接成功后立即设置回阻塞模式
				if (wait_seconds > 0)
				{
					setBlock(sfd);
				}
				
				// 给socket一点时间来完全就绪，避免立即读取时出现EINPROGRESS错误
				usleep(50000); // 50ms延迟
				
				// 验证socket是否真的就绪，尝试一个非阻塞的peek操作
				char testBuf[1];
				int peekRet = recv(sfd, testBuf, 1, MSG_PEEK | MSG_DONTWAIT);
				if (peekRet < 0)
				{
					int peekErrno = errno;
					if (peekErrno == EAGAIN || peekErrno == EWOULDBLOCK)
					{
						printf("connectTimeout: Socket verification successful (no data available yet)\n");
					}
					else
					{
						printf("connectTimeout: Socket verification failed: %s\n", strerror(peekErrno));
					}
				}
				else
				{
					printf("connectTimeout: Socket verification successful (data available)\n");
				}
			}
			else
			{
				// 连接失败
				printf("connectTimeout: Connection failed with error: %s\n", strerror(err));
				errno = err;
				ret = -1;
			}
		}
	}
	// 无论连接成功或失败，只要使用了非阻塞模式，都需要设置回阻塞模式
	if (wait_seconds > 0)
	{
		setBlock(sfd);	// 套接字设置回阻塞模式
		printf("connectTimeout: Restored socket to blocking mode\n");
	}
	
	if (ret == 0)
	{
		printf("connectTimeout: Connection completed successfully\n");
	}
	else
	{
		printf("connectTimeout: Connection failed with errno: %d (%s)\n", errno, strerror(errno));
	}
	
	return ret;
}

int TcpSocket::readN(void *buf, int count)
{
    size_t nleft = count;       // 剩余未读字节数
    size_t nread = 0;
    char* buf1 = (char*)buf;

    while (nleft > 0) {
        // 先执行read并赋值给nread，再判断是否为错误（<0）
        if ((nread = read(sfd, buf1, nleft)) < 0) { 
			printf("read wrong");
            return -1;
            
        } 
		else if (nread == 0) {
            return count - nleft;
        } 
		else {
            // 正常读取到数据，移动指针并减少剩余字节数
            buf1 += nread;
            nleft -= nread;
        }
    }
    // 读取完所有请求的字节数
    return count;
}
//
int TcpSocket::writeN(const void *buf, int count)
{
	// 在写入数据前检查socket状态
	if (checkSocketStatus() != 0)
	{
		disconnect();
		errno = EIO; // 设置适当的错误码
		return -1;
	}
    	size_t nleft = count;
		ssize_t nwritten;
		char *bufp = (char*)buf;
		int retry_count = 0;
		const int MAX_RETRY = 10;
		int sleep_time = 1000; // 初始休眠时间1毫秒

		while (nleft > 0)
		{
				if ((nwritten = write(sfd, bufp, nleft)) < 0)
				{
						if (errno == EINTR) // 被信号打断
						{
								continue;
						}
						else if (errno == EAGAIN || errno == EWOULDBLOCK)
						{
							// 指数退避重试策略
							if (retry_count >= MAX_RETRY)
							{
									printf("writen() failed after max retries, errno: %d\n", errno);
									return -1;
							}
							
							// 短暂休眠后重试，使用指数退避
							usleep(sleep_time);
							sleep_time *= 2; // 指数增加休眠时间
							retry_count++;
							continue;
					}
					else if (errno == EINPROGRESS) // 操作正在进行中
					{
							// 指数退避重试策略
							if (retry_count >= MAX_RETRY)
							{
									printf("writen() failed after max retries, errno: %d (EINPROGRESS)\n", errno);
									return -1;
							}
							
							printf("writen() operation in progress, retrying... (attempt %d/%d)\n", retry_count + 1, MAX_RETRY);
							usleep(sleep_time);
							sleep_time *= 2;
							retry_count++;
							continue;
				}
					else
				{
					printf("writen() failed with errno: %d\n", errno);
					return -1;
				}
			}
			else if (nwritten == 0)
			{
				continue;
		}

		bufp += nwritten;
		nleft -= nwritten;
		// 重置重试计数和休眠时间
		retry_count = 0;
		sleep_time = 1000;
	}

	return count;
}


