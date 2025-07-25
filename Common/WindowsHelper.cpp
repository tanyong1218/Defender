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
		_snwprintf_s(buffer, _countof(buffer), _TRUNCATE, L"CreateToolhelp32Snapshot Fail Error = %d", GetLastError());
		goto END;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe32))
	{
		_snwprintf_s(buffer, _countof(buffer), _TRUNCATE, L"Process32First Fail Error = %d pe32.dwSize = %d", GetLastError(), pe32.dwSize);
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
	/*
	* 	HRESULT hr;
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

	if (pLoc)
	{
		pLoc->Release();
	}

	if (pSvc)
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
	*/
	return FALSE;
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
	DWORD	dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	BOOL	bRet = TRUE;

	if (!pszUserName)
	{
		return FALSE;
	}

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
void CWindowsHelper::SeGetWindowsVersion(int& iWinVersion, BOOL& b64Bit)
{
	BOOL bRet = FALSE;
	HMODULE hModNtdll = NULL;
	DWORD dwMajorVersion = 0;
	DWORD dwMinorVersion = 0;
	DWORD dwBuildNumber = 0;
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

BOOL CWindowsHelper::GetPIDByProcessName(__in const wstring  processName, __out DWORD& dwPid)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	// Take a snapshot for all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);
		return FALSE;
	}

	BOOL bRet = FALSE;
	do
	{
		if (processName == pe32.szExeFile)
		{
			dwPid = pe32.th32ProcessID;
			bRet = TRUE;
			break;
		}
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);

	return bRet;
}

BOOL CWindowsHelper::GetMacByIp(__in const wstring wsIp, __out wstring& wstrMacAddress)
{
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;

	pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) != ERROR_SUCCESS)
	{
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
	{
		pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			if (pAdapter->Type == MIB_IF_TYPE_ETHERNET ||
				pAdapter->Type == 71//pAdapter->Type是71为：无线网卡
				)
			{
				//循环匹配输入的IP地址,针对一网卡多IP的情况
				std::string strIP = CStrUtil::ConvertW2A(wsIp);
				PIP_ADDR_STRING pIpAddrString = &pAdapter->IpAddressList;
				while (pIpAddrString != NULL)
				{
					if (_stricmp(strIP.c_str(), pIpAddrString->IpAddress.String) == 0)
					{
						wstrMacAddress = CStrUtil::MacAddrToString(pAdapter->Address);
						goto DONE;
					}
					pIpAddrString = pIpAddrString->Next;
				}
			}
			pAdapter = pAdapter->Next;
		}
	}

DONE:
	if (pAdapterInfo)
	{
		free(pAdapterInfo);
		pAdapterInfo = NULL;
	}

	return TRUE;
}

BOOL CWindowsHelper::QueryServiceInfoByName(const tstring& strSrvName, DWORD& dwCurrentState, DWORD& dwStartType, tstring* pStrErr, DWORD* pLastErrCode, tstring* pStrServiceDisplayName)
{
	BOOL bRes = FALSE;
	wostringstream  strTemp;

	SERVICE_STATUS ssStatus;
	SC_LOCK sclLockA = NULL;
	SC_HANDLE shDefineService = NULL;

	DWORD dwBytesNeeded = 0;
	DWORD cbBufSize = 0;
	DWORD dwError = 0;
	LPQUERY_SERVICE_CONFIG lpqsConfig = NULL;

	SC_HANDLE shServiceManager = OpenSCManager(NULL,SERVICES_ACTIVE_DATABASE,SC_MANAGER_ALL_ACCESS);
	if (shServiceManager == NULL)
	{
		dwError = GetLastError();
		strTemp<<_T("OSUtils::QueryServiceInfoByName: OpenSCManager fail, err= ")<<GetLastError()<<_T(",");
		goto END;
	}

	shDefineService = OpenService(shServiceManager, strSrvName.c_str(), SERVICE_QUERY_STATUS|SERVICE_QUERY_CONFIG);
	if (shDefineService == NULL)
	{

		dwError = GetLastError();
		strTemp<<_T("OSUtils::QueryServiceInfoByName: OpenService fail, err= ")<<GetLastError()<<_T(",");
		goto END;
	}

	if((QueryServiceStatus(shDefineService,&ssStatus))==0)
	{
		dwError = GetLastError();
		strTemp<<_T("OSUtils::StartService: QueryServiceStatus fail, err=")<<GetLastError()<<_T(",");
		goto END;
	}

	dwCurrentState = ssStatus.dwCurrentState;

	if(!QueryServiceConfig(shDefineService, NULL, 0, &dwBytesNeeded))
	{
		dwError = GetLastError();
		
		if(ERROR_INSUFFICIENT_BUFFER != dwError)
		{
			strTemp<<_T("OSUtils::QueryServiceConfig: QueryServiceConfig error, err=")<<dwError<<_T("\r\n");
			goto END;
		}

		cbBufSize = dwBytesNeeded;
		lpqsConfig = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LMEM_FIXED, cbBufSize);

		if(lpqsConfig)
		{
			if(!QueryServiceConfig(shDefineService, lpqsConfig, dwBytesNeeded, &dwBytesNeeded))
			{
				strTemp << _T("OSUtils::QueryServiceConfig: QueryServiceConfig failed, err=")<<GetLastError()<<_T("\r\n");
				goto END;
			}
			if(pStrServiceDisplayName)
			{
				*pStrServiceDisplayName = lpqsConfig->lpDisplayName;
			}
			dwStartType = lpqsConfig->dwStartType;
			
		}
	}


	bRes = TRUE;

