#pragma once
#include <windows.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <StringHelper.h>

#ifdef FILESCANCONTROL_EXPORTS
#define FILESCANCONTROL_EXPORTS __declspec(dllexport)
#else
#define FILESCANCONTROL_EXPORTS __declspec(dllimport)
#endif

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
	std::vector<std::string> m_FileList;
	BOOL m_bStopSearch;
private:

};
