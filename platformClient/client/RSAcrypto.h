#pragma once
#include <string>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include"Hash.h"
enum SignLevel{
    level1=NID_md5,
    level2=NID_sha1,
    level3=NID_sha224,
    level4=NID_sha256,
    level5=NID_sha384,
    level6=NID_sha512
};
class RSAcrypto{
public:
    RSAcrypto();
    ~RSAcrypto();
    RSAcrypto(std::string filename,bool isPrivate=true);

    // 通过解析字符串得到秘钥
	void parseKeyString(std::string keystr, bool pubKey = true);
    
	//  生成RSA密钥对,写入到文件中
	void generateRsakey(int bits, std::string pub = "public.pem", std::string pri = "private.pem");

	// 公钥加密
	std::string rsaPubKeyEncrypt(std::string data);
	// 私钥解密
	std::string rsaPriKeyDecrypt(std::string encData);

	// 使用RSA签名
	std::string rsaSign(std::string data, SignLevel level = level3);
	// 使用RSA验证签名
	bool rsaVerify(std::string data, std::string signData, SignLevel level = level3);

private:
    RSA* m_publickey;
    RSA* m_privatekey;

    std::string toBase64(const char* str, int len);
	// base64解码
	char* fromBase64(std::string str);

	// 得到公钥
	bool initPublicKey(std::string pubfile);
	// 得到私钥
	bool initPrivateKey(std::string prifile);
};