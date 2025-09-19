#include "SecKeyShm.h"

SecKeyShm::SecKeyShm(int key, int NodeMaxsize): 
	BaseShm(key, NodeMaxsize*sizeof(NodeSecKeyInfo))
	,m_maxNode(NodeMaxsize)
{

}

SecKeyShm::SecKeyShm(std::string pathName, int maxNode):
BaseShm(pathName, maxNode*sizeof(NodeSecKeyInfo)),
m_maxNode(maxNode)
{
}

SecKeyShm::~SecKeyShm()
{
}

void SecKeyShm::shmInit()
{
	void* pAddr = mapShm();
    if (pAddr == nullptr) {
        std::cerr << "❌ shmInit失败：映射共享内存失败" << std::endl;
        return;
    }
    // 清空所有节点（按实际大小初始化）
    memset(pAddr, 0, m_maxNode * sizeof(NodeSecKeyInfo));
    std::cout << "✅ 共享内存初始化成功（" << m_maxNode << "个节点）" << std::endl;

    // 解除映射（避免长期占用）
    unmapShm();
}

int SecKeyShm::shmWrite(NodeSecKeyInfo *pNodeInfo) {
    int ret = -1;
    if (pNodeInfo == NULL) {
        std::cerr << "❌ 传入的节点信息为空" << std::endl;
        return ret;
    }
    NodeSecKeyInfo* pAddr = static_cast<NodeSecKeyInfo*>(mapShm());
    if (pAddr == NULL || pAddr == (void*)-1) {
        std::cerr << "❌ 共享内存映射失败" << std::endl;
        return ret;
    }
    try {
        NodeSecKeyInfo* pNode = NULL;
        time_t currentTime = time(NULL);
        // 步骤1：查找相同clientID+serverID的节点（覆盖更新）

        for (int i = 0; i < m_maxNode; i++) {
            pNode = pAddr + i;
            // 检查节点是否有效（ID非空且匹配）
            if (pNode->clientID[0] != '\0' && pNode->serverID[0] != '\0' &&
                strncmp(pNode->clientID, pNodeInfo->clientID, sizeof(pNode->clientID)-1) == 0 &&
                strncmp(pNode->serverID, pNodeInfo->serverID, sizeof(pNode->serverID)-1) == 0) {
                memcpy(pNode, pNodeInfo, sizeof(NodeSecKeyInfo));
                std::cout << "✅ 覆盖已有节点（索引: " << i << "）" << std::endl;
                unmapShm();
                return 0;
            }
        }
         // 步骤2：写入空节点（ID为空）
        for (int i = 0; i < m_maxNode; ++i) {
            pNode = pAddr + i;
            // 空节点判断：clientID和serverID均为空
            if (pNode->clientID[0] == '\0' && pNode->serverID[0] == '\0') {
                memcpy(pNode, pNodeInfo, sizeof(NodeSecKeyInfo));
                std::cout << "✅ 写入空节点（索引: " << i << "）" << std::endl;
                unmapShm();
                return 0;
            }
        }

        // 步骤3：无空节点，覆盖最后一个节点
        int lastIndex = m_maxNode - 1;
        if (lastIndex < 0) {
            throw std::runtime_error("最大节点数不能为0");
        }
        pNode = pAddr + lastIndex;
        memcpy(pNode, pNodeInfo, sizeof(NodeSecKeyInfo));
        std::cout << "⚠️ 共享内存已满，覆盖最后一个节点（索引: " << lastIndex << "）" << std::endl;
        unmapShm();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "❌ shmWrite异常: " << e.what() << std::endl;
        unmapShm();
        return -1;
    }
}

NodeSecKeyInfo SecKeyShm::shmRead(std::string clientID, std::string serverID)
{
	NodeSecKeyInfo emptyInfo;
	memset(&emptyInfo, 0, sizeof(NodeSecKeyInfo));
	emptyInfo.seckeyID = -1;
	if (clientID.empty() || serverID.empty())
	{
		return emptyInfo;
	}
	NodeSecKeyInfo *pAddr = NULL;
	try
	{
		pAddr = static_cast<NodeSecKeyInfo*>(mapShm());
		if (pAddr == NULL)
		{
			return emptyInfo;
		}
		int i = 0;
		NodeSecKeyInfo info;
		NodeSecKeyInfo	*pNode = NULL;
		// 通过clientID和serverID查找节点
		for (i = 0; i < m_maxNode; i++)
		{
			pNode = pAddr + i;
			// 检查clientID和serverID是否为空字符串，避免访问无效内存
			if (strlen(pNode->clientID) > 0 && strlen(pNode->serverID) > 0 &&
				strcmp(pNode->clientID, clientID.c_str()) == 0 &&
				strcmp(pNode->serverID, serverID.c_str()) == 0&&pNode->status==1)
			{
				// 找到的节点信息, 拷贝到传出参数
				info = *pNode;
				unmapShm();
				return info;
			}
		}
		
		// 如果没有找到，返回空信息
		unmapShm();
		return emptyInfo;
	}
	catch (const std::exception& e)
	{
		if (pAddr != NULL)
		{
			unmapShm();
		}
		return emptyInfo;
	}
}
