#include "clientOP.h"
#include <json/json.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include "RequestFactory.h"
#include "RequestCodec.h"
#include "RSAcrypto.h"
#include "TcpSocket.h"
#include "RespondFactory.h"
#include "RespondCodec.h"
#include "message.pb.h"
#include "Hash.h"
#include "CommunicateOP.h"

ClientOP::ClientOP(std::string jsonFile)
{
    // 解析json文件, 读文件 -> Value
    std::ifstream ifs(jsonFile);
    Json::Reader r;
    Json::Value root;
    r.parse(ifs, root);
    // 将root中的键值对value值取出
    m_info.ServerID = root["ServerID"].asString();
    m_info.ClientID = root["ClientID"].asString();
    m_info.ip = root["ServerIP"].asString();
    m_info.port = root["Port"].asInt();

    // 实例化共享内存对象
    // 从配置文件中读 key/pathname
    std::string shmKey = root["ShmKey"].asString();
    int maxNode = root["ShmMaxNode"].asInt();
    // 客户端存储的秘钥只有一个
    m_shm = new SecKeyShm(shmKey, maxNode);
}

ClientOP::~ClientOP()
{
    delete m_shm;
}

bool ClientOP::seckeyAgree()
{
    // 0. 生成密钥对, 将公钥字符串读出
    RSAcrypto rsa;
    // 生成密钥对
    rsa.generateRsakey(1024);
    // 读公钥文件
    std::ifstream ifs("public.pem");
    if (!ifs)
    {
        std::cerr << "无法打开公钥文件: " << std::endl;
        return false;
    }
    std::stringstream str;
    str << ifs.rdbuf();

    // 1. 初始化序列化数据
    // 序列化的类对象 -> 工厂类创建
    RequestInfo reqInfo;
    reqInfo.clientID = m_info.ClientID;
    reqInfo.serverID = m_info.ServerID;
    reqInfo.cmd = 1;          // 秘钥协商
    reqInfo.data = str.str(); // 非对称加密的公钥

    // 创建哈希对象
    Hash sha256(m_SHA256);
    sha256.addData(str.str());

    reqInfo.sign = rsa.rsaSign(sha256.getHashResult()); // 公钥的的哈希值签名

    std::cout << "clientOP:公钥签名完成..." << std::endl;

    CodecFactory *factory = new RequestFactory(&reqInfo);
    Codec *c = factory->createCodec();
    // 得到序列化之后的数据, 可以将其发送给服务器端
    std::string encstr = c->encodeMsg();

    // 套接字通信, 当前是客户端, 连接服务器
    TcpSocket *tcp = new TcpSocket;
    // 连接服务器
    int ret = tcp->connectToHost(m_info.ip, m_info.port);
    if (ret != 0)
    {
        std::cout << "clientOP:连接服务器失败..." << std::endl;
        return false;
    }
    std::cout << "clientOP:连接服务器成功..." << std::endl;

    // 发送序列化的数据
    int sendRet = tcp->sendMsg(encstr);
    if (sendRet <= 0)
    {
        std::cout << "发送数据失败，错误码: " << sendRet << std::endl;
        tcp->disconnect();
        delete tcp;
        return false;
    }

    std::cout << "数据发送成功，发送字节数: " << sendRet << std::endl;
    // 等待服务器回复
    usleep(100);
    std::string msg = tcp->recvMsg();
    if (msg.empty())
    {
        std::cout << "接收服务器数据失败，消息为空" << std::endl;
        tcp->disconnect();
        delete tcp;
        return false;
    }

    std::cout << "数据接收成功，接收到字节数: " << msg.size() << std::endl;
    factory = new RespondFactory(msg);
    c = factory->createCodec();
    RespondMsg *resData = (RespondMsg *)c->decodeMsg();
    if (!resData->status())
    {
        std::cout << "秘钥协商失败" << std::endl;
        return false;
    }
    std::string key = rsa.rsaPriKeyDecrypt(resData->data());

    NodeSecKeyInfo info;
    strcpy(info.clientID, m_info.ClientID.data());
    strcpy(info.serverID, m_info.ServerID.data());
    strcpy(info.seckey, key.data());
    info.seckeyID = resData->seckeyid();
    info.status = true;
    m_shm->shmWrite(&info);
    delete factory;
    delete c;
    // 这是一个短连接, 通信完成, 断开连接
    tcp->disconnect();
    delete tcp;
    return true;
}

