#pragma once
#include <string>
#include "TcpSocket.h"
#include "../interface/interface.h"
#include<mutex>
#include<condition_variable>
#include<thread>
class CommunicateOP
{
public:
	// 构造函数，接收服务器信息和共享内存中的密钥信息
	CommunicateOP(std::string serverIP, unsigned short serverPort, std::string clientID, std::string serverID);
	~CommunicateOP();

	// 与服务器进行加密通信的方法
	bool communicateWithServer();

	// 发送加密消息到服务器
	bool sendEncryptedMsg(const std::string& msg);

	// 接收并解密来自服务器的消息
	std::string recvDecryptedMsg();

private:
	std::string m_serverIP;          // 服务器IP
	unsigned short m_serverPort; // 服务器端口
	std::string m_clientID;          // 客户端ID
	std::string m_serverID;          // 服务器ID
	TcpSocket* m_socket;        // 通信套接字
	Interface* m_interface;     // 加密接口
};