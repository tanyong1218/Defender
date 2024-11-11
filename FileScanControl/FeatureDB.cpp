#include "FeatureDB.h"
#include <shlobj.h> // SHCreateDirectory
#include <filesystem>
#include <FileOperationHelper.h>
#include "sha256.h"
#include "sha1.h"
#include "sm3.h"
#include "sm4.h"
#include "SM4Crypto.h" 
#include "md5.h"
namespace fs = boost::filesystem;  //Boost��
// �������������������
#define HASHSTORE_DB_NAME    "Feature.Lib"
#define VIRUS_NAME_RANSOMWARE "Ransomware"

// lmdb��С��Ĭ��20MB��
#define FEATURE_LIB_MAP_SIZE (1024 * 1024 * 20)

CFeatureDB* CFeatureDB::m_pInst = NULL;

int GetHashLengthByType(int nType)
{
    int nLength = -1;

    switch(nType)
    {
    case HASH_TYPE_MD5:
        nLength = MD5_HASH_SIZE;
        break;
    case HASH_TYPE_SHA1:
        nLength = SHA1_HASH_SIZE;
        break;    
    case HASH_TYPE_SHA2:
        nLength = SHA256_HASH_SIZE;
        break;
    case HASH_TYPE_SM3:
        nLength = SM3_HASH_SIZE;
        break;
    default:
        break;
    }

    return nLength;
}

void Bin2Hex(const unsigned char *bin, int length, char *hex) 
{
    const char hex_table[] = "0123456789ABCDEF";
    int i;

    for (i = 0; i < length; i++) {
        hex[i * 2] = hex_table[(bin[i] >> 4) & 0x0F];
        hex[i * 2 + 1] = hex_table[bin[i] & 0x0F];
    }
    hex[length * 2] = '\0';
}

//hashֵתΪ16������ʽ���ַ���
void HexToWStr(std::wstring& strHex, const unsigned char* pHex, unsigned int nLen)
{
    strHex.clear();
    if (!pHex || !nLen)
    {
        return;
    }

    TCHAR szFromat[3] = { 0 };
    for (unsigned int i = 0; i < nLen; i++)
    { 
        szFromat[2] = '\0';
        _sntprintf_s(szFromat, 2, _T("%02X"), pHex[i]);
        strHex += szFromat;
    }
}

CFeatureDB* CFeatureDB::GetInstance()
{
    static CFeatureDB m_pInst;
    return &m_pInst;
}

void CFeatureDB::Destroy()
{
    if (NULL !=  m_pInst)
    {
        delete m_pInst;
        m_pInst = NULL;
    }
}

CFeatureDB::CFeatureDB()
: m_pEnv(NULL)
, m_bInit(FALSE)
{

}

CFeatureDB::~CFeatureDB()
{
    Uninit();
}

// ��ʼ��
BOOL CFeatureDB::Init()
{
    if (m_bInit)
    {
        return m_bInit;
    }

    InitDBPath();

    m_bInit = InitEnv();

    return m_bInit;
}

// ����ʼ��
void CFeatureDB::Uninit()
{
    if (m_bInit 
     && NULL != m_pEnv)
    {
        mdb_env_close(m_pEnv);
        m_pEnv = NULL;
    }
}

BOOL CFeatureDB::InitEnv()
{
    int  nRet = MDB_SUCCESS;
    m_bInit = FALSE;

    do 
    {
        if (NULL != m_pEnv)
        {
            return m_bInit;
        }

        // ����mdb��������
        nRet = mdb_env_create(&m_pEnv);
        if (MDB_SUCCESS != nRet)
        {
            //WriteError(("Failed to create a env! Err->{}, {}"), nRet, mdb_strerror(nRet));
            break;
        }

        // ����map��С��̫С�ᵼ��д��ʧ�ܣ�
        nRet = mdb_env_set_mapsize(m_pEnv, FEATURE_LIB_MAP_SIZE); 

        // ��һ��������ע�⣬���һ������mode��ֻ��Linux����Ч��Windows�²����õ���
		nRet = mdb_env_open(m_pEnv, CStrUtil::StringToUTF8(m_strDBPath).c_str(), 0, 0660); 
        if (MDB_SUCCESS != nRet)
        {
            //WriteError(("Failed to open a env! Err->{}, {}"), nRet, mdb_strerror(nRet));
            break;
        }

        m_bInit = TRUE;
    } while (false);

    return m_bInit;
}

