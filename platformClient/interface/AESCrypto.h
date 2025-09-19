#pragma once
#include <openssl/aes.h>
#include<string>
class AESCrypto {
public:
// 可使用 16byte, 24byte, 32byte 的秘钥
    AESCrypto(std::string key);
    ~AESCrypto();
    std::string AESCBCEncrpt(std::string data);
    std::string AESCBCDecrpt(std::string data);
private:
    std::string aesCrypto(std::string data, int crypto);
    void generateIvec(unsigned char* iv);
private:
    AES_KEY encKey;
    AES_KEY decKey;
    std::string m_key;
};
