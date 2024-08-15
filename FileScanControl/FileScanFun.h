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
#define FILESCANCONTROL_EXPORTS __declspec(dllexport)



using namespace std;
class CFileScanFun
{
public:
	CFileScanFun();
	~CFileScanFun();
	static unsigned int	WINAPI ScanFileThread(LPVOID lpParameter);
	BOOL EnableScanFileFunction();
	BOOL GetFileListByFolder(const std::wstring wstrFolder, BOOL IsSHA1Lib);
	BOOL CheckIsPEFile(const std::wstring wstrFilePath);
	TCHAR* GetHashString(const unsigned char* pcSrc, DWORD dwSrcLen, TCHAR* pszDst, DWORD dwDstLen);
	CPEFileValidate m_pefilevalidate;
	std::mutex m_VectorMutex;
	BOOL m_bStopSearch;

	vector<std::pair<wstring,wstring>> m_FileHash;
private:

};
