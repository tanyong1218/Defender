// USBDeview.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <tchar.h>
#include <vector>
#include <userenv.h>
#include <LogHelper.h>
#include <WindowsHelper.h>
using namespace std;

const TCHAR DEVICECONTROL[] = _T("DeviceControl.dll");
typedef int(_cdecl* TESTDLL)();


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

int main(int argc, char const* argv[])
{
	SetConsoleOutputCP(65001); // 设置为UTF-8
	CWindowsHelper::EnablePrivilege(SE_DEBUG_NAME,FALSE);

	std::wstring processName = L"WLClient.exe";
	DWORD processId = 0;
	CWindowsHelper::GetProcessIdFromName(processName, processId);

	HMODULE hMoudle = ::LoadLibrary(DEVICECONTROL);
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




