#include "mysqlOP.h"
#include <string.h>
#include <time.h>
#include <sstream>

MySQLOP::MySQLOP()
{
    m_schema=nullptr;
    m_session=nullptr;
}

MySQLOP::~MySQLOP()
{
    closeDB();
}

bool MySQLOP::connectDB(std::string host, int port, std::string user, std::string passwd, std::string dbname) {
    // 先释放已有连接
    closeDB();

    try {
        m_session = new Session(host, port, user, passwd, dbname);
        m_schema = new Schema(*m_session, dbname);  // 仅创建一次
        std::cout << "✅ 连接数据库成功（dbname: " << dbname << "）" << std::endl;
        return true;
    } catch (const mysqlx::Error& e) {
        std::cerr << "❌ 数据库连接失败（X DevAPI错误）: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ 数据库连接失败（异常）: " << e.what() << std::endl;
    }

    // 异常情况下确保资源释放
    closeDB();
    return false;
}

int MySQLOP::getKeyID()
{
    if (m_session == nullptr || m_schema == nullptr) {
        std::cerr << "❌ 数据库连接未初始化" << std::endl;
        return -1;
    }

    try {
        Table keySn = m_schema->getTable("KEYSN");
        // 根据需求决定是否保留共享锁
        RowResult result = keySn.select("ikeysn")
            // .lockShared()  // 仅在需要强一致性时保留
            .execute();

        Row row = result.fetchOne();
        if (!row) {
            std::cout << "⚠️ KEYSN表中无记录，无法获取密钥ID" << std::endl;
            return -2;  // 用-2明确表示“无记录”，区别于-1（连接错误/异常）
        }

        int keyID = row[0].get<int>();
        return keyID;
    }
    catch (const mysqlx::Error& e) {
        std::cout << "❌ 获取密钥ID异常（可能字段类型不匹配）:" << e.what() << std::endl;
        return -1;
    }
}

bool MySQLOP::updataKeyID(int keyID)
{
     // 1. 检查数据库连接
    if (m_session == NULL || m_schema == NULL) {
        std::cout << "mysqlOP:数据库未连接" << std::endl;
        return false;
    }    
    return true;
}


bool MySQLOP::writeSecKey(NodeSecKeyInfo *pNode) {
    // 入参及连接校验
    if (pNode == nullptr) {
        std::cerr << "❌ writeSecKey失败：节点信息为空" << std::endl;
        return false;
    }
    if (m_session == nullptr || m_schema == nullptr ) {
        std::cerr << "❌ writeSecKey失败：数据库连接未初始化" << std::endl;
        return false;
    }

    try {
        // 1. 提取客户端/服务器ID和密钥信息
        std::string clientID(pNode->clientID, strnlen(pNode->clientID, sizeof(pNode->clientID)));
        std::string serverID(pNode->serverID, strnlen(pNode->serverID, sizeof(pNode->serverID)));
        std::string secKey(pNode->seckey, strnlen(pNode->seckey, sizeof(pNode->seckey)));
        std::cout << "📋 直接写入密钥：clientID=" << clientID << ", serverID=" << serverID << std::endl;


        // 获取刚生成的自增id（作为keyid）
        RowResult idResult = m_session->sql("SELECT LAST_INSERT_ID() as keyid;").execute();
        Row idRow = idResult.fetchOne();
        if (!idRow) {
            std::cerr << "❌ 无法获取KEYSN表自增ID" << std::endl;
            return false;
        }
        int keyid = idRow[0].get<int>();
        std::cout << "✅ KEYSN表自增生成密钥ID：" << keyid << std::endl;

        // 写入SECKEYINFO表（跳过SECNODE校验）
        Table secKeyTable = m_schema->getTable("SECKEYINFO");
        std::string curTime = getCurTime();

        Result insertResult = secKeyTable.insert(
            "clientid", "serverid",  "createtime", "state", "seckey"
        ).values(
            clientID,
            serverID,
            curTime,
            pNode->status,  
            secKey
        ).execute();

        // 检查插入结果
        if (insertResult.getAffectedItemsCount() == 0) {
            std::cerr << "❌ SECKEYINFO表插入失败，影响行数为0" << std::endl;
            return false;
        }
        // 记录密钥ID
        pNode->seckeyID = keyid;
        std::cout << "✅ 密钥直接写入成功！keyid=" << keyid << std::endl;
        return true;

    } catch (const mysqlx::Error& e) {
        std::cerr << "❌ 数据库错误：" << e.what() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 系统异常：" << e.what() << std::endl;
    }

    std::cout << "writeSecKey wrongserverOP:写入密钥信息到数据库失败..." << std::endl;
    return false;
}

