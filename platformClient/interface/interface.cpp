#include"interface.h"
Interface::Interface(std::string json)
{
	// 解析json文件
	// 1. 得到流对象 -> 读文件
	std::ifstream ifs(json);
	// 2. 创建json Reader 
	Json::Reader rd;
	// 3. 调用Reader对象 parse, 初始化一个Value对象
	Json::Value root;
	rd.parse(ifs, root);
	// 4. 对Value对象中的数据
	std::string shmkey = root["shmkey"].asString();
	std::string serverID = root["serverID"].asString();
	std::string clientID = root["clientID"].asString();
	int maxNode = root["maxNode"].asInt();

	// 读共享内存
	SecKeyShm shm(shmkey, maxNode);
	// 得到秘钥
	NodeSecKeyInfo node = shm.shmRead(clientID, serverID);
	m_key = std::string(node.seckey);
}


Interface::~Interface()
{
}

// "hello,world"
std::string Interface::encryptData(std::string data)
{
	// data -> 要加密的数据
	std::string head = "666 ";
	std::string str = head + data;	// 666helloworld
	AESCrypto aes(m_key);
	std::string ret = aes.AESCBCDecrpt(str);
	return ret;
}

std::string Interface::decryptData(std::string data)
{
	AESCrypto aes(m_key);
	return aes.AESCBCDecrpt(data);
}
