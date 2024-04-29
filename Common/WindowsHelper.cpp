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
* @fn            GetSIDByUserName_Wmic
* @brief         通过Wmic的方式获取用户的SID
* @param[in]	 LPCTSTR pszUserName
* @param[out]	 wstring strUserSid
* @return        BOOL TRUE：FALSE
* @author
* @modify：		2023. create it.
*/
BOOL CWindowsHelper::GetSIDByUserName_Wmic(std::wstring& strUserSid, LPCTSTR pszUserName)
{
	HRESULT hr;
	IWbemLocator* pLoc = NULL;
	IWbemServices* pSvc = NULL;
	IEnumWbemClassObject* pEnumerator = NULL;
	IWbemClassObject* pclsObj = NULL;

	ULONG	uReturn = 0;
	wchar_t buffer[MAX_PATH] = { 0 };
	BOOL    bRes = FALSE;
	size_t	bufferSize = 0;

	if (NULL == pszUserName)
	{
		goto END;
	}

	if (_T('\0') == pszUserName[0])
	{
		goto END;
	}

	hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		goto END;
	}

	hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);


	hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
	if (FAILED(hr))
	{
		goto END;
	}

	hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
	if (FAILED(hr))
	{
		goto END;
	}

	hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
	if (FAILED(hr))
	{
		goto END;
	}
	bufferSize = sizeof(buffer) / sizeof(TCHAR);
	swprintf_s(buffer, bufferSize - 1, _T("SELECT * FROM Win32_UserAccount WHERE Name = \"%s\""), pszUserName);
	hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(buffer), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
	if (FAILED(hr))
	{
		goto END;
	}

	while (pEnumerator)
	{

		VARIANT vtSID;
		VARIANT vtUserName;
		VariantInit(&vtSID);
		VariantInit(&vtUserName);

		hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
		if (FAILED(hr))
		{
			break;
		}

		if (uReturn == 0)
		{
			//hr = S_FALSE(0x00000001),情况下可能代表执行成功但是没获得值
			//pclsObj->Release();
			break;
		}

		hr = pclsObj->Get(L"SID", 0, &vtSID, 0, 0);
		if (FAILED(hr))
		{
			pclsObj->Release();
			break;
		}

		hr = pclsObj->Get(L"name", 0, &vtUserName, 0, 0);
		if (FAILED(hr))
		{
			VariantClear(&vtSID);
			pclsObj->Release();
			break;
		}

		//vtSID有值的情况下去clear
		if (vtSID.vt != VT_NULL && vtSID.vt != VT_EMPTY)
		{
			std::wstring userSid(vtSID.bstrVal);
			strUserSid = userSid;
			VariantClear(&vtSID);
			VariantClear(&vtUserName);
		}

		pclsObj->Release();
	}
END:
	CoUninitialize();
	if (pEnumerator)
	{
		pEnumerator->Release();
	}

	if(pLoc)
	{
		pLoc->Release();
	}

	if(pSvc)
	{
		pSvc->Release();
	}

	if (strUserSid.empty())
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}

}

/*
* @fn            GetSIDByUserName_Lookup
* @brief         通过Lookup的方式获取用户的SID
* @param[in]	 LPCTSTR pszUserName
* @param[out]	 wstring strUserSid
* @return        BOOL TRUE：FALSE
* @author
* @modify：		2023. create it.
*/
BOOL CWindowsHelper::GetSIDByUserName_Lookup(std::wstring& strUserSid, LPCTSTR pszUserName)
{
	strUserSid.clear();

	BOOL bRtn = FALSE;

	unsigned char byUserSID[64] = { 0 };
	WCHAR szUserDomain[64] = { 0 };
	DWORD dwSIDSize = sizeof(byUserSID);
	DWORD dwDomainSize = sizeof(szUserDomain) / sizeof(WCHAR);
	SID_NAME_USE snu = SidTypeUser;
	WCHAR* pwszUserSid = NULL;

	if (NULL == pszUserName)
	{
		bRtn = FALSE;
		goto END;
	}

	if (_T('\0') == pszUserName[0])
	{
		bRtn = TRUE;
		goto END;
	}

	bRtn = LookupAccountName(NULL, (LPWSTR)pszUserName, (PSID)byUserSID, &dwSIDSize, (LPWSTR)szUserDomain, &dwDomainSize, &snu);
	if (!bRtn)
	{
		bRtn = FALSE;
		goto END;
	}

	bRtn = ConvertSidToStringSid((PSID)byUserSID, &pwszUserSid);
	if (!bRtn || NULL == pwszUserSid)
	{
		bRtn = FALSE;
		goto END;
	}

	strUserSid = pwszUserSid;
	bRtn = TRUE;

END:
	if (pwszUserSid != NULL)
	{
		LocalFree(pwszUserSid);
		pwszUserSid = NULL;
	}

	return bRtn;
}

