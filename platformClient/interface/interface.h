#pragma once
#include <string>
 #include <json/json.h>
#include <fstream>
#include "SecKeyShm.h"
#include "AESCrypto.h"
class Interface
{
public:
	Interface(std::string json);
	~Interface();

	// 数据加密
	std::string encryptData(std::string data);
	// 数据解密
	std::string decryptData(std::string data);

private:
	std::string m_key;	// 秘钥
};

 