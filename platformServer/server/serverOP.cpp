
#include <string.h>
#include <json/json.h>
#include <unistd.h>
#include <signal.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include "serverOP.h"
#include <exception>
#include "RequestFactory.h"
#include "RespondFactory.h"
// 构造函数
ServerOP::ServerOP(std::string jsonFile)
{

    m_shm = NULL;
    m_mysql = NULL;
    m_seckeyServer = NULL;

    m_pool = NULL;

    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_cond, NULL);
    m_isShutdown = false;
    std::ifstream ifs(jsonFile);
    if (!ifs.is_open())
    {
        std::cout << "json file open failed" << std::endl;
        return;
    }
    
    Json::Reader rd;
    Json::Value root;
    if (!rd.parse(ifs, root))
    {
        std::cout << "json file parse failed" << std::endl;
        return;
    }
    m_serverID = root["serverID"].asString();
    m_dbHost = root["dbHost"].asString();

    m_dbPort = root["dbPort"].asInt();

    m_dbUser = root["dbUser"].asString();
    m_dbPasswd = root["dbPasswd"].asString();
    m_dbName = root["dbName"].asString();
    m_seckeyPort = root["port"].asInt(); // 使用原端口作为密钥协商端口
    m_shmKey = root["shmkey"].asString();
    m_maxNode = root["maxnode"].asInt();

    m_shm = new SecKeyShm(m_shmKey, m_maxNode);

    m_mysql = new MySQLOP();
    if (!m_mysql->connectDB(m_dbHost, m_dbPort, m_dbUser, m_dbPasswd, m_dbName))
    {
        std::cout << "MySQL数据库连接失败" << std::endl;
        // 清理已分配的资源
        if (m_shm != NULL)
        {
            delete m_shm;
            m_shm = NULL;
        }
        return;
    }

    // 初始化密钥协商服务器
    m_seckeyServer = new TcpServer();
    int ret = m_seckeyServer->setListen(m_seckeyPort);
    if (ret != 0)
    {
        std::cout << "密钥协商服务器端口(" << m_seckeyPort << ")绑定失败，错误码: "
                  << ret << ", 错误信息: " << strerror(ret) << std::endl;
        // 清理已分配的资源
        delete m_seckeyServer;
        m_seckeyServer = NULL;
        delete m_shm;
        m_shm = NULL;
        delete m_mysql;
        m_mysql = NULL;
        return;
    }
    m_pool = new ThreadPool(4);
}

ServerOP::~ServerOP()
{
 
    stopServer();

    // 等待一段时间确保线程有机会退出
    sleep(2);

    // 释放资源
    if (m_shm != NULL)
    {
        delete m_shm;
        m_shm = NULL;
    }
    if (m_mysql != NULL)
    {
        delete m_mysql;
        m_mysql = NULL;
    }
    if (m_seckeyServer != NULL)
    {
        delete m_seckeyServer;
        m_seckeyServer = NULL;
    }
    if (m_pool != NULL)
    {
        delete m_pool;
        m_pool = NULL;
    }

    // 销毁同步变量
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}

// 启动服务器
void ServerOP::startServer()
{
    if (m_shm == NULL || m_mysql == NULL || m_seckeyServer == NULL  || m_pool == NULL)
    {
        std::cout << "服务器组件初始化失败，无法启动服务" << std::endl;
        return;
    }

    std::cout << "服务器启动成功..." << std::endl;
    std::cout << "密钥协商端口: " << m_seckeyPort << std::endl;

    // 创建密钥协商线程
    pthread_create(&m_seckeyThreadId, NULL, seckeyServerThread, this);
    // 主线程等待信号
    pthread_mutex_lock(&m_mutex);
    while (!m_isShutdown)
    {
        pthread_cond_wait(&m_cond, &m_mutex);
    }
    pthread_mutex_unlock(&m_mutex);
    pthread_join(m_seckeyThreadId, NULL);
    
}

void ServerOP::stopServer()
{
    pthread_mutex_lock(&m_mutex);
    m_isShutdown = true;
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_mutex);
}

