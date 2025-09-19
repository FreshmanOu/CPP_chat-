#pragma once
#include <string>
#include"../interface/SecKeyShm.h"


struct ClientInfo
{
	std::string ServerID;
	std::string ClientID;
	std::string ip;
	unsigned short port;
};

class ClientOP
{
public:
	ClientOP(std::string jsonFile);
	~ClientOP();

	// 秘钥协商
	bool seckeyAgree();

	// 秘钥校验
	void seckeyCheck();

	// 秘钥注销
	void seckeylogout();
	// 获取客户端ID
	std::string getClientID() const { return m_info.ClientID; }

	// 获取服务器ID
	std::string getServerID() const { return m_info.ServerID; }

	// 获取服务器IP
	std::string getServerIP() const { return m_info.ip; }

	// 获取服务器端口
	unsigned short getServerPort() const { return m_info.port; }


private:
	ClientInfo m_info;
	SecKeyShm* m_shm;
	// 检查是否存在有效的密钥
};

