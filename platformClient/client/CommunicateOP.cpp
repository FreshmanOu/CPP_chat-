#include "CommunicateOP.h"

CommunicateOP::CommunicateOP(std::string serverIP, unsigned short serverPort, std::string clientID, std::string serverID)
{
    m_serverIP = serverIP;
	m_serverPort = serverPort;
	m_clientID = clientID;
	m_serverID = serverID;
	m_socket = new TcpSocket();
	m_interface = new Interface("client.json");
}

CommunicateOP::~CommunicateOP()
{
    if (m_socket != nullptr)
	{
		m_socket->disconnect();
		delete m_socket;
		m_socket = nullptr;
	}
	if (m_interface != nullptr)
	{
		delete m_interface;
		m_interface = nullptr;
	}
}

bool CommunicateOP::communicateWithServer()
{
    // 连接服务器
	int ret = m_socket->connectToHost(m_serverIP, m_serverPort);
	if (ret != 0)
	{
		std::cout << "连接服务器失败，请检查服务器是否开启" <<std:: endl;
		return false;
	}
	std::cout << "连接服务器成功，发送客户端ID进行验证..." <<std:: endl;
	
	// 发送客户端ID给服务器进行验证
	ret = m_socket->sendMsg(m_clientID);
	if (!ret)
	{
		std::cout << "发送客户端ID失败" << std::endl;
		m_socket->disconnect();
		return false;
	}
	
	// 接收服务器的验证响应
	std::string response = m_socket->recvMsg();
	if (response.empty() || response == "密钥不存在")
	{
		std::cout << "服务器响应：" << (response.empty() ? "连接已断开" : response) << std::endl;
		m_socket->disconnect();
		return false;
	}
	
	std::cout << "客户端ID验证成功，可以开始加密通信..." << std::endl;
	std::cout << "提示：输入 'exit' 结束通信会话" << std::endl;

	// 创建互斥锁和条件变量，用于多线程同步
	std::mutex mtx;
	std::condition_variable cv;
	bool stopFlag = false;

	// 创建接收消息的线程
	std::thread recvThread([&]()
	{
		while (!stopFlag)
		{
			std::string response = recvDecryptedMsg();
			if (!response.empty())
			{
				std::cout << "服务器回复: " << response << std::endl;
			}
			else
			{
				// 如果接收失败，可能是连接已断开
				{// 临界区开始
					std::unique_lock<std::mutex> lock(mtx);
					if (!stopFlag)
					{
						std::cout << "连接已断开，退出通信会话" << std::endl;
						stopFlag = true;
					}
				}// 临界区结束
				cv.notify_one();
				break;
			}
		}
	});

	// 主线程处理用户输入和发送消息
	while (!stopFlag)
	{
		std::cout << "请输入要发送的消息: ";
		std::string message;
		getline(std::cin, message);

		if (message == "exit")
		{
			{// 临界区开始
				std::unique_lock<std::mutex> lock(mtx);
				stopFlag = true;
			}
			cv.notify_one();
			break;
		}

		if (!sendEncryptedMsg(message))
		{
			std::cout << "发送消息失败，请重试" << std::endl;
		}
	}

	// 等待接收线程结束
	if (recvThread.joinable())
	{
		recvThread.join();
	}

	// 断开连接
	m_socket->disconnect();
	std::cout << "通信会话已结束" << std::endl;
	return true;
}

bool CommunicateOP::sendEncryptedMsg(const std::string &msg)
{
    try
	{
		// 使用Interface类加密数据
		std::string encryptedMsg = m_interface->encryptData(msg);
		
		// 发送加密后的消息
		bool ret = m_socket->sendMsg(encryptedMsg);
		if (ret)
		{
			std::cout << "消息发送成功" << std::endl;
		}
		return ret;
	}
	catch (const std::exception& e)
	{
		std::cout << "加密消息时发生错误: " << e.what() << std::endl;
		return false;
	}
}

std::string CommunicateOP::recvDecryptedMsg()
{
    try
	{
		// 接收加密后的消息
		std::string encryptedMsg = m_socket->recvMsg();
		if (encryptedMsg.empty())
		{
			return "";
		}
		
		// 使用Interface类解密数据
		std::string decryptedMsg = m_interface->decryptData(encryptedMsg);
		return decryptedMsg;
	}
	catch (const std::exception& e)
	{
		std::cout << "解密消息时发生错误: " << e.what() << std::endl;
		return "";
	}
}