void CFeatureDB::InitDBPath()
{
    int nRet = ERROR_SUCCESS;

    std::wstring wstrPath = CWindowsHelper::GetRunDir();
    if (!wstrPath.empty())
    {
        wstrPath += _T("\\") _T(HASHSTORE_DB_NAME);
    }

	fs::path filePath(wstrPath);

	if (!fs::exists(filePath))
	{
		nRet = SHCreateDirectory(NULL, wstrPath.c_str());
		if (nRet != 0)
		{
			//WriteError(("Failed({}) to create dir({})"), nRet, wstrPath.c_str());
			return;
		}
	}

    m_strDBPath = CStrUtil::ConvertW2A(wstrPath);
}

//�����ļ���hashֵ
BOOL CFeatureDB::FetchFileHash(PUCHAR pHash, const int& nLength, const int& nType, const std::wstring& strFile)
{
    int    nCheckLength = 0;
    BOOL   bRet         = FALSE;
    HANDLE hFile        = INVALID_HANDLE_VALUE;
    HANDLE hMapFile     = NULL;
    PUCHAR pBuf         = NULL;
    DWORD  dwRead       = 0;
    LARGE_INTEGER filesize = { 0 };

    do
    {
        // ���У��
        if (NULL == pHash) 
        {
            break;
        }

        nCheckLength = GetHashLengthByType(nType);
        if (nCheckLength <= 0 || 
            nCheckLength != nLength)
        {
            break;
        }

        hFile = CreateFile(strFile.c_str(),
                           GENERIC_READ,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING, 
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            //WriteError(_T("Failed to open file! Err: 0x%08x"), GetLastError());
            break;
        }

        GetFileSizeEx(hFile, &filesize);

        hMapFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, filesize.HighPart, filesize.LowPart, NULL);
        if (NULL == hMapFile)
        {
            //WriteError(_T("Failed to create file map! Err: 0x%08x"), GetLastError());
            break;
        }

        pBuf = (PUCHAR)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, filesize.QuadPart);
        if (NULL == pBuf)
        {
            //WriteError(_T("Failed to map view of file!Err: 0x%08x"), GetLastError());
            break;
        }

		// ���ɨ�財����ʱ��U��ͻȻ���γ���������ɨ������У�
		// ��Ϊ��ҳ���ļ�������ļ���ͼ��ȡ��д���ļ������ܵ��»ᵼ�� EXCEPTION_IN_PAGE_ERROR �쳣
		__try
		{
			// ���Է��� pBuf �Ĵ���
			CalcFileHash(pHash, nLength, nType, pBuf, filesize.QuadPart);
		}
		__except (GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? 
			EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			// �����쳣�Ĵ���
			//WriteError(_T("UDisk is removed, GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR"));
		}
        
		UnmapViewOfFile(pBuf);

        bRet = TRUE;
    } while(false);

    if (NULL != hMapFile)
    {
        CloseHandle(hMapFile);
        hMapFile = NULL;
    }

    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return bRet;
}



