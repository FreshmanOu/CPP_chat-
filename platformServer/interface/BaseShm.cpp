#include "BaseShm.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

const char RandX = 'x';
BaseShm::BaseShm(int key)
{
	getShmID(key, 0, 0);
}

BaseShm::BaseShm(int key, int size)
{
	m_shmID=-1;
	getShmID(key, size, IPC_CREAT | 0666);
}

BaseShm::BaseShm(std::string name)
{m_shmID=-1;
	key_t key = ftok(name.c_str(), RandX);
	getShmID(key, 0, 0);
}

BaseShm::BaseShm(std::string name, int size)
{	m_shmID=-1;
	key_t key = ftok(name.c_str(), RandX);
	// 创建共享内存
	getShmID(key, size, IPC_CREAT | 0666);
}

void * BaseShm::mapShm()
{
	if (m_shmID == -1) {  // m_shmID 是构造函数中 shmget 的返回值
        std::cerr << "❌ 共享内存未创建（shmID=-1）" << std::endl;
        return nullptr;
    }
	// 若已映射，直接返回
    if (m_shmAddr != nullptr) {
        return m_shmAddr;
    }
    m_shmAddr = shmat(m_shmID, nullptr, 0);
     if (m_shmAddr == (void*)-1) {
        std::cerr << "❌ shmat映射失败: " << strerror(errno) << std::endl;
        m_shmAddr = nullptr;  // 重置为无效状态
        return nullptr;
    }

    std::cout << "✅ 共享内存映射成功（地址: " << m_shmAddr << "）" << std::endl;
    return m_shmAddr;
}

int BaseShm::unmapShm()
{
	int ret = shmdt(m_shmAddr);
	m_shmAddr = nullptr;  // 重置映射地址
	return ret;
}

int BaseShm::delShm()
{
	int ret = shmctl(m_shmID, IPC_RMID, nullptr);
	m_shmID = -1;  // 标记为已删除
	return ret;
}

BaseShm::~BaseShm()
{
	 unmapShm();  // 自动解除映射，避免资源泄漏
}

int BaseShm::getShmID(key_t key, int shmSize, int flag)
{
	m_shmID = shmget(key, shmSize, flag);
	if (m_shmID == -1)
	{
		printf("getShmID bug");
		return -1;
	}
	return m_shmID;
}
