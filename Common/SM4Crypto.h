#ifndef __COMMON_SM4CRYPTO_H__
#define __COMMON_SM4CRYPTO_H__

class CSM4Crypto {
public:

    static CSM4Crypto* GetInstance();

    void EncryptSector (void *In,void *Out,int UnitCount);
    void DecryptSector (void *In,void *Out,int UnitCount);

private:
    CSM4Crypto();

    void SM4EncryptBuffer(void *In, void *Out, int Len);
    void SM4DecryptBuffer(void *In, void *Out, int Len);

private:
    SM4_KEY m_SK4;
};


#endif // __COMMON_SM4CRYPTO_H__