// USBDeview.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <tchar.h>
#include <vector>
#include <userenv.h>
using namespace std;

const TCHAR WLMINIDEVCTRL[] = _T("DynamicLinkLibrary.dll");
typedef int(_cdecl* TESTDLL)();

/*
* @fn            EnablePrivilege
* @brief         提权函数
* @param[in]	 LPCTSTR lpszPrivilege
* @param[in]	 BOOL	 bEnablePrivilege
* @return        BOOL TRUE：FALSE
* @author
* @modify：		2023. create it.
*/
BOOL EnablePrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
	TOKEN_PRIVILEGES tp;
	HANDLE hToken;
	LUID luid;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		return FALSE;
	}

	if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
	{
		CloseHandle(hToken);
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;

	if (bEnablePrivilege)
	{
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	}
	else
	{
		tp.Privileges[0].Attributes = 0;
	}

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{
		CloseHandle(hToken);
		return FALSE;
	}

	CloseHandle(hToken);
	return TRUE;
}



HWND GetMainWindowHandle(DWORD processId)
{
	HWND hWnd = NULL;
	do {
		hWnd = FindWindowEx(NULL, hWnd, NULL, NULL);
		if (hWnd != NULL) {
			DWORD procId;
			GetWindowThreadProcessId(hWnd, &procId);
			if (procId == processId) {
				return hWnd;
			}
		}
	} while (hWnd != NULL);
	return NULL;
}


//为下面的GetProcessIdFromName写一个函数头
DWORD GetProcessIdFromName(const std::wstring& processName)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (Process32First(snapshot, &entry))
	{
		do
		{
			if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0)
			{
				CloseHandle(snapshot);
				return entry.th32ProcessID;
			}
		} while (Process32Next(snapshot, &entry));
	}
	CloseHandle(snapshot);
	return 0;
}


int main(int argc, char const* argv[])
{
	SetConsoleOutputCP(65001); // 设置为UTF-8
	std::wstring processName = L"WLClient.exe";
	DWORD processId = GetProcessIdFromName(processName);

	HMODULE hMoudle = ::LoadLibrary(WLMINIDEVCTRL);
	//搜索**.dll中函数名为TestFuction的对外接口
	if (hMoudle)
	{
		TESTDLL lpproc = (TESTDLL)GetProcAddress(hMoudle, "TestFuction");
		lpproc();
		FreeLibrary(hMoudle);
	}
	else
	{
		printf("LoadLibrary failed\n");
	}

	
	return 0;
}




