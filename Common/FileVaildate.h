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

	// 插入文件列表到系统白名单库或某个白名单文件中
	BOOL InsertFileInfoListToWhiteListLib(unsigned int magic, const TCHAR* szWhiteListLibFile, VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount, BOOL bClearHistory, BOOL bDeduplication = FALSE, BOOL bWithHash = FALSE);
	BOOL InsertFileInfoListToSystemWhiteListLib(VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount, BOOL bDeduplication = FALSE, BOOL bWithHash = FALSE);

	// BOOL InsertFileInfoListToSystemWhiteListLib_LessMemoary(VECTOR_FILEINFOSTRUCT &vector_fileinfostruct, OUT DWORD &dwFileCount, BOOL bDeduplication = FALSE, BOOL bWithHash = FALSE);
	// BOOL InsertFileInfoListToWhiteListLib_LessMemoary(unsigned int magic, const TCHAR* szWhiteListLibFile, VECTOR_FILEINFOSTRUCT &vector_fileinfostruct, OUT DWORD &dwFileCount, BOOL bClearHistory, BOOL bDeduplication = FALSE, BOOL bWithHash= FALSE);

	 // 获取系统白名单列表
	BOOL GetFileInfoListFromSystemWhiteListLib(VECTOR_INTEGRITY_ITEM& VecPEDigest);
	BOOL RemoveFileInfoListFromSystemWhiteListLib(VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount);

	// 合并白名单列表到系统白名单库或某个白名单文件中
	BOOL MergeWhiteListFileToWhiteListLib(const TCHAR* szDstWhiteListLibFile, const TCHAR* szSrcWhiteListLibFile, BOOL bDeduplication = FALSE);
	BOOL MergeWhiteListFileToSystemWhiteListLib(const TCHAR* szSrcWhiteListLibFile);

	// 插入信任路径/进程到系统信任路径库中
	BOOL InsertFileInfoListToSystemWhiteListAuxLib(unsigned int magic, VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount, BOOL bDeduplication = FALSE);
	// 获取系统信任路径/进程列表
	BOOL GetFileInfoListFromSystemWhiteListAuxLib(VECTOR_INTEGRITY_ITEM& VecPEDigest);
	BOOL RemoveFileInfoListFromSystemWhiteListAuxLib(unsigned int magic, VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount);

	// 插入列表
	BOOL InsertFileInfoListToPlyFile(LPCTSTR lpFileName, unsigned int magic, VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount, BOOL bClearHistory, BOOL bDeduplication = FALSE);
	// 获取列表
	BOOL GetFileInfoListFromPlyFile(LPCTSTR lpFileName, VECTOR_INTEGRITY_ITEM& VecPEDigest);
	// 删除列表
	BOOL RemoveFileInfoListFromPlyFile(LPCTSTR lpFileName, unsigned int magic, VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount);

	// 添加一条注册表项
	BOOL AddRegItem(LPCTSTR lpRegName);

	// 添加注册表项
	BOOL AddRegItem(LPCTSTR lpFileName, VECTOR_FILEINFOSTRUCT& vector_fileinfostruct, OUT DWORD& dwFileCount, BOOL bClearHistory, BOOL bDeduplication = FALSE);

	BOOL JudgeRegKeyOrValue(std::wstring wstrRegItemPath, unsigned int& Magic);

	BOOL ClearRegItemsFromDB();	// SystemAux.dat
	BOOL ClearFileItemsFromDB();	// SystemAux.dat
};

double endTime(LARGE_INTEGER& start);
void startTime(LARGE_INTEGER& start);