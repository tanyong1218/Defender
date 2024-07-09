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
#define FILESCANCONTROL_EXPORTS __declspec(dllexport)
enum FILE_TYPE
{
	PE = 0,
	OEL,
};


using namespace std;
class CFileScanFun
{
public:
	CFileScanFun();
	~CFileScanFun();
	static unsigned int	WINAPI ScanFileThread(LPVOID lpParameter);
	BOOL EnableScanFileFunction();
	BOOL GetFileListByFolder(const std::wstring wstrFolder, BOOL IsSHA1Lib);
	FILE_TYPE GetFileType(const std::wstring wstrFilePath);
	BOOL CheckIsPEFile(const std::wstring wstrFilePath);
	std::unordered_map<std::string,FILE_TYPE> m_FileMap;
	CPEFileValidate m_pefilevalidate;
	std::mutex MapMutex;
	BOOL m_bStopSearch;

	vector<std::pair<wstring,double>> m_FileSHA1TimeLib;
	vector<std::pair<wstring,double>> m_FileSHA1Time;
private:

};
