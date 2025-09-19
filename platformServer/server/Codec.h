#pragma once
#include <string>

struct RespondInfo
{
	int status;
	int seckeyID;
	std::string clientID;
	std::string serverID;
	std::string data;
};

struct RequestInfo
{
	int cmd;//
	std::string clientID;
	std::string serverID;
	std::string sign;//签名
	std::string data;//
};

class Codec
{
private:
    /* data */
public:
    Codec();
    virtual std::string encodeMsg();
	virtual void* decodeMsg();
	virtual ~Codec();
};
