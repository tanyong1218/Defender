#include "WindowsHelper.h"
CWindowsHelper::CWindowsHelper(void)
{
}
CWindowsHelper::~CWindowsHelper(void)
{
}
/*
* @fn            GetOnePIDBelongUserName
* @brief         获取一个指定用户名的进程的PID
* @param[in]	 wstring wstrUserName
* @param[out]	 DWORD&  dwPid
* @param[out]	 wstring& wstrErr
* @return        BOOL TRUE：FALSE
* @author
* @modify：		2023. create it.
*/
BOOL CWindowsHelper::GetOnePIDBelongUserName(wstring wstrUserName, DWORD& dwPid, wstring& wstrErr)
{
	HANDLE hToken = NULL;
	BOOL bFind = FALSE;
	HANDLE hSnapshot = nullptr;
	wchar_t buffer[1024] = { 0 };
	PROCESSENTRY32 pe32;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		wsprintf(buffer, L"CreateToolhelp32Snapshot Fail Error = %d", GetLastError());
		goto END;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe32))
	{
		wsprintf(buffer, L"Process32First Fail Error = %d pe32.dwSize = %d", GetLastError(), pe32.dwSize);
		goto END;
	}

	do
	{
		PTOKEN_USER pTokenUser = nullptr;
		DWORD bufferSize = 0;
		HANDLE hProcess = nullptr;
		LPWSTR username = nullptr;
		WCHAR* domain = nullptr;
		SID_NAME_USE sidType;
		DWORD dwUsernameSize = 0;
		DWORD dwDomainSize = 0;
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
		if (!hProcess)
		{
			continue;
		}

		if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
		{
			CloseHandle(hProcess);
			continue;
		}

		if (!GetTokenInformation(hToken, TokenUser, nullptr, 0, &bufferSize))
		{
			pTokenUser = (PTOKEN_USER)new BYTE[bufferSize];
			if (!pTokenUser)
			{
				CloseHandle(hProcess);
				CloseHandle(hToken);
				continue;
			}

			if (!GetTokenInformation(hToken, TokenUser, pTokenUser, bufferSize, &bufferSize))
			{
				delete[] pTokenUser;
				CloseHandle(hProcess);
				CloseHandle(hToken);
				continue;
			}
		}

		if (pTokenUser)
		{
			LookupAccountSid(nullptr, pTokenUser->User.Sid, nullptr, &dwUsernameSize, nullptr, &dwDomainSize, &sidType);
			if (dwUsernameSize == 0 || dwDomainSize == 0)
			{
				delete[] pTokenUser;
				CloseHandle(hProcess);
				CloseHandle(hToken);
				continue;
			}

			username = new WCHAR[dwUsernameSize];
			domain = new WCHAR[dwDomainSize];

			if (LookupAccountSid(NULL, pTokenUser->User.Sid, username, &dwUsernameSize, domain, &dwDomainSize, &sidType) &&
				_wcsicmp(username, wstrUserName.c_str()) == 0)
			{
				dwPid = pe32.th32ProcessID;
				bFind = TRUE;
			}
		}

		if (username)
		{
			delete[] username;
		}
		if (domain)
		{
			delete[] domain;
		}
		if (pTokenUser)
		{
			delete[] pTokenUser;
		}
		CloseHandle(hProcess);
		CloseHandle(hToken);
		if (bFind)
		{
			break;
		}

	} while (Process32Next(hSnapshot, &pe32));
END:
	if (hSnapshot)
	{
		CloseHandle(hSnapshot);
	}
	wstrErr = buffer;
	return bFind;
}
/*
* @fn            EnablePrivilege
* @brief         提权函数
* @param[in]	 LPCTSTR lpszPrivilege
* @param[in]	 BOOL	 bEnablePrivilege
* @return        BOOL TRUE：FALSE
* @author
* @modify：		2023. create it.
*/
BOOL CWindowsHelper::EnablePrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
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

