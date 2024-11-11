
/*++
��˾����Ŭ�ع�˾

ժҪ:

	 ���ܡ�����

������:
	Qi YF

ģ�鴴������:
	2022-1-4


����˵��:
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
������EncryptSector

���ã�
	1��ʵ�ּ��ܣ�Ŀǰ�����ù���4 �����㷨��

ע�⣺1�����㵥Ԫ���֣���0���롣
	  2�����ü��ܺ���Ӱ��Ƚϴ󣬱����ϸ�Ҫ�����롣

������
	 1��In ��Ҫ���ܵĴ�����
	 2��Out �����Ժ�Ĵ�����
	 3��UnitCount �������ĵ�Ԫ����һ����Ԫ��С��CRYPTO_UNIT_SIZE��


********************************************************/
void CSM4Crypto::EncryptSector(void *In, void *Out, int UnitCount)
{
	SM4EncryptBuffer(In, Out, UnitCount * CRYPTO_UNIT_SIZE);
	return;
}

/********************************************************
������DecryptSector

********************************************************/
void CSM4Crypto::DecryptSector(void *In, void *Out, int UnitCount)
{
	SM4DecryptBuffer(In, Out, UnitCount * CRYPTO_UNIT_SIZE);
	return;
}
