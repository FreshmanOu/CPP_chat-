#include "SecKeyShm.h"

SecKeyShm::SecKeyShm(int key, int NodeMaxsize) : BaseShm(key, NodeMaxsize * sizeof(NodeSecKeyInfo)), m_maxNode(NodeMaxsize)
{
}

SecKeyShm::SecKeyShm(std::string pathName, int maxNode) : BaseShm(pathName, maxNode * sizeof(NodeSecKeyInfo)),
                                                          m_maxNode(maxNode)
{
}

SecKeyShm::~SecKeyShm()
{
}

void SecKeyShm::shmInit()
{
    void *pAddr = mapShm();
    if (pAddr == nullptr)
    {
        std::cerr << "❌ shmInit失败：映射共享内存失败" << std::endl;
        return;
    }
    memset(pAddr, 0, m_maxNode * sizeof(NodeSecKeyInfo));
    std::cout << "✅ 共享内存初始化成功（" << m_maxNode << "个节点）" << std::endl;
    unmapShm();
}

int SecKeyShm::shmWrite(NodeSecKeyInfo *pNodeInfo)
{
    int ret = -1;
    if (pNodeInfo == NULL)
    {
        std::cerr << "❌ 传入的节点信息为空" << std::endl;
        return ret;
    }
    NodeSecKeyInfo *pAddr = static_cast<NodeSecKeyInfo *>(mapShm());
    if (pAddr == NULL || pAddr == (void *)-1)
    {
        std::cerr << "❌ 共享内存映射失败" << std::endl;
        return ret;
    }
    try
    {
        NodeSecKeyInfo *pNode = NULL;
        time_t currentTime = time(NULL);
        // 步骤1：查找相同clientID+serverID的节点（覆盖更新）
        for (int i = 0; i < m_maxNode; i++)
        {
            pNode = pAddr + i;
            // 检查节点是否有效（ID非空且匹配）
            if (pNode->clientID[0] != '\0' && pNode->serverID[0] != '\0' &&
                strncmp(pNode->clientID, pNodeInfo->clientID, sizeof(pNode->clientID) - 1) == 0 &&
                strncmp(pNode->serverID, pNodeInfo->serverID, sizeof(pNode->serverID) - 1) == 0)
            {
                memcpy(pNode, pNodeInfo, sizeof(NodeSecKeyInfo));
                std::cout << "✅ 覆盖已有节点（索引: " << i << "）" << std::endl;
                unmapShm();
                return 0;
            }
        }
        // 步骤2：写入空节点（ID为空）
        for (int i = 0; i < m_maxNode; ++i)
        {
            pNode = pAddr + i;
            // 空节点判断：clientID和serverID均为空
            if (pNode->clientID[0] == '\0' && pNode->serverID[0] == '\0')
            {
                memcpy(pNode, pNodeInfo, sizeof(NodeSecKeyInfo));
                std::cout << "✅ 写入空节点（索引: " << i << "）" << std::endl;
                unmapShm();
                return 0;
            }
        }

        // 步骤3：无空节点，覆盖最后一个节点
        int lastIndex = m_maxNode - 1;
        if (lastIndex < 0)
        {
            throw std::runtime_error("最大节点数不能为0");
        }
        pNode = pAddr + lastIndex;
        memcpy(pNode, pNodeInfo, sizeof(NodeSecKeyInfo));
        std::cout << "⚠️ 共享内存已满，覆盖最后一个节点（索引: " << lastIndex << "）" << std::endl;
        unmapShm();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ shmWrite异常: " << e.what() << std::endl;
        unmapShm();
        return -1;
    }
}

NodeSecKeyInfo SecKeyShm::shmRead(std::string clientID, std::string serverID,int keyid)
{

    NodeSecKeyInfo emptyInfo;
    memset(&emptyInfo, 0, sizeof(NodeSecKeyInfo));
    emptyInfo.seckeyID = -1;
    if (clientID.empty() || serverID.empty() || keyid <= 0) {
        std::cerr << "❌ 读取失败：无效参数（clientID=" << clientID 
                  << ", serverID=" << serverID << ", keyid=" << keyid << "）" << std::endl;
        return emptyInfo;
    }
    NodeSecKeyInfo* pAddr = nullptr;
    try {
        pAddr = static_cast<NodeSecKeyInfo*>(mapShm());
        if (pAddr == nullptr || pAddr == reinterpret_cast<NodeSecKeyInfo*>(-1)) {
            std::cerr << "❌ 共享内存映射失败" << std::endl;
            return emptyInfo;
        }
        NodeSecKeyInfo targetInfo;
        bool found = false;
        for (int i = 0; i < m_maxNode; ++i) {
            NodeSecKeyInfo* pNode = pAddr + i;

            if (pNode->clientID[0] == '\0') {
                continue;
            }
            bool clientMatch = (strncmp(pNode->clientID, clientID.c_str(), 
                                       sizeof(pNode->clientID) - 1) == 0);
            bool serverMatch = (strncmp(pNode->serverID, serverID.c_str(), 
                                       sizeof(pNode->serverID) - 1) == 0);
            bool keyidMatch = (pNode->seckeyID == keyid);

            if (clientMatch && serverMatch && keyidMatch) {
                targetInfo = *pNode;
                found = true;
                std::cout << "✅ 共享内存中找到匹配节点（索引: " << i << "）" << std::endl;
                break; 
            }
        }
        unmapShm();
        return found ? targetInfo : emptyInfo;
    } catch (const std::exception& e) {
        std::cerr << "❌ 共享内存读取异常: " << e.what() << std::endl;
        if (pAddr != nullptr) {
            unmapShm();
        }
        return emptyInfo;
    }
}

bool SecKeyShm::deleteNodeByKeyidAndStatus(const std::string &clientID, const std::string &serverID, int keyid, int status)
{
    NodeSecKeyInfo *pAddr = static_cast<NodeSecKeyInfo *>(mapShm());
    if (pAddr == nullptr || pAddr == reinterpret_cast<NodeSecKeyInfo *>(-1))
    {
        std::cerr << "❌ 共享内存映射失败，无法执行删除操作" << std::endl;
        return false;
    }
    bool deleted = false;
    try
    {
        for (int i = 0; i < m_maxNode; i++)
        {
            NodeSecKeyInfo *pNode = pAddr + i;
            if (pNode->clientID[0] == '\0')
                continue;
            bool clientMatch = (strncmp(pNode->clientID, clientID.c_str(), sizeof(pNode->clientID) - 1) == 0);
            bool serverMatch = (strncmp(pNode->serverID, serverID.c_str(), sizeof(pNode->serverID) - 1) == 0);
            bool keyidMatch = (pNode->seckeyID == keyid);
            bool statusMatch = (pNode->status == status);

            if (clientMatch && serverMatch && keyidMatch && statusMatch)
            {
                memset(pNode, 0, sizeof(NodeSecKeyInfo));
                std::cout << "✅ 成功删除共享内存节点（索引: " << i << "）" << std::endl;
                deleted = true;
                break;
            }
        }
        if (!deleted)
        {
            std::cerr << "❌ 未找到匹配的共享内存节点" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ 删除共享内存节点异常: " << e.what() << std::endl;
        deleted = false;
    }
    unmapShm();
    return deleted;
}
