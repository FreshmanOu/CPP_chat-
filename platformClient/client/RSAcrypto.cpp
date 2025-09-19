#include "RSAcrypto.h"

RSAcrypto::RSAcrypto()
{
    m_publickey=RSA_new();
    m_privatekey=RSA_new();
}

RSAcrypto::~RSAcrypto()
{
     if (m_privatekey != nullptr) {
        RSA_free(m_privatekey);
        m_privatekey = nullptr;
    }
    if (m_publickey != nullptr) {
        RSA_free(m_publickey);
        m_publickey = nullptr;
    }
}

RSAcrypto::RSAcrypto(std::string fileName, bool isPrivate)
{
    m_publickey=RSA_new();
    m_privatekey=RSA_new();
    if (isPrivate)
	{
		if(initPrivateKey(fileName)){
			std::cout<<"初始化成功"<<std::endl;
		}else{
			printf("initprivatekey failed\n");
		}
	}
	else
	{
		if(initPublicKey(fileName)){
			std::cout<<"初始化成功"<<std::endl;
		}else{
			printf("initpubkey failed\n");
		}
	}
}
// 将公钥/私钥字符串数据解析到 RSA 对象中
void RSAcrypto::parseKeyString(std::string keystr, bool pubKey)
{
    // 字符串数据 -> BIO对象中
	BIO* bio = BIO_new_mem_buf(keystr.data(), keystr.size());
   
    if(pubKey){

        PEM_read_bio_RSAPublicKey(bio, &m_publickey, NULL, NULL);
    }else{

        PEM_read_bio_RSAPrivateKey(bio, &m_privatekey, NULL, NULL);
    }
    BIO_free(bio);
}
// 生成RSA密钥对,写入到文件中
void RSAcrypto::generateRsakey(int bits, std::string pub, std::string pri)
{
    RSA* r = RSA_new();
	// 生成RSA密钥对
	// 创建bignum对象
	BIGNUM* e = BN_new();
	// 初始化bignum对象
	BN_set_word(e, RSA_F4);
	// 生成密钥对
	RSA_generate_key_ex(r, bits, e, NULL);
	
	BIO* pubIO = BIO_new_file(pub.data(), "w");
	// 公钥写入到pem文件中
	PEM_write_bio_RSAPublicKey(pubIO, r);

	// 缓存中的数据刷到文件中
	BIO_flush(pubIO);
	BIO_free(pubIO);

	BIO* priBio = BIO_new_file(pri.data(), "w");
	// 私钥写入文件中
	PEM_write_bio_RSAPrivateKey(priBio, r, NULL, NULL, 0, NULL, NULL);

	// 缓存中的数据刷到文件中
	BIO_flush(priBio);
	BIO_free(priBio);

	// 得到公钥和私钥
	m_privatekey = RSAPrivateKey_dup(r);
	m_publickey = RSAPublicKey_dup(r);

	// 释放资源
	BN_free(e);
	RSA_free(r);
}

std::string RSAcrypto::rsaPubKeyEncrypt(std::string data)
{
    return std::string();
}

std::string RSAcrypto::rsaPriKeyDecrypt(std::string encData)
{
    return std::string();
}

std::string RSAcrypto::rsaSign(std::string data, SignLevel level)
{
    unsigned int len;
	char* signBuf = new char[1024];//RSA_size(m_privatekey);
	memset(signBuf, 0, 1024);
	int ret = RSA_sign(level, (const unsigned char*)data.data(), data.size(), (unsigned char*)signBuf,
		&len, m_privatekey);
	if (ret == -1)
	{
		ERR_print_errors_fp(stdout);
	}
	std::cout << "sign len: " << len << ", ret: " << ret << std::endl;
	std::string retStr = toBase64(signBuf, len);
	delete[]signBuf;
	return retStr;
}

bool RSAcrypto::rsaVerify(std::string data, std::string signData, SignLevel level)
{
   // 验证签名
	int keyLen = RSA_size(m_publickey);
	char* sign = fromBase64(signData);
	int ret = RSA_verify(level, (const unsigned char*)data.data(), data.size(),
		(const unsigned char*)sign, keyLen, m_publickey);
	delete[]sign;
	if (ret == -1)
	{
		ERR_print_errors_fp(stdout);
	}
	if (ret != 1)
	{
		return false;
	}
	return true;
}

std::string RSAcrypto::toBase64(const char *str, int len)
{
    BIO* mem = BIO_new(BIO_s_mem());
	BIO* bs64 = BIO_new(BIO_f_base64());
	// mem添加到bs64中
	bs64 = BIO_push(bs64, mem);
	// 写数据
	BIO_write(bs64, str, len);
	BIO_flush(bs64);
	// 得到内存对象指针
	BUF_MEM *memPtr;
	BIO_get_mem_ptr(bs64, &memPtr);
	std::string retStr = std::string(memPtr->data, memPtr->length - 1);
	BIO_free_all(bs64);
	return retStr;
}

char *RSAcrypto::fromBase64(std::string str)
{
    int length = str.size();
	BIO* bs64 = BIO_new(BIO_f_base64());
	BIO* mem = BIO_new_mem_buf(str.data(), length);
	BIO_push(bs64, mem);
	char* buffer = new char[length];
	memset(buffer, 0, length);
	BIO_read(bs64, buffer, length);
	BIO_free_all(bs64);

	return buffer;
}

bool RSAcrypto::initPublicKey(std::string pubfile)
{
    // 通过BIO读文件
	BIO* pubBio = BIO_new_file(pubfile.data(), "r");
	if (pubBio == nullptr) {
        std::cerr << "❌ 未检测到当前路径存在公钥所在文件：" << pubfile << std::endl;
        ERR_print_errors_fp(stderr);  // 打印 OpenSSL 错误详情（如文件不存在、权限不足）
        return false;
    }
	// 将bio中的pem数据读出
	if (PEM_read_bio_RSAPublicKey(pubBio, &m_publickey, NULL, NULL) == NULL)
	{
		ERR_print_errors_fp(stdout);
		return false;
	}
	BIO_free(pubBio);
	return true;
}

bool RSAcrypto::initPrivateKey(std::string prifile)
{
    // 通过bio读文件
	BIO* priBio = BIO_new_file(prifile.data(), "r");
	// 将bio中的pem数据读出
	if (priBio == nullptr) {
        std::cerr << "❌ 未检测到当前路径存在私钥所在文件：" << prifile << std::endl;
        ERR_print_errors_fp(stderr);  // 打印 OpenSSL 错误详情（如文件不存在、权限不足）
        return false;
    }
	if (PEM_read_bio_RSAPrivateKey(priBio, &m_privatekey, NULL, NULL) == NULL)
	{
		ERR_print_errors_fp(stdout);
		return false;
	}
	BIO_free(priBio);
	return true;
}