/*
* @fn            GetSIDByUserName
* @brief         获取用户的SID
* @param[in]	 LPCTSTR pszUserName
* @param[out]	 wstring strUserSid
* @return        BOOL TRUE：FALSE
* @author
* @modify：		2023. create it.
*/
//Win2000上Lookup无法获取LOCAL SERVICE的SID，在机器名和用户名相同的情况下无法正确获取SID，需要使用wmic的方式
BOOL CWindowsHelper::GetSIDByUserName(std::wstring& strUserSid, LPCTSTR pszUserName)
{
	wchar_t szComputerName[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
	DWORD	dwSize	= MAX_COMPUTERNAME_LENGTH + 1;
	BOOL	bRet	= TRUE;

	if (wcscmp(pszUserName, L"SYSTEM") == 0)
	{
		strUserSid = L"S-1-5-18";
	}
	else if (wcscmp(pszUserName, L"LOCAL SERVICE") == 0)
	{
		strUserSid = L"S-1-5-19";
	}
	else if (wcscmp(pszUserName, L"NETWORK SERVICE") == 0)
	{
		strUserSid = L"S-1-5-20";
	}

	if (::GetComputerName(szComputerName, &dwSize) && strUserSid.empty())
	{
		wstring wstrUserName = pszUserName;
		transform(wstrUserName.begin(), wstrUserName.end(), wstrUserName.begin(), ::towupper);
		if (wcscmp(szComputerName, wstrUserName.c_str()) == 0)
		{
			bRet = GetSIDByUserName_Wmic(strUserSid, pszUserName);
		}
		else
		{
			bRet = GetSIDByUserName_Lookup(strUserSid, pszUserName);
		}
	}

	return bRet;
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


//获取当前运行目录
std::wstring CWindowsHelper::GetRunDir()
{
	TCHAR chPath[MAX_PATH];
	wstring strPath;
	GetModuleFileName(NULL, (LPTSTR)chPath, sizeof(chPath));
	strPath = chPath;

	wstring::size_type nPos = strPath.rfind(_T("\\"));

	if (wstring::npos == nPos)
	{
		return _T("");
	}

	return strPath.substr(0, nPos);
}

//获取系统目录
std::wstring CWindowsHelper::GetSystemDir()
{
	TCHAR systemDir[MAX_PATH];
	wstring wstrSystemDir;
	if (GetSystemDirectory(systemDir, MAX_PATH) != 0)
	{
		wstrSystemDir = systemDir;
	}
	return wstrSystemDir;
}

BOOL CWindowsHelper::IsProcessExist(LPCTSTR pszProcessName)
{
	BOOL bExist = FALSE;
	HANDLE hSnapshot = NULL;
	PROCESSENTRY32 pe32 = { 0 };
	pe32.dwSize = sizeof(PROCESSENTRY32);
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	if (Process32First(hSnapshot, &pe32))
	{
		do
		{
			if (_wcsicmp(pe32.szExeFile, pszProcessName) == 0)
			{
				bExist = TRUE;
				break;
			}
		} while (Process32Next(hSnapshot, &pe32));
	}

	if (hSnapshot)
	{
		CloseHandle(hSnapshot);
	}
	return bExist;
}

BOOL CWindowsHelper::StartProcess(LPCTSTR pszProcessName)
{
	wstring             wsWorkDir = _T("");
	wstring             wsCmdLine = _T("");
	STARTUPINFO         s = { sizeof(s) };
	PROCESS_INFORMATION pi = { 0 };

	wsWorkDir = CWindowsHelper::GetRunDir();
	wsCmdLine = CWindowsHelper::GetRunDir();

	wsCmdLine.append(_T("\\"));
	wsCmdLine.append(pszProcessName);

	s.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	s.wShowWindow = SW_NORMAL;//

	if (CreateProcess(NULL, (LPTSTR)wsCmdLine.c_str(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, wsWorkDir.c_str(), &s, &pi))
	{
		WaitForSingleObject(pi.hProcess, 1000);

		//等待进程执行完毕
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return TRUE;

	}
	return FALSE;
}

