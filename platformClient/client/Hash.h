#pragma once 
#include<openssl/md5.h>
#include<openssl/sha.h>
#include<iostream>
#include<cstring>
enum  HashType { m_MD5,m_SHA1, m_SHA224,m_SHA256, m_SHA384,m_SHA512 };
class Hash{
public:
    Hash(HashType type);
    ~Hash();
    void addData(std::string str);
    //return hashed result
    std::string getHashResult(); 

private:
    // md5  128bit
	inline void md5Init() { MD5_Init(&m_md5); }
	inline void md5AddData(const char* data)
	{
		MD5_Update(&m_md5, data, strlen(data));
	}
	std::string md5Result();

	// sha1 160bit
	inline void sha1Init() { SHA1_Init(&m_sha1); }
	inline void sha1AddData(const char* data)
	{
		SHA1_Update(&m_sha1, data, strlen(data));
	}
	std::string sha1Result();

	// sha224 224bit
	inline void sha224Init() { SHA224_Init(&m_sha224); }
	inline void sha224AddData(const char* data)
	{
		SHA224_Update(&m_sha224, data, strlen(data));
	}
	std::string sha224Result();

	// sha256 256bit
	inline void sha256Init() { SHA256_Init(&m_sha256); }
	inline void sha256AddData(const char* data)
	{
		SHA256_Update(&m_sha256, data, strlen(data));
	}
	std::string sha256Result();

	// sha384   384bit
	inline void sha384Init() { SHA384_Init(&m_sha384); }
	inline void sha384AddData(const char* data)
	{
		SHA384_Update(&m_sha384, data, strlen(data));
	}
	std::string sha384Result();

	// sha512 512bit
	inline void sha512Init() { SHA512_Init(&m_sha512); }
	inline void sha512AddData(const char* data)
	{
		SHA512_Update(&m_sha512, data, strlen(data));
	}
	std::string sha512Result();    
private:
    HashType m_type;
    MD5_CTX m_md5;
    SHA_CTX m_sha1;
    SHA256_CTX m_sha256;
    SHA256_CTX m_sha224;
    SHA512_CTX m_sha384;
    SHA512_CTX m_sha512;

};
