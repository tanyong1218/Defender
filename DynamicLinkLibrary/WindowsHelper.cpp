#include "WindowsHelper.h"
#pragma comment(lib, "User32.lib")  

typedef void (WINAPI* PGNSI)(LPSYSTEM_INFO);
typedef BOOL(WINAPI* PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);
typedef int(__stdcall* FP_Wow64DisableWow64FsRedirection)(PVOID*);
typedef int(__stdcall* FP_Wow64RevertWow64FsRedirection)(PVOID);

CGetWindowsVersion::CGetWindowsVersion(void)
{
}

CGetWindowsVersion::~CGetWindowsVersion(void)
{
}

/*
* function : SeGetWindowsVersion
* description : 获取当前系统版本
* param : iWinVersion
* return : void
*/
void CGetWindowsVersion::SeGetWindowsVersion(int& iWinVersion)
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
			iWinVersion =  WIN_2003_SERVER_R2_X64;
		else
			iWinVersion =  WIN_2003_SERVER_R2_X32;
	}
	else if (dwMajorVersion == 6 && dwMinorVersion == 0)
	{
		if (b64Bit)
			iWinVersion = WIN_VISTA_X64;
		else
			iWinVersion =  WIN_VISTA_X32;
	}
	else if (dwMajorVersion == 6 && dwMinorVersion == 1)
	{
		if (b64Bit)
			iWinVersion =  WIN_7_X64;
		else
			iWinVersion =  WIN_7_X32;
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

BOOL CGetWindowsVersion::RtlGetOSVersionInfo(PRTL_OSVERSIONINFOEXW lpOsVersionInfo)
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
