#pragma once
#include"BaseShm.h"
#include<thread>
#include"../server/SeckKeyNodeInfo.h"
class SecKeyShm:public BaseShm{
public:
//// 打开或创建一块共享内存
	// 这个操作是在父类中做的
    SecKeyShm(int key, int NodeMaxsize);
    SecKeyShm(std::string pathName, int maxNode);
    ~SecKeyShm();
    //使用直接 SecKeyShm(key,maxnode):BaseShm(key,maxnode*sizeof(NodeSecKeyInfo))

	void shmInit();
	int shmWrite(NodeSecKeyInfo* pNodeInfo);
	NodeSecKeyInfo shmRead(std::string clientID, std::string serverID,int keyid);
    bool deleteNodeByKeyidAndStatus(
        const std::string& clientID, 
        const std::string& serverID, 
        int keyid, 
        int status);
private:
    // 秘钥节点maxsize
    int m_maxNode;
};