bool MySQLOP::deleteSecKey(const std::string& clientID, const std::string& serverID, int keyid)
{
    if (m_session == NULL || m_schema == NULL)
	{
		std::cout << "数据库未连接" << std::endl;
		return false;
	}

	try
	{
		// 组织更新的SQL语句，将密钥状态从1->0
		Table secKeyTable = m_schema->getTable("SECMNG.SECKEYINFO");
		Result result = secKeyTable.update()
			.set("state", 0)
			.where("clientid = :clientID AND serverid = :serverID AND keyid = :keyid AND state = 1")
			.bind("clientID", clientID)
			.bind("serverID", serverID)
            .bind("keyid",keyid)
			.execute();

		uint64_t rowsUpdated = result.getAffectedItemsCount();
		if (rowsUpdated > 0)
		{
			std::cout << "MySQL数据库密钥注销成功，影响行数: " << rowsUpdated << std::endl;
			return true;
		}
		else
		{
			std::cout << "MySQL数据库密钥注销失败，未找到匹配的有效密钥" << std::endl;
			return false;
		}
	}
	catch (const mysqlx::Error &e)
	{
		std::cout << "MySQL数据库密钥注销异常: " << e.what() << std::endl;
		return false;
	}
}

bool MySQLOP::querySecKey(const std::string& clientID, const std::string& serverID,int keyid)
{
    if (m_session == NULL || m_schema == NULL)
    {
        std::cout << "数据库未连接" << std::endl;
        return false;
    }

    try
    {
        // 查询数据库中是否存在指定clientID和serverID且状态为1的密钥
        Table secKeyTable = m_schema->getTable("SECMNG.SECKEYINFO");
        RowResult result = secKeyTable.select("COUNT(*) as count")
            .where("clientid = :clientID AND serverid = :serverID AND keyid =: kid AND state = 1")
            .bind("clientID", clientID)
			.bind("serverID", serverID)
            .bind("kid",keyid)
            .execute();
        
        Row row = result.fetchOne();
        if (row)
        {
            std::cout << "✅ 密钥查询成功（keyid=" << keyid << "）" << std::endl;
            int count = row[0].get<int>();
            return (count > 0);
        }
        std::cerr << "❌ 未找到匹配的密钥（clientID=" <<std::endl;
        return false;
    }
    catch (const mysqlx::Error &e)
    {
        std::cout << "查询密钥异常: " << e.what() << std::endl;
        return false;
    }
}

NodeSecKeyInfo* MySQLOP::querySecKeyInfo(const std::string& clientID, const std::string& serverID,int keyid)
{
    if (m_session == NULL || m_schema == NULL)
    {
        std::cout << "数据库未连接" << std::endl;
        return NULL;
    }
    NodeSecKeyInfo* node = NULL;
    try
    {
        // 查询数据库中指定clientID和serverID且状态为1的密钥详细信息
        Table secKeyTable = m_schema->getTable("SECMNG.SECKEYINFO");
        RowResult result = secKeyTable.select("keyid", "seckey", "clientid", "serverid")
            .where("clientid = :clientID AND serverid = :serverID AND keyid = :kid ANDstate = 1")
            .bind("clientID", std::string(clientID))
            .bind("serverID", std::string(serverID))
            .bind("kid",keyid)
            .execute();
        
        Row row = result.fetchOne();
        if (row)
        {
            node = new NodeSecKeyInfo();
            if (node == NULL)
            {
                std::cout << "分配内存失败" << std::endl;
                return NULL;
            }
            node->status = 1; // 有效状态
            node->seckeyID = row[0].get<int>();
            // 注意：mysqlx::string的c_str()返回char16_t*，需要转换为std::string
            std::string seckey = std::string(row[1].get<mysqlx::string>());
            std::string clientId = std::string(row[2].get<mysqlx::string>());
            std::string serverId = std::string(row[3].get<mysqlx::string>());
             // 使用strcpy进行字符串拷贝，确保不会发生缓冲区溢出
            strncpy(node->seckey, seckey.c_str(), sizeof(node->seckey) - 1);
            node->seckey[sizeof(node->seckey) - 1] = '\0';
            strncpy(node->clientID, clientId.c_str(), sizeof(node->clientID) - 1);
            node->clientID[sizeof(node->clientID) - 1] = '\0';
            strncpy(node->serverID, serverId.c_str(), sizeof(node->serverID) - 1);
            node->serverID[sizeof(node->serverID) - 1] = '\0';
            return node;
        }
        return NULL;
    }
    catch (const mysqlx::Error &e)
    {
        std::cout << "查询密钥详细信息异常: " << e.what() << std::endl;
         if (node != NULL)
        {
            delete node;
        }
        return NULL;
    }
    catch (const std::exception& e)
    {
        std::cout << "查询密钥详细信息标准异常: " << e.what() << std::endl;
        if (node != NULL)
        {
            delete node;
        }
        return NULL;
    }
}

void MySQLOP::closeDB()
{
    if (m_schema != NULL)
    {
        delete m_schema;
        m_schema = NULL;
    }
    
    if (m_session != NULL)
    {
        try {
            
            m_session->close();
            delete m_session;
        } catch (const mysqlx::Error &e) {
            std::cout << "关闭MySQL会话异常: " << e.what() << std::endl;
        }
        m_session = NULL;
    }
}

std::string MySQLOP::getCurTime()
{
    time_t timep;
    time(&timep);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&timep));
    return tmp;
}
