#pragma once
#include <Windows.h>
#include <string>
#include <TlHelp32.h>
using namespace std;
class CWindowsHelper
{
public:
	static BOOL GetOnePIDBelongUserName(wstring wstrUserName, DWORD& dwPid, wstring& wstrErr);
	static BOOL EnablePrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	static BOOL GetProcessIdFromName(const std::wstring& processName, DWORD& dwPid);
};