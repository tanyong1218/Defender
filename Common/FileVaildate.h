#pragma once
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <vector>
//#include <atlstr.h>
#include<string>
using namespace std;

#define WHITELIST_NONE 		0
#define WHITELIST_ADD 		1
#define WHITELIST_DELETE 	2
#define MAX_PATH_LENGTH		(512)
#define SHA1_HASH_LENGTH	(20)
#define SHA2_HASH_LENGTH	(32)
#define INTEGRITY_LENGTH	(SHA1_HASH_LENGTH)

typedef struct __FileInfoStruct {
	TCHAR szFileFullPath[MAX_PATH_LENGTH];
	unsigned char bHashCode[INTEGRITY_LENGTH];
	unsigned long	ulOpType;
	unsigned int magic;
}FileInfoStruct;

#define __VALID_DATA_LENGTH (sizeof(unsigned int) + MAX_PATH_LENGTH * sizeof(WCHAR) + INTEGRITY_LENGTH)
typedef struct __SYSTEM_HASH_ITEM
{
	unsigned int magic;
	//WCHAR path[MAX_PATH_LENGTH + 1];
	WCHAR path[MAX_PATH_LENGTH];
	unsigned char bHashCode[INTEGRITY_LENGTH];
	unsigned char pad[((__VALID_DATA_LENGTH + 15) / 16) * 16 - __VALID_DATA_LENGTH];
}SYSTEM_HASH_ITEM, * PSYSTEM_HASH_ITEM;

typedef SYSTEM_HASH_ITEM  INTEGRITY_ITEM;

typedef vector<FileInfoStruct> VECTOR_FILEINFOSTRUCT;
typedef vector<INTEGRITY_ITEM> VECTOR_INTEGRITY_ITEM;
class CPEFileValidate
{
public:
	CPEFileValidate(void);
	~CPEFileValidate(void);

public:
	BOOL CheckPEFile(const TCHAR* szFileFullPath);
	BOOL CheckPEFileEX(const TCHAR* szFileFullPath);
	BOOL GetPEFileDegist(const TCHAR* szFileFullPath, unsigned char bHashCode[INTEGRITY_LENGTH]);

	BOOL GetPEFileDegistByLib(const TCHAR* szFileFullPath, unsigned char bHashCode[INTEGRITY_LENGTH]);

	// �����ļ��б�ϵͳ���������ĳ���������ļ���
	BOOL InsertFileInfoListToWhiteListLib(unsigned int magic, const TCHAR* szWhiteListLibFile, VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount, BOOL bClearHistory, BOOL bDeduplication = FALSE, BOOL bWithHash = FALSE);
	BOOL InsertFileInfoListToSystemWhiteListLib(VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount, BOOL bDeduplication = FALSE, BOOL bWithHash = FALSE);

	// BOOL InsertFileInfoListToSystemWhiteListLib_LessMemoary(VECTOR_FILEINFOSTRUCT &vector_fileinfostruct, OUT DWORD &dwFileCount, BOOL bDeduplication = FALSE, BOOL bWithHash = FALSE);
	// BOOL InsertFileInfoListToWhiteListLib_LessMemoary(unsigned int magic, const TCHAR* szWhiteListLibFile, VECTOR_FILEINFOSTRUCT &vector_fileinfostruct, OUT DWORD &dwFileCount, BOOL bClearHistory, BOOL bDeduplication = FALSE, BOOL bWithHash= FALSE);

	 // ��ȡϵͳ�������б�
	BOOL GetFileInfoListFromSystemWhiteListLib(VECTOR_INTEGRITY_ITEM& VecPEDigest);
	BOOL RemoveFileInfoListFromSystemWhiteListLib(VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount);

	// �ϲ��������б�ϵͳ���������ĳ���������ļ���
	BOOL MergeWhiteListFileToWhiteListLib(const TCHAR* szDstWhiteListLibFile, const TCHAR* szSrcWhiteListLibFile, BOOL bDeduplication = FALSE);
	BOOL MergeWhiteListFileToSystemWhiteListLib(const TCHAR* szSrcWhiteListLibFile);

	// ��������·��/���̵�ϵͳ����·������
	BOOL InsertFileInfoListToSystemWhiteListAuxLib(unsigned int magic, VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount, BOOL bDeduplication = FALSE);
	// ��ȡϵͳ����·��/�����б�
	BOOL GetFileInfoListFromSystemWhiteListAuxLib(VECTOR_INTEGRITY_ITEM& VecPEDigest);
	BOOL RemoveFileInfoListFromSystemWhiteListAuxLib(unsigned int magic, VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount);

	// �����б�
	BOOL InsertFileInfoListToPlyFile(LPCTSTR lpFileName, unsigned int magic, VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount, BOOL bClearHistory, BOOL bDeduplication = FALSE);
	// ��ȡ�б�
	BOOL GetFileInfoListFromPlyFile(LPCTSTR lpFileName, VECTOR_INTEGRITY_ITEM& VecPEDigest);
	// ɾ���б�
	BOOL RemoveFileInfoListFromPlyFile(LPCTSTR lpFileName, unsigned int magic, VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount);

	// ���һ��ע�����
	BOOL AddRegItem(LPCTSTR lpRegName);

	// ���ע�����
	BOOL AddRegItem(LPCTSTR lpFileName, VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount, BOOL bClearHistory, BOOL bDeduplication = FALSE);

	BOOL JudgeRegKeyOrValue(std::wstring wstrRegItemPath, unsigned int& Magic);

	BOOL ClearRegItemsFromDB();	// SystemAux.dat
	BOOL ClearFileItemsFromDB();	// SystemAux.dat
};

double endTime(LARGE_INTEGER& start);
void startTime(LARGE_INTEGER& start);