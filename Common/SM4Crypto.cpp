
/*++
公司：威努特公司

摘要:

	 加密、解密

责任人:
	Qi YF

模块创建日期:
	2022-1-4


更新说明:
    2023.10.10 Convert c to cpp
--*/
#include "sm4.h"
#include "SM4Crypto.h"


#ifndef CRYPTO_UNIT_SIZE
#define CRYPTO_UNIT_SIZE 16
#endif

const unsigned char key[16] = { 0x10,0x32,0x54,0x76,0x98,0xab,0xcd,0x55,0xfe,0xcd,0xbc,0x89,0x76,0x54,0x75,0x46 };

CSM4Crypto* CSM4Crypto::GetInstance()
{
    static CSM4Crypto instance;
    return &instance;
}

CSM4Crypto::CSM4Crypto()
{
    SM4_set_key(key, &m_SK4);
}

void CSM4Crypto::SM4EncryptBuffer(void *In, void *Out, int Len)
{
	unsigned char *localINBuf = (unsigned char *)In;
	unsigned char *localOUTBuf = (unsigned char *)Out;

	int i;

	if (Len % 16)
	{
		return;
	}

	for (i = 0; i < Len; i = i + 16)
	{
		SM4_encrypt(localINBuf, localOUTBuf, &m_SK4);
		localINBuf += 16;
		localOUTBuf += 16;
	}
}

void CSM4Crypto::SM4DecryptBuffer(void *In, void *Out, int Len)
{
	unsigned char *localINBuf = (unsigned char *)In;
	unsigned char *localOUTBuf = (unsigned char *)Out;

	int i;

	if (Len % 16)
	{
		return;
	}

	for (i = 0; i < Len; i = i + 16)
	{
		SM4_decrypt(localINBuf, localOUTBuf, &m_SK4);
		localINBuf += 16;
		localOUTBuf += 16;
	}
}

/********************************************************
函数：EncryptSector

作用：
	1、实现加密，目前仅采用国密4 加密算法。

注意：1、不足单元部分，用0补齐。
	  2、调用加密函数影响比较大，必须严格按要求输入。

参数：
	 1、In 需要加密的传冲区
	 2、Out 加密以后的传冲区
	 3、UnitCount 传冲区的单元数（一个单元大小：CRYPTO_UNIT_SIZE）


********************************************************/
void CSM4Crypto::EncryptSector(void *In, void *Out, int UnitCount)
{
	SM4EncryptBuffer(In, Out, UnitCount * CRYPTO_UNIT_SIZE);
	return;
}

/********************************************************
函数：DecryptSector

********************************************************/
void CSM4Crypto::DecryptSector(void *In, void *Out, int UnitCount)
{
	SM4DecryptBuffer(In, Out, UnitCount * CRYPTO_UNIT_SIZE);
	return;
}