BOOL CFeatureDB::LoadDat(const std::wstring& strDatPath, std::wstring& strDatVersion)
{
    BOOL   bRet            = FALSE;
    HANDLE hFile           = INVALID_HANDLE_VALUE;
    BYTE  *pBuffer         = NULL;
    DWORD  dwBufSize       = 16 * 1024; // 16KB
    DWORD  dwRealRead      = 0;
    UINT   iReadCount      = 0;
    DWORD  dwBufRealSize   = 0;
    DWORD  dwVirusInfoSize = sizeof(VIRUS_INFO);
    DWORD  dwUnitCount     = dwVirusInfoSize / CRYPTO_UNIT_SIZE;
    int    iRet            = 0;
    UINT   i               = 0;
    UINT   iTotal          = 0;
    UINT   iDup            = 0;
    
    sm3_context *SM3Context                  = NULL;
    UCHAR        SM3Hash[SM3_HASH_SIZE]      = { 0 };
    UCHAR        SM3HashCheck[SM3_HASH_SIZE] = { 0 };
    MDB_dbi      dbi                         = 0;
    MDB_txn     *pTxn                        = NULL;
    MDB_cursor  *pCursor                     = NULL;
    VIRUS_INFO  *pVirusInfo                  = NULL;
    VIRUS_INFO  *pVirusInfoOut               = NULL;
    PHASHSTORE_HEADER pHeader                = NULL;
    WCHAR        szDatVersion[32]            = { 0 };

    do 
    {
        if (strDatPath.empty())
        {
            break;
        }

        // ���ļ�
        hFile = CreateFile(strDatPath.c_str(), 
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            //WriteError(_T("Failed to open file!(0x%08x)"), GetLastError());
            break;
        }

        // ���仺���С
        pBuffer = new BYTE[dwBufSize];
        if (NULL == pBuffer)
        {
            break;
        }

        // ��ȡ�ļ�ͷ
        dwBufRealSize = min(dwBufSize, sizeof(HASHSTORE_HEADER));
        bRet = ReadFile(hFile, pBuffer, dwBufRealSize, &dwRealRead, NULL);
        if (!bRet)
        {
            //WriteError(_T("Failed to read file(0x%08x)"), GetLastError());
            break;
        }
        bRet = FALSE;

        // У���ļ�magic
        pHeader = (PHASHSTORE_HEADER)pBuffer;
        if (HASHSTORE_HEADER_TGA1 != pHeader->TGA_1
         || HASHSTORE_HEADER_TGA2 != pHeader->TGA_2
         || HASHSTORE_HEADER_TGA3 != pHeader->TGA_3)
        {
            //WriteError(_T("Invalid hashstore header!"));
            break;
        }
        _stprintf_s(szDatVersion, 32, _T("%d.%d.%d.%d"),
            pHeader->StoreVersion_1, pHeader->StoreVersion_2,
            pHeader->StoreVersion_3, pHeader->StoreVersion_4);
        strDatVersion = szDatVersion;
        //WriteInfo(_T("Load HashStore Verison: %s"), szDatVersion);

        // �ݴ�һ��ͷ����HASHֵ����Ϊbuffer�ᱻ�ظ�ʹ�ã����ݻᱻ����
        memcpy(SM3HashCheck, pHeader->Sha1, SM3_HASH_SIZE);

        // ��ʼ��SM3 content
        SM3Context = (sm3_context*)malloc(sizeof(sm3_context));
        if (NULL == SM3Context)
        {
            //WriteError(_T("Failed to malloc sm3 context"));
            break;
        }
        sm3_starts(SM3Context);

        // �ļ�ͷ��У�� = ͷ��������SHA1�ֶΣ� + ����
        memset(pHeader->Sha1, 0, SM3_HASH_SIZE);
        sm3_update(SM3Context, (unsigned char*)pHeader, sizeof(HASHSTORE_HEADER));

        // ׼��item��ʱ������
        pVirusInfoOut = (PVIRUS_INFO)malloc(dwVirusInfoSize);
        if (NULL == pVirusInfoOut)
        {
            //WriteError(_T("Failed to allocate memory!"));
            break;
        }

        // ��ȡitem�������sm3
        iRet = mdb_txn_begin(m_pEnv, NULL, 0, &pTxn);
        if (MDB_SUCCESS != iRet)
        {
            //WriteError(_T("Failed to mdb_txn_begin(err: %d)"), iRet);
            break;
        }
       
        // ��mdb
        iRet = mdb_dbi_open(pTxn, NULL, MDB_CREATE, &dbi);
        if (MDB_SUCCESS != iRet)
        {
            //WriteError(_T("Failed to open a datebase(err: %d)"), iRet);
            break;
        }

        // ��cursor
        iRet = mdb_cursor_open(pTxn, dbi, &pCursor);
        if (MDB_SUCCESS != iRet)
        {
            //WriteError(_T("Failed to open a cursor(err-%d: %S)"), iRet, mdb_strerror(iRet));
            break;
        }

        memset(pBuffer, 0, dwRealRead);
        iReadCount = dwBufSize / dwVirusInfoSize;
        dwBufRealSize = iReadCount * dwVirusInfoSize;
        while(ReadFile(hFile, pBuffer, dwBufRealSize, &dwRealRead, NULL))
        {
            if (0 == dwRealRead)
            {
                break;
            }

            // �б仯ʱ����Ҫ����iReadCount
            if (dwRealRead < dwBufRealSize)
            {
                iReadCount = dwRealRead / dwVirusInfoSize;
            }
            
            // �����ݴ������ݿ���
            for (UINT i = 0; i < iReadCount; ++i)
            {
                MDB_val key;
                MDB_val value;
                 
                pVirusInfo = (PVIRUS_INFO)(pBuffer + i * dwVirusInfoSize);


                //ransom_23.12.19.100.dat��CSM4Crypto����
                // ����
                CSM4Crypto::GetInstance()->DecryptSector(pVirusInfo, pVirusInfoOut, dwUnitCount);

                // �� key �� value ��ֵ
                key.mv_data = (void*)pVirusInfoOut->Hash; 
                key.mv_size = GetHashLengthByType(pVirusInfoOut->HashType);

                value.mv_data = (void*)pVirusInfoOut;
                value.mv_size = dwVirusInfoSize;

                // ����lmdb��
                //iRet = mdb_put(pTxn, dbi, &key, &value, MDB_NOOVERWRITE);
                iRet = mdb_cursor_put(pCursor, &key, &value, MDB_NOOVERWRITE);
                if (iRet == MDB_KEYEXIST)
                {
                    ++iDup;
                }
                else
                {
                    std::wstring strVirusHash;
                    HexToWStr(strVirusHash, pVirusInfoOut->Hash, GetHashLengthByType(pVirusInfoOut->HashType));
                }

                // ���� У��ֵ
                sm3_update(SM3Context, (unsigned char*)pVirusInfoOut, dwVirusInfoSize);
                ++iTotal;
            }

            memset(pBuffer, 0, dwRealRead);
            memset(pVirusInfoOut, 0, dwVirusInfoSize);
        }
        
		if (NULL != pCursor)
		{
			mdb_cursor_close(pCursor);
			pCursor = NULL;
		}

        // �Ƚ�Hash
        sm3_finish(SM3Context, SM3Hash);
        if (0 == memcmp(SM3Hash, SM3HashCheck, SM3_HASH_SIZE))
        {
            //commitһ�����ݿ����
            iRet = mdb_txn_commit(pTxn);
            if (MDB_SUCCESS == iRet)
            {
               bRet = TRUE;
            }
        }
        else
        {
            // hashУ��δͨ��
            //WriteInfo(_T("Hash isn't match, drop commit!"));

            // �����ύ
            mdb_txn_abort(pTxn);

            iRet = MDB_SUCCESS;
        }
    } while (false);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    if (NULL != pBuffer)
    {
        delete []pBuffer;
        pBuffer = NULL;
    }

    if (NULL != SM3Context)
    {
        free(SM3Context);
        SM3Context = NULL;
    }

    if (NULL != pVirusInfoOut)
    {
        free(pVirusInfoOut);
        pVirusInfoOut = NULL;
    }
    
    mdb_close(m_pEnv, dbi);

    if (NULL != pTxn && MDB_SUCCESS != iRet)
    {
        mdb_txn_reset(pTxn);
        mdb_txn_abort(pTxn);
    }
    pTxn = NULL;

    return bRet;
}



