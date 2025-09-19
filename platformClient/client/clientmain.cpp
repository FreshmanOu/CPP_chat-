#include"clientOP.h"
int usage()
{
	int nSel = -1;
	std::cout<<"*************************************************************"<<std::endl;
	std::cout<<"*************************************************************"<<std::endl;
	std::cout<<"*     1.密钥协商                                            *"<<std::endl;
	std::cout<<"*     2.密钥校验                                            *"<<std::endl;
	std::cout<<"*     3.密钥注销                                            *"<<std::endl;
	std::cout<<"*     0.退出系统                                            *"<<std::endl;
	std::cout<<"*************************************************************"<<std::endl;
	std::cout<<"*************************************************************"<<std::endl;
	std::cout<<"选择:"<<std::endl;

	std::cin>>nSel;
	while (getchar() != '\n');

	return nSel;
}
int main(){
    ClientOP op("client.json");
    while (1)
	{
		int sel = usage();
		switch (sel)
		{
		case 1:
			// 秘钥协商
			if (op.seckeyAgree()) { 
				std::cout << "密钥协商成功！" << std::endl;
			} else { 
				std::cout << "密钥协商失败！" << std::endl;
			}
			break;
		case 2:
			// 秘钥校验
			op.seckeyCheck();
			break;
		case 3:
			// 秘钥注销
			op.seckeylogout();
			break;
		case 0:
			std::cout << "客户端退出, bye,byte..." << std::endl;
			return 0;
		default:
			std::cout << "无效的选择，请重新输入！" << std::endl;
			break;
		}
    }
}