END:
	if(pLastErrCode)
	{
		*pLastErrCode = dwError;
	}

	if (shDefineService)
	{
		CloseServiceHandle(shDefineService);
	}

	if (shServiceManager)
	{
		CloseServiceHandle(shServiceManager);
	}


	if (pStrErr)
	{
		*pStrErr = strTemp.str();
	}

    if ( lpqsConfig)
    {
        LocalFree(lpqsConfig);
    }

	return bRes;
}

BOOL Wow64RedirectOff::m_bCheckVersion = FALSE;
BOOL Wow64RedirectOff::m_bWin64 = FALSE;
Wow64RedirectOff::Wow64RedirectOff() {
#ifdef _WIN64//X64_BIT					// only 32bit application needs the function
	return;
#endif
	BOOL nRes = FALSE;
	DWORD dwRes = 0;
	if (!m_bCheckVersion)
	{
		CWindowsHelper getwindowversion;
		int iWindowsVersion = 0;
		getwindowversion.SeGetWindowsVersion(iWindowsVersion, m_bWin64);
		m_bCheckVersion = TRUE;
	}

	if (!m_bWin64)
	{
		LPFN_Disable = NULL;
		return;
	}

	LPFN_Disable = (FN_Wow64DisableWow64FsRedirection)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")), "Wow64DisableWow64FsRedirection");
	if (LPFN_Disable) {
		nRes = LPFN_Disable(&m_OldValue);
		dwRes = GetLastError();
	}
}

VOID Wow64RedirectOff::SetWow64RedirectOff() {
#ifdef _WIN64//X64_BIT					// only 32bit application needs the function
	return;
#endif
	BOOL nRes = FALSE;
	DWORD dwRes = 0;

	if (!m_bWin64)
	{
		LPFN_Disable = NULL;
		return;
	}

	LPFN_Disable = (FN_Wow64DisableWow64FsRedirection)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")), "Wow64DisableWow64FsRedirection");
	if (LPFN_Disable) {
		nRes = LPFN_Disable(&m_OldValue);
		dwRes = GetLastError();
	}
}

VOID Wow64RedirectOff::SetWow64RedirectOn() {
#ifdef _WIN64//X64_BIT
	return;
#endif

	if (LPFN_Disable) {
		FN_Wow64RevertWow64FsRedirection LPFN_Revert = (FN_Wow64RevertWow64FsRedirection)GetProcAddress(
			GetModuleHandle(TEXT("kernel32")), "Wow64RevertWow64FsRedirection");
		if (LPFN_Revert) {
			LPFN_Revert(m_OldValue);
		}
	}
}

Wow64RedirectOff::~Wow64RedirectOff() {
#ifdef _WIN64//X64_BIT
	return;
#endif

	if (LPFN_Disable) {
		FN_Wow64RevertWow64FsRedirection LPFN_Revert = (FN_Wow64RevertWow64FsRedirection)GetProcAddress(
			GetModuleHandle(TEXT("kernel32")), "Wow64RevertWow64FsRedirection");
		if (LPFN_Revert) {
			LPFN_Revert(m_OldValue);
		}
	}
}