bool CFeatureDB::ClearLMDB()
{
    int         iRet    = 0;
    MDB_txn    *pTxn    = NULL;    
    MDB_dbi     dbi     = 0;

    if(!m_bInit || NULL == m_pEnv)
    {
        return false;
    }

    iRet = mdb_txn_begin(m_pEnv, NULL, MDB_RDONLY, &pTxn);
    if (iRet != MDB_SUCCESS)
    {
        //WriteError(_T("Failed(%d) to begin a transcation!"), iRet);
        goto END;
    }

    iRet = mdb_dbi_open(pTxn, NULL, 0, &dbi);
    if (MDB_SUCCESS != iRet)
    {
        //WriteError(_T("Failed(%d) to open a database!"), iRet);
        goto END;
    }

    iRet = mdb_drop(pTxn, dbi, 0);
    if (MDB_SUCCESS == iRet)
    {
        iRet = mdb_txn_commit(pTxn); // �ɹ����ύ
    }
    else 
    {
        mdb_txn_abort(pTxn);
        //WriteError(_T("Failed(%d) to drop a mdb_drop!"), iRet);
    }

    mdb_dbi_close(m_pEnv, dbi);

    return true;

END: 

    if (NULL != pTxn)
    {
        mdb_txn_abort(pTxn); // ʧ�ܣ���������
    }

    if(0 != dbi)
    {
        mdb_dbi_close(m_pEnv, dbi);
    }   

    return false;
}