void ClientOP::seckeyCheck()
{

    NodeSecKeyInfo nodeInfo = m_shm->shmRead(m_info.ClientID, m_info.ServerID);
    if (nodeInfo.status != 1)
    {
        std::cout << "未找到有效的密钥信息，无法进行密钥校验" << std::endl;
        return;
    }

    RequestInfo reqInfo;
    reqInfo.clientID = m_info.ClientID;
    reqInfo.serverID = m_info.ServerID;
    reqInfo.cmd = 2;
    reqInfo.data = std::to_string(nodeInfo.seckeyID);
    // 通过bio读文件
    BIO *priBio = BIO_new_file("private.pem", "r");
    // 将bio中的pem数据读出
    if (priBio == nullptr)
    {
        std::cerr << "❌ 未检测到当前路径存在私钥所在文件：" << std::endl;
        ERR_print_errors_fp(stderr); // 打印 OpenSSL 错误详情（如文件不存在、权限不足）
        return;
    }
    // 2. 检查通过，立即释放临时BIO（关键：避免重复占用文件资源）
    BIO_free(priBio);
    priBio = nullptr; // 避免野指针
    RSAcrypto rsa("private.pem");
    Hash sha256(m_SHA256);
    sha256.addData(reqInfo.data);
    reqInfo.sign = rsa.rsaSign(sha256.getHashResult());
    std::cout << "生成校验请求签名完成..." << std::endl;
    CodecFactory *factory = new RequestFactory(&reqInfo);
    Codec *c = factory->createCodec();
    std::string encstr = c->encodeMsg();
    TcpSocket *tcp = new TcpSocket;
    int ret = tcp->connectToHost(m_info.ip, m_info.port);
    if (ret != 0)
    {
        std::cout << "连接服务器失败，无法进行密钥校验" << std::endl;
        delete factory;
        delete c;
        delete tcp;
        return;
    }
    std::cout << "连接服务器成功，发送校验请求..." << std::endl;
    tcp->sendMsg(encstr);
    std::string msg = tcp->recvMsg();
    factory = new RespondFactory(msg);
    c = factory->createCodec();
    RespondMsg *resData = (RespondMsg *)c->decodeMsg();
    if (resData->status())
    {

        std::cout << "密钥校验成功！密钥ID: " << nodeInfo.seckeyID << " 有效" << std::endl;
    }
    else
    {
        std::cout << "密钥校验失败！密钥ID: " << nodeInfo.seckeyID << " 无效" << std::endl;
        nodeInfo.status = 0;
        m_shm->shmWrite(&nodeInfo);
    }
    delete factory;
    delete c;
    tcp->disconnect();
    delete tcp;
}

void ClientOP::seckeylogout()
{
    NodeSecKeyInfo nodeInfo = m_shm->shmRead(m_info.ClientID, m_info.ServerID);
    if (nodeInfo.status != 1)
    {
        std::cout << "未找到有效的密钥信息，无需注销" << std::endl;
        return;
    }
    RequestInfo reqInfo;
    reqInfo.clientID = m_info.ClientID;
    reqInfo.serverID = m_info.ServerID;
    reqInfo.cmd = 3;
    reqInfo.data = std::to_string(nodeInfo.seckeyID);
    // 通过bio读文件
    BIO *priBio = BIO_new_file("private.pem", "r");
    // 将bio中的pem数据读出
    if (priBio == nullptr)
    {
        std::cerr << "❌ 未检测到当前路径存在私钥所在文件：" << std::endl;
        ERR_print_errors_fp(stderr); // 打印 OpenSSL 错误详情（如文件不存在、权限不足）
        return;
    }
    // 2. 检查通过，立即释放临时BIO（关键：避免重复占用文件资源）
    BIO_free(priBio);
    priBio = nullptr; // 避免野指针
    RSAcrypto rsa("private.pem");
    Hash sha1(m_SHA256);
    sha1.addData(reqInfo.data);
    reqInfo.sign = rsa.rsaSign(sha1.getHashResult());
    std::cout << "生成注销请求签名完成..." << std::endl;
    CodecFactory *factory = new RequestFactory(&reqInfo);
    Codec *c = factory->createCodec();
    std::string encstr = c->encodeMsg();
    TcpSocket *tcp = new TcpSocket;
    int ret = tcp->connectToHost(m_info.ip, m_info.port);
    if (ret != 0)
    {
        std::cout << "连接服务器失败，无法进行密钥注销" << std::endl;
        delete factory;
        delete c;
        delete tcp;
        return;
    }
    std::cout << "连接服务器成功，发送注销请求..." << std::endl;
    tcp->sendMsg(encstr);
    std::string msg = tcp->recvMsg();
    factory = new RespondFactory(msg);
    c = factory->createCodec();
    RespondMsg *resData = (RespondMsg *)c->decodeMsg();
    if (resData->status())
    {
        std::cout << "密钥注销成功！密钥ID: " << nodeInfo.seckeyID << " 已注销" << std::endl;
        nodeInfo.status = 0;
        m_shm->shmWrite(&nodeInfo);
    }
    else
    {
        std::cout << "密钥注销失败！" << std::endl;
    }
    delete factory;
    delete c;
    tcp->disconnect();
    delete tcp;
}
