#pragma once
#include <windows.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <StringHelper.h>
#include <mutex>
#include <LogHelper.h>
#include <ThreadPoolHelper.h>
#include <FileVaildate.h>
#include <JsonParse.h>
#include "PECache.h"
#define FILESCANCONTROL_EXPORTS __declspec(dllexport)



using namespace std;
class CFileScanFun
{
public:
	CFileScanFun();
	~CFileScanFun();
	static unsigned int	WINAPI ScanFileThread(LPVOID lpParameter);
	BOOL EnableScanFileFunction();
	BOOL GetFileListByFolder(const std::wstring wstrFolder);
	BOOL CheckIsPEFile(const std::wstring wstrFilePath);
	TCHAR* GetHashString(const unsigned char* pcSrc, DWORD dwSrcLen, TCHAR* pszDst, DWORD dwDstLen);
	BOOL GetFileInfoEx(const std::wstring wstrFullFileName, wstring& wstrHashCode, ULONGLONG& FileSize, ULONGLONG& LastWriteTime);

public:
	CPEFileValidate m_pefilevalidate;
	BOOL m_bStopSearch;
private:
	PECacheHelper* m_PeCacheHelper;

};
