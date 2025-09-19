#include "AESCrypto.h"

AESCrypto::AESCrypto(std::string key)
{
    if(key.size()==16||key.size()==24||key.size()==32)
    {
        //TODO
        const unsigned char*aeskey=(const unsigned char*)key.data();
        //TODO
        AES_set_encrypt_key(aeskey, key.size()*8, &encKey);
        AES_set_decrypt_key(aeskey, key.size()*8, &decKey);
        m_key=key;
    }
}

AESCrypto::~AESCrypto()
{
}

std::string AESCrypto::AESCBCEncrpt(std::string data)
{
    return aesCrypto(data,AES_ENCRYPT);
}

std::string AESCrypto::AESCBCDecrpt(std::string data)
{
    return aesCrypto(data,AES_DECRYPT);
}

std::string AESCrypto::aesCrypto(std::string data, int crypto)
{
    AES_KEY *key= crypto==AES_ENCRYPT?&encKey:&decKey;
    unsigned char iv[AES_BLOCK_SIZE];
    int len = data.size()+1;//   +1 for \0
    if(len%16){
        len = len/16+1;
        len*=16;
    }
    char *out = new char[len];
    generateIvec(iv);
    AES_cbc_encrypt((const unsigned char*)data.data(), (unsigned char*)out, len, key, iv, crypto);
    std::string outstr(out,len);
    delete[] out;
    out = NULL;
    return outstr;
}

void AESCrypto::generateIvec(unsigned char *iv)
{
    for(int i=0;i<AES_BLOCK_SIZE;i++)
    {
        //
        iv[i]=m_key.at(AES_BLOCK_SIZE-i-1);
    }
}
