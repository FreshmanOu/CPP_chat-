#pragma once
#include <mysqlx/xdevapi.h>
#include<iostream>
#include "SeckKeyNodeInfo.h"
using namespace mysqlx;

class MySQLOP
{
public:
    MySQLOP();
    ~MySQLOP();

    // 初始化环境连接数据库
    bool connectDB(std::string host, int port, std::string user, std::string passwd, std::string dbname);
    // 得到keyID -> 根据实际业务需求封装的小函数
    int getKeyID();
    bool updataKeyID(int keyID);
    bool writeSecKey(NodeSecKeyInfo *pNode);
	// 根据客户端ID和服务器ID注销密钥
	bool deleteSecKey(const std::string& clientID, const std::string& serverID, int keyid);
	// 查询密钥是否存在且有效
    bool querySecKey(const std::string& clientID, const std::string& serverID,int keyid);
	// 查询密钥详细信息并返回
	NodeSecKeyInfo* querySecKeyInfo(const std::string& clientID, const std::string& serverID,int keyid);
    void closeDB();

private:
    // 获取当前时间, 并格式化为字符串
    std::string getCurTime();

private:
    Session *m_session;
    Schema *m_schema;

};