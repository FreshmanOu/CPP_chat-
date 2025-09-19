#pragma once
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
class BaseShm
{
public:
	// 通过key打开共享内存
	BaseShm(int key);

	// 通过传递进来的key创建/打开共享内存
	BaseShm(int key, int size);

	// 通过路径打开共享内存
	BaseShm(std::string name);

	// 通过路径创建/打开共享内存
	BaseShm(std::string name, int size);
    /*
    创建或获取共享内存标识符
    将共享内存映射到当前进程的地址空间
    解除共享内存与当前进程的映射
    删除共享内存
    */
	void* mapShm();
	int unmapShm();
	int delShm();
	~BaseShm();

private:
	int getShmID(key_t key, int shmSize, int flag);

private:
	int m_shmID;
protected:
	void* m_shmAddr = NULL;
};

 