/*
* @fn            GetProcessIdFromName
* @brief         通过进程名获取进程ID
* @param[in]	 wstring processName
* @return        BOOL TRUE：FALSE
* @author
* @modify：		2023. create it.
*/
BOOL CWindowsHelper::GetProcessIdFromName(const std::wstring& processName, DWORD& dwPid)
{
	BOOL bFind = FALSE;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hSnapshot = nullptr;
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot)
	{
		if (Process32First(hSnapshot, &entry))
		{
			do
			{
				if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0)
				{
					dwPid = entry.th32ProcessID;
					bFind = TRUE;
					break;
				}
			} while (Process32Next(hSnapshot, &entry));
		}
	}

	if (hSnapshot)
	{
		CloseHandle(hSnapshot);
	}
	return bFind;
}
/*
* function : SeGetWindowsVersion
* description : 获取当前系统版本
* param : iWinVersion
* return : void
*/
void CWindowsHelper::SeGetWindowsVersion(int& iWinVersion)
{
	BOOL bRet = FALSE;
	HMODULE hModNtdll = NULL;
	DWORD dwMajorVersion = 0;
	DWORD dwMinorVersion = 0;
	DWORD dwBuildNumber = 0;
	BOOL b64Bit = FALSE;
	SYSTEM_INFO si;
	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	::GetNativeSystemInfo(&si);
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
	{
		b64Bit = TRUE;
	}

	if (hModNtdll = ::LoadLibraryW(L"ntdll.dll"))
	{
		typedef void (WINAPI* pfRTLGETNTVERSIONNUMBERS)(DWORD*, DWORD*, DWORD*);
		pfRTLGETNTVERSIONNUMBERS pfRtlGetNtVersionNumbers;
		pfRtlGetNtVersionNumbers = (pfRTLGETNTVERSIONNUMBERS)::GetProcAddress(hModNtdll, "RtlGetNtVersionNumbers");
		if (pfRtlGetNtVersionNumbers)
		{
			pfRtlGetNtVersionNumbers(&dwMajorVersion, &dwMinorVersion, &dwBuildNumber);
			dwBuildNumber &= 0x0ffff;
			bRet = TRUE;
		}

		::FreeLibrary(hModNtdll);
		hModNtdll = NULL;
	}
	if (dwMajorVersion == 5 && dwMinorVersion == 0)
	{
		iWinVersion = WIN_2000;
	}
	else if (dwMajorVersion == 5 && dwMinorVersion == 1)
	{
		iWinVersion = WIN_XP;
	}
	else if (dwMajorVersion == 5 && dwMinorVersion == 2)
	{
		if (b64Bit)
			iWinVersion = WIN_2003_SERVER_R2_X64;
		else
			iWinVersion = WIN_2003_SERVER_R2_X32;
	}
	else if (dwMajorVersion == 6 && dwMinorVersion == 0)
	{
		if (b64Bit)
			iWinVersion = WIN_VISTA_X64;
		else
			iWinVersion = WIN_VISTA_X32;
	}
	else if (dwMajorVersion == 6 && dwMinorVersion == 1)
	{
		if (b64Bit)
			iWinVersion = WIN_7_X64;
		else
			iWinVersion = WIN_7_X32;
	}
	else if (dwMajorVersion == 6 && dwMinorVersion == 2)
	{
		if (b64Bit)
			iWinVersion = WIN_2012_X64;
		else
			iWinVersion = WIN_2012_X32;
	}
	else if (dwMajorVersion == 6 && dwMinorVersion == 3)
	{
		if (b64Bit)
			iWinVersion = WIN_2012_R2_X64;
		else
			iWinVersion = WIN_2012_R2_X32;
	}
	else if (dwMajorVersion == 10 && dwMinorVersion == 0)
	{
		if (b64Bit)
			iWinVersion = WIN_10_X64;
		else
			iWinVersion = WIN_10_X32;
	}
	else
	{
		iWinVersion = -1;
	}

}

BOOL CWindowsHelper::RtlGetOSVersionInfo(PRTL_OSVERSIONINFOEXW lpOsVersionInfo)
{
	BOOL bRet = FALSE;
	HMODULE hModNtdll = NULL;
	if (lpOsVersionInfo != NULL && ((hModNtdll = ::LoadLibraryW(L"ntdll.dll")) != NULL))
	{
		typedef NTSTATUS(NTAPI* pfRTLGETVERSION)(PRTL_OSVERSIONINFOW lpVersionInformation);

		pfRTLGETVERSION pfRtlGetVersion;
		pfRtlGetVersion = (pfRTLGETVERSION)::GetProcAddress(hModNtdll, "RtlGetVersion");
		if (pfRtlGetVersion != NULL)
		{
			pfRtlGetVersion((PRTL_OSVERSIONINFOW)lpOsVersionInfo);
			bRet = TRUE;
		}

		::FreeLibrary(hModNtdll);
		hModNtdll = NULL;
	}

	return bRet;
}