// ָ��Ŀ¼�����dat�ļ����ж�ȡ
BOOL CFeatureDB::Load(const std::wstring& strHashstoreDir, const std::wstring &strSuffix /*= _T(".dat")*/)
{
    std::wstring xstrSuffix;
    std::wstring strPath;
    std::wstring strVersion;
    std::list<std::wstring> lstVers;
    BOOL         bRet    = FALSE;
    DWORD        dwAttrs = 0;

    do 
    {
        // ���У��
        if (strHashstoreDir.empty())
        {
            strPath = CWindowsHelper::GetRunDir();
            strPath += _T("\\") RANSOM_DIR;
        }
        else
        {
            strPath = strHashstoreDir;
        }

        // ׼����Ҫ��ȡ���ļ���׺
        if (strSuffix.empty())
        {
            xstrSuffix = _T(".dat");
        }
        else
        {
            xstrSuffix = strSuffix;
        }

        // ��ȡ����
        dwAttrs = GetFileAttributes(strPath.c_str());
        if (INVALID_FILE_ATTRIBUTES == dwAttrs)
        {
            //WriteError(("'{}' isn't found!"), strPath.c_str());
            break;
        }
        
        if (FILE_ATTRIBUTE_DIRECTORY == 
            (FILE_ATTRIBUTE_DIRECTORY & dwAttrs))
        {
            std::list<std::wstring> lstFiles;
            std::list<std::wstring>::iterator it;
            int iSucc = 0;
            
            // ����ָ�����ͱ���Ŀ¼
            bRet = FileOperationHelper::FetchXFromDir(lstFiles, strPath, xstrSuffix);
            if (!bRet)
            {
               // WriteError(("Failed to fetch file from dir({})!"), strPath.c_str());
                break;
            }

            // �жϻ����Ƿ��Ѿ�׼����
            if (!m_bInit && (!Init()))
            {
                //WriteError(("Failed to init env"));
                break;
            }

            bRet = ClearLMDB();
            if (!bRet)
            {
                //WriteError(("Failed to ClearMdb"));
                break;
            }

            for(it = lstFiles.begin(); it != lstFiles.end(); ++it)
            {
                strVersion.clear();
                bRet = LoadDat(*it, strVersion);
                if (!bRet)
                {
                    //WriteWarn(("Failed to load '{}'! Ignore!!!"),(*it).c_str());
                }
                else
                {
                    ++iSucc;
                    lstVers.push_back(strVersion);
                }
            }

            bRet = !(iSucc == 0 && !lstFiles.empty()); 
        }
        else
        {
            if (xstrSuffix.length() > strPath.length())
            {
                //WriteError(("Invalid file path(Path len:{} - Suffix len:{})"),strPath.length(), xstrSuffix.length());
                break;
            }

            // ��ȡ�ļ��ĺ�׺
            std::wstring strSuff;
            strSuff = strPath.substr(strPath.length() - xstrSuffix.length());

            if (0 != _tcsicmp(strSuff.c_str(), xstrSuffix.c_str()))
            {
                //WriteError(("Invalid file! Need '{}', given '{}'"),  xstrSuffix.c_str(), strSuff.c_str());
                break;
            }

            // �жϻ����Ƿ��Ѿ�׼����
            if (!m_bInit && (!Init()))
            {
                //WriteError(("Failed to init env"));
                break;
            }

            bRet = LoadDat(strPath, strVersion);
            lstVers.push_back(strVersion);
        }

    } while (false);

    return bRet;
}

BOOL CFeatureDB::CheckRansomware(string strVirusName, wstring wstrFileName, int nType)
{
    BOOL   bRet    = FALSE;
    BOOL   bExist  = FALSE;
    PUCHAR pHash   = NULL;
    CHAR   hexHash[80] = { 0 };
    int    nLength = 0;

	if (wstrFileName.empty())
	{
		return FALSE;
	}

    do 
    {
        // У���ļ�
        if (wstrFileName.empty())
        {
            // WriteWarn(_T("No file given!"));
            return bRet;
        }

        // У�����ͣ���������ʹ��Ĭ�ϵ�MD5��
        if (nType < HASH_TYPE_SHA1 
         || nType > HASH_TYPE_SM3)
        {
            nType = HASH_TYPE_MD5;
        }

        // �������ͼ���ָ���ļ���HASHֵ
        nLength = GetHashLengthByType(nType);
        if (nLength <= 0)
        {
            //WriteWarn(_T("Failed to fetch hash length by type"));
            break;
        }

        pHash = new UCHAR[nLength];
        if (NULL == pHash)
        {
            //WriteError(_T("Failed to allocate memory!"));
            break;
        }

        // ���ýӿڼ����ļ�Hashֵ
        bRet = FetchFileHash(pHash, nLength, nType, wstrFileName);
        if (!bRet)
        {
            //WriteError(_T("Failed to fetch file hash!'%s'"),strFile.c_str());
            break;
        }
        Bin2Hex(pHash, nLength, hexHash);
        //WriteDebug(_T("Get '%s' hash(%d): '%S'"), strFile.c_str(), nType, hexHash);

        // ����Hash
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			bRet = IsHashExist(pHash, nLength, bExist, strVirusName);
		}
		if (!bRet)
		{
			//WriteDebug(_T("Process is not virus by HashStore. Path = %s, hash(%d): '%S'."), strFile.c_str(), nType, hexHash);
			break;
		}
		
		if (bExist)
		{
			//WriteWarn(_T("Process is virus by HashStore. Path = %s, hash(%d): '%S'."), strFile.c_str(), nType, hexHash);
		}

        bRet = bExist;
    } while (false);

    if (NULL != pHash)
    {
        delete []pHash;
        pHash = NULL;
    }

    return bRet;
}


