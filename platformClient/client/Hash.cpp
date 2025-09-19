#include "Hash.h"

Hash::Hash(HashType type)
{
    m_type=type;
    switch (m_type)
    {
    case m_MD5:
        md5Init();
        break;
    case m_SHA1:
        sha1Init();

        break;
    case m_SHA224:
        sha224Init();
        break;
    case m_SHA256:
        sha256Init();
        break;
    case m_SHA384:
        sha384Init();
        break;
    case m_SHA512:
        sha512Init();
        break;
    default:
        sha256Init();
        break;
    }
}

Hash::~Hash()
{
}

void Hash::addData(std::string str)
{
    char*data=(char*)str.c_str();
    switch (m_type)
    {
    case m_MD5:
        md5AddData(data);
        break;
    case m_SHA1:
        sha1AddData(data);
        break;
    case m_SHA224:
        sha224AddData(data);
        break;
    case m_SHA256:
        sha256AddData(data);
        break;
    case m_SHA384:
        sha384AddData(data);
        break;
    case m_SHA512:
        sha512AddData(data);
        break;
    default:
        sha256AddData(data);
        break;
    }
}

std::string Hash::getHashResult()
{
    switch (m_type)
    {
    case m_MD5:
        return md5Result();
    case m_SHA1:
        return sha1Result();
    case m_SHA224:
        return sha224Result();
    case m_SHA256:
        return sha256Result();
    case m_SHA384:
        return sha384Result();
    case m_SHA512:
        return sha512Result();
    default:
        return md5Result();
    }
}
/*
hash数组用于存储原始的MD5哈希值，每个元素是一个字节（8位），所以大小是MD5_DIGEST_LENGTH（通常是16字节）。
result数组用于存储最终的十六进制字符串表示，其中：
每个原始字节需要用两个十六进制字符表示（比如一个字节0xFF需要表示为"FF"）
因此字符串长度是原始字节长度的2倍
额外的+1是为字符串的终止符'\0'预留空间
*/
std::string Hash::md5Result()
{
    unsigned char hash[MD5_DIGEST_LENGTH];
    char result[MD5_DIGEST_LENGTH*2+1];
    MD5_Final(hash, &m_md5);
    //将hash数组中的每个字节转换为两个十六进制字符，并存储在result数组中
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        sprintf(&result[i * 2], "%02x", hash[i]);

    }
    return std::string(result, MD5_DIGEST_LENGTH * 2+1);
}

std::string Hash::sha1Result()
{
    unsigned char hash[SHA_DIGEST_LENGTH];
    char result[SHA_DIGEST_LENGTH*2+1];
    SHA1_Final(hash, &m_sha1);
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        sprintf(&result[i * 2], "%02x", hash[i]);

    }
    return std::string(result, SHA_DIGEST_LENGTH * 2+1);
}

std::string Hash::sha224Result()
{
    unsigned char hash[SHA224_DIGEST_LENGTH];
    char result[SHA224_DIGEST_LENGTH*2+1];
    SHA224_Final(hash, &m_sha224);
    for (int i = 0; i < SHA224_DIGEST_LENGTH; i++)
    {
        sprintf(&result[i * 2], "%02x", hash[i]);
    }
    return std::string(result, SHA224_DIGEST_LENGTH * 2+1);
}

std::string Hash::sha256Result()
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char result[SHA256_DIGEST_LENGTH*2+1];
    SHA256_Final(hash, &m_sha256);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(&result[i * 2], "%02x", hash[i]);

    }
    return std::string(result, SHA256_DIGEST_LENGTH * 2+1);
}

std::string Hash::sha384Result()
{
    unsigned char hash[SHA384_DIGEST_LENGTH];
    char result[SHA384_DIGEST_LENGTH*2+1];
    SHA384_Final(hash, &m_sha384);
    for (int i = 0; i < SHA384_DIGEST_LENGTH; i++)
    {
        sprintf(&result[i * 2], "%02x", hash[i]);

    }
    return std::string(result, SHA384_DIGEST_LENGTH * 2+1);
}

std::string Hash::sha512Result()
{
    unsigned char hash[SHA512_DIGEST_LENGTH];
    char result[SHA512_DIGEST_LENGTH*2+1];
    SHA512_Final(hash, &m_sha512);
    for (int i = 0; i < SHA512_DIGEST_LENGTH; i++)
    {
        sprintf(&result[i * 2], "%02x", hash[i]);

    }
    return std::string(result, SHA512_DIGEST_LENGTH * 2+1);
}
