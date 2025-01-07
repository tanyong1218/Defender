#pragma once
#include "../lmdb/lmdb.h"
#include "WindowsHelper.h"
#include <string>
#include <list>
#include <FileVaildate.h>
#include "StringHelper.h"


#define RANSOM_DIR  _T("HashStore")

typedef BOOL (APIENTRY *PFUNCTION_CHECK_RANSOMWARE) (PCHAR pszVirusName, PWCHAR pszFileName, int nType);

#pragma pack (1)
#define HASH_TYPE_SHA1 (1)
#define HASH_TYPE_SHA2 (2)
#define HASH_TYPE_MD5  (3)
#define HASH_TYPE_SM3  (4)

#define HASHSTORE_HEADER_TGA1 'A'
#define HASHSTORE_HEADER_TGA2 'R'
#define HASHSTORE_HEADER_TGA3 'S'
#define SM3_HASH_SIZE 32
#define SHA256_HASH_SIZE 32
#define MAX_VIRUS_NAME_LEN              (40)
#define CRYPTO_UNIT_SIZE 16
#define MD5_HASH_SIZE 16
#define SHA1_HASH_SIZE 20
//按16字节对齐
typedef struct _HASHSTORE_HEADER
{
    UCHAR TGA_1;//0x65 A				//标识信息
    UCHAR TGA_2;//0x82 R
    UCHAR TGA_3;//0x83 S

    SHORT StoreVersion_1;		//版本信息
    SHORT StoreVersion_2;
    SHORT StoreVersion_3;
    SHORT StoreVersion_4;

    __int64 Time;

    UCHAR Reserve[45];

    UCHAR Sha1[SM3_HASH_SIZE];	//Hash校验
}
HASHSTORE_HEADER, *PHASHSTORE_HEADER;

//按16字节对齐
typedef struct _VIRUS_INFO
{
    UCHAR HashType;				//Hash类型，md5/sha1/sha256
    UCHAR Hash[SHA256_HASH_SIZE];	//Hash

    CHAR VirusName[MAX_VIRUS_NAME_LEN];

    UCHAR Reserve[7];
}
VIRUS_INFO, *PVIRUS_INFO;

#pragma pack ()

class CFeatureDB 
{
public:
    static CFeatureDB* GetInstance();

    static void Destroy();

    // 初始化Feature DB
    BOOL Init();

    // 指定目录或具体dat文件进行读取
    BOOL Load(const std::wstring& strHashstoreDir = std::wstring(), const std::wstring &strSuffix = _T(".dat"));

    // 对指定文件进行查毒（判断是否在特征库中）
    BOOL CheckRansomware(string strVirusName, wstring wstrFileName, int nType = 3 /*HASH_TYPE_MD5*/);

    void CalcFileHash(PUCHAR pHash, int nLength, int nType, PBYTE pBuf, const DWORD& size);

    // 查询指定HASH是否存在
    BOOL IsHashExist(UCHAR *pHash, int nLength, BOOL &bExist, string pszVirusName);

    BOOL FetchFileHash(PUCHAR pHash, const int& nLength, const int& nType, const std::wstring& strFile);
protected:
    CFeatureDB();
    ~CFeatureDB();

    // 卸载
    void Uninit();

    // 初始化环境
    BOOL InitEnv();

    // 初始化DB路径
    void InitDBPath();

    // 加载dat文件
    BOOL LoadDat(const std::wstring& strDatPath, std::wstring& strDatVersion);

    bool ClearLMDB();

private:
    static CFeatureDB* m_pInst; // 单实例对象

    MDB_env *m_pEnv;
    BOOL m_bInit; // 初始化状态
    std::string m_strDBPath;

	std::mutex m_Mutex;
	//CMutexUtil m_Mutex;
};