// 线程处理函数
void ServerOP::working(void *arg, void *client)
{
    // 类型转换
    ServerOP *op = (ServerOP *)arg;       // arg是this指针
    TcpSocket *tcp = (TcpSocket *)client; // client是TcpSocket指针

    std::string msg = tcp->recvMsg();
    if (msg.empty())
    {
        std::cout << "接收数据失败..." << std::endl;
        tcp->disconnect();
        delete tcp;
        return;
    }
    std::cout << "接收数据成功 " << std::endl;

    // 2. 使用RequestCodec解析protobuf序列化数据
    RequestFactory factory(msg);
    Codec *codec = factory.createCodec();
    RequestMsg *request = (RequestMsg *)codec->decodeMsg();

    if (request == nullptr)
    {
        std::cout << "解析请求数据失败..." << std::endl;
        delete codec;
        tcp->disconnect();
        delete tcp;
        return;
    }

    // 3. 提取请求数据
    std::string clientID = request->clientid();
    std::string serverID = request->serverid();
    std::string sign = request->sign();
    std::string data = request->data();
    int cmdType = request->cmdtype();
    std::cout << "解析请求数据成功:" << std::endl;
    // 4. 验证客户端ID不为空
    if (clientID.empty())
    {
        std::cout << "客户端ID不能为空..." << std::endl;
        delete codec;

        tcp->disconnect();
        delete tcp;
        return;
    }
    switch (cmdType)
    {
    case 1:{
        //  密钥协商 - 使用客户端发送的公钥数据
        NodeSecKeyInfo keyInfo;
        std::string key = op->seckeyAgree(clientID, serverID, data, keyInfo);
        // return a seceretkey
        if (key.empty())
        {
            std::cout << "密钥协商失败..." << std::endl;
            delete codec;

            tcp->disconnect();
            delete tcp;
            return;
        }
        std::cout << "密钥协商success..." << std::endl;
        RespondInfo respondInfo1;
        respondInfo1.status = 1;
        respondInfo1.seckeyID = keyInfo.seckeyID;
        respondInfo1.clientID = clientID;
        respondInfo1.serverID = serverID;
        respondInfo1.data = key;
        RespondFactory respondFactory(&respondInfo1);
        Codec *respondCodec1 = respondFactory.createCodec();
        std::string respondData = respondCodec1->encodeMsg();
        tcp->sendMsg(respondData);
        std::cout << "发送响应数据成功，数据长度: " << respondData.size() << " 字节" << std::endl;
        delete respondCodec1;
        delete codec;
        tcp->disconnect();
        delete tcp;
        break;
    }
    case 2:{
        // 先检查共享内存中是否存在对应客户端的密钥
        NodeSecKeyInfo node = op->m_shm->shmRead(clientID, serverID, std::stoi(data));
        bool hasValidKey = false;
        std::string seckey;
        if (node.seckeyID != -1)
        {
            std::cout << "共享内存中存在客户端密钥，密钥ID: " << node.seckeyID << std::endl;
            seckey = std::string(node.seckey);
            hasValidKey = true;
        }
        else
        {
            std::cout << "共享内存中未找到密钥，查询数据库..." << std::endl;
            if (op->m_mysql->querySecKey(clientID, serverID, std::stoi(data)))
            {
                hasValidKey = true;
                std::cout << "数据库中存在客户端密钥，重新加载到共享内存..." << std::endl;
                NodeSecKeyInfo *secKeyNode = op->m_mysql->querySecKeyInfo(clientID, serverID, std::stoi(data));
                if (secKeyNode != NULL)
                {
                    seckey = std::string(secKeyNode->seckey);
                    op->m_shm->shmWrite(secKeyNode);
                }
                else
                {
                    std::cout << "虽然存在密钥记录，但获取详细信息失败" << std::endl;
                }
            }
            else
            {
                std::cout << "数据库中也未找到客户端密钥..." << std::endl;
            }
        }
        RespondInfo respondInfo2;
        respondInfo2.seckeyID = std::stoi(data);
        respondInfo2.clientID = clientID;
        respondInfo2.serverID = serverID;
        respondInfo2.data = "";
        RespondFactory respondFactory(&respondInfo2);
        Codec *respondCodec2 = respondFactory.createCodec();
        std::string respondData2 = respondCodec2->encodeMsg();
        if (!hasValidKey)
        {
            respondInfo2.status = 0;
            std::cout << "客户端没有有效的密钥..." << std::endl;
        }
        else
        {
            respondInfo2.status = 1;
            std::cout << "客户端验证成功..." << std::endl;
        }
        tcp->sendMsg(respondData2);
        delete codec;
        tcp->disconnect();
        delete respondCodec2;
        delete tcp;
        break;
    }
    case 3:{
        // zhuxiao
        NodeSecKeyInfo node = op->m_shm->shmRead(clientID, serverID, std::stoi(data));
        if (node.seckeyID == -1)
        {
            std::cout << " shm  mei zhao dao " << std::endl;
        }
        else
        {
            std::cout << "共享内存中存在客户端密钥: " << std::endl;
            if (op->m_shm->deleteNodeByKeyidAndStatus(clientID, serverID, std::stoi(data), 1))
            {
                std::cout << "共享内存密钥delete success" << std::endl;
            }
            else
            {
                std::cout << "共享内存密钥delete wrong " << std::endl;
            }
        }
        std::cout << "查询数据库..." << std::endl;
        NodeSecKeyInfo *secKeyNode = op->m_mysql->querySecKeyInfo(clientID, serverID, std::stoi(data));
        if (secKeyNode != NULL)
        {
            if (secKeyNode->status == 1)
            {
                std::cout << "数据库密钥delete" << std::endl;
                op->m_mysql->deleteSecKey(clientID, serverID, std::stoi(data));
            }
            delete secKeyNode;
        }
        else
        {
            std::cout << "数据库中也未找到客户端密钥..." << std::endl;
        }
        std::cout << "密钥delete success..." << std::endl;
        RespondInfo respondInfo3;
        respondInfo3.status = 0;
        respondInfo3.seckeyID = std::stoi(data);
        respondInfo3.clientID = clientID;
        respondInfo3.serverID = serverID;
        respondInfo3.data = "";
        RespondFactory respondFactory(&respondInfo3);
        Codec *respondCodec3 = respondFactory.createCodec();
        std::string respondData3 = respondCodec3->encodeMsg();
        tcp->sendMsg(respondData3);
        tcp->disconnect();
        delete respondCodec3;
        delete codec;
        delete tcp;
        break;
    }
    default:{
        std::cout << "不支持的命令类型: " << cmdType << std::endl;
        delete codec;
        tcp->disconnect();
        delete tcp;
        break;
        }
    }
}

void *ServerOP::seckeyServerThread(void *arg)
{
    ServerOP *op = (ServerOP *)arg;
    while (true)
    {
        // Check if server is shutting down
        pthread_mutex_lock(&op->m_mutex);
        bool isShutdown = op->m_isShutdown;
        // 额外检查服务器对象是否有效
        bool serverValid = (op->m_seckeyServer != NULL && op->m_pool != NULL);
        pthread_mutex_unlock(&op->m_mutex);

        if (isShutdown || !serverValid)
        {
            std::cout << "密钥协商线程退出条件满足: isShutdown="
                      << isShutdown << ", serverValid=" << serverValid << std::endl;
            break;
        }
        std::cout << "等待密钥协商连接..." << std::endl;

        // Use shorter timeout for better responsiveness
        TcpSocket *client = op->m_seckeyServer->acceptConn(1000);
        if (client == NULL)
        {
            continue;
        }
        std::cout << "密钥协商连接成功..." << std::endl;

        // 添加密钥协商任务
        op->m_pool->addTask([op, client]()
                            { ServerOP::working(op, client); });
    }
    std::cout << "密钥协商连接已退出" << std::endl;
    return NULL;
}

// 密钥协商
std::string ServerOP::seckeyAgree(std::string clientID, std::string serverID, std::string randStr, NodeSecKeyInfo &keyInfo)
{
    
    std::string seckey = getRandKey();
    int keyID = m_mysql->getKeyID();
    keyInfo.status = 1;
    keyInfo.seckeyID = keyID;
    strncpy(keyInfo.clientID, clientID.c_str(), sizeof(keyInfo.clientID) - 1);
    keyInfo.clientID[sizeof(keyInfo.clientID) - 1] = '\0';
    strncpy(keyInfo.serverID, serverID.c_str(), sizeof(keyInfo.serverID) - 1);
    keyInfo.serverID[sizeof(keyInfo.serverID) - 1] = '\0';
    strncpy(keyInfo.seckey, seckey.c_str(), sizeof(keyInfo.seckey) - 1);
    keyInfo.seckey[sizeof(keyInfo.seckey) - 1] = '\0';

    m_shm->shmWrite(&keyInfo);
    printf("写入密钥信息shm  success\n");
    if (!m_mysql->writeSecKey(&keyInfo))
    {
        std::cout << "serverOP:写入密钥信息到数据库失败..." << std::endl;
        return "";
    }
    std::cout << seckey.data() << std::endl;
    return seckey;
}


std::string ServerOP::getRandKey()
{
    unsigned char key[24];
    int ret = RAND_bytes(key, sizeof(key));
    if (ret != 1)
    {
        std::cout << "生成随机密钥失败..." << std::endl;
        return "";
    }

    // 转换为16进制字符串
    char buf[49] = {0};
    for (int i = 0; i < 24; i++)
    {
        sprintf(buf + i * 2, "%02x", key[i]);
    }

    return std::string(buf);
}