void CFeatureDB::CalcFileHash(PUCHAR pHash, int nLength, int nType, PBYTE pBuf, const DWORD & size)
{
    switch (nType)
    {
    case HASH_TYPE_SHA1:
        {
            sha1_context ctx;
            sha1_starts(&ctx);
            sha1_update(&ctx, pBuf, size);
            sha1_finish(&ctx, pHash);
        }
        break;
    case HASH_TYPE_SHA2:
        {
            sha256_context ctx;
            sha256_starts(&ctx, 0);
            sha256_update(&ctx, pBuf, size);
            sha256_finish(&ctx, pHash);
        }
        break;
    case HASH_TYPE_SM3:
        {
            sm3_context ctx;
            sm3_starts(&ctx);
            sm3_update(&ctx, pBuf, size);
            sm3_finish(&ctx, pHash);
        }
        break;
    case HASH_TYPE_MD5:
    default:
        {
            MD5_CTX_EX ctx;
            MD5Init(&ctx);
            MD5Update(&ctx, pBuf, size);
            MD5Final(&ctx, pHash);
        }
        break;
    }
}

BOOL CFeatureDB::IsHashExist(UCHAR* pHash, int nLength, BOOL& bExist, string strVirusName)
{
    BOOL bRet = FALSE;

    if (NULL == pHash || nLength < MD5_HASH_SIZE)
    {
        return bRet;
    }

    // ����Ƿ��ʼ��
    if (!m_bInit && !Init())
    {
        return bRet;
    }

    int         iRet    = 0;
    MDB_txn    *pTxn    = NULL;
    MDB_cursor *pCursor = NULL;
    MDB_dbi     dbi     = 0;
    
    do 
    {
        iRet = mdb_txn_begin(m_pEnv, NULL, MDB_RDONLY, &pTxn);
        if (iRet != MDB_SUCCESS)
        {
            //WriteError(_T("Failed(%d) to begin a transcation!"), iRet);
            break;
        }
        
        iRet = mdb_dbi_open(pTxn, NULL, 0, &dbi);
        if (MDB_SUCCESS != iRet)
        {
            //WriteError(_T("Failed(%d) to open a database!"), iRet);
            break;
        }
   
        std::wstring strVirusHash;
        HexToWStr(strVirusHash, pHash, nLength);

        // ��ѯ
        MDB_val key;
        MDB_val value;
        key.mv_data = (void*)pHash;
        key.mv_size = nLength;
        //iRet = mdb_cursor_get(pCursor, &key, &value, MDB_FIRST);
        iRet = mdb_get(pTxn, dbi, &key, &value);
        if (iRet != MDB_SUCCESS)
        {
            bExist = FALSE;
            if (iRet == MDB_NOTFOUND)
            {
                bRet = TRUE;
            }
        }
        else
        {
            bRet = TRUE;
            bExist = TRUE;

            PVIRUS_INFO pVirusInfo = (PVIRUS_INFO)value.mv_data;
            if (strlen(pVirusInfo->VirusName) == 0)
            {
                strVirusName = VIRUS_NAME_RANSOMWARE;
            }
            else
            {
				strVirusName = pVirusInfo->VirusName;
            }
            //WriteInfo(_T("Get virus name: '%S'"), strVirusName.c_str());
        }

    } while (false);

    mdb_dbi_close(m_pEnv, dbi);

    // �ر�Transcation
    if (NULL != pTxn)
    {
        mdb_txn_reset(pTxn);
        mdb_txn_abort(pTxn);
    }

    return bRet;
}
