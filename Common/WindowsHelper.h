#pragma once
#include <Windows.h>
#include <string>
#include <TlHelp32.h>
#include <algorithm>
#include <tchar.h>
#include <comdef.h>
#include <sddl.h>
#include <pathcch.h>
#include <boost/filesystem.hpp>
#pragma comment(lib, "advapi32.lib")
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
using namespace std;
namespace fs = boost::filesystem;
enum {
	WIN_2000 = int(1),
	WIN_XP,
	WIN_2003_SERVER_R2_X32,
	WIN_2003_SERVER_R2_X64,
	WIN_2008_SERVER_X32,
	WIN_2008_SERVER_X64,
	WIN_2008_SERVER_R2_X32,
	WIN_2008_SERVER_R2_X64,
	WIN_VISTA_X32,
	WIN_VISTA_X64,
	WIN_7_X32,
	WIN_7_X64,
	WIN_8_0_X32,
	WIN_8_0_X64,
	WIN_8_1_X32,
	WIN_8_1_X64,
	WIN_2012_X32,
	WIN_2012_X64,
	WIN_2012_R2_X32,
	WIN_2012_R2_X64,
	WIN_10_X32,
	WIN_10_X64,
	WIN_11_X32,
	WIN_11_X64,
	WIN_2016_X32,
	WIN_2016_X64,
	WIN_2019_X32,
	WIN_2019_X64,
	WIN_2022_X32,
	WIN_2022_X64
};

// winnt.hµÄ²¹³ä
#define PRODUCT_HOME_PREMIUM_N                      0x0000001A
#define PRODUCT_ENTERPRISE_N                        0x0000001B
#define PRODUCT_ULTIMATE_N                          0x0000001C
#define PRODUCT_WEB_SERVER_CORE                     0x0000001D
#define PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT    0x0000001E
#define PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY      0x0000001F
#define PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING     0x00000020
#define PRODUCT_SMALLBUSINESS_SERVER_PRIME          0x00000021
#define PRODUCT_HOME_PREMIUM_SERVER                 0x00000022
#define PRODUCT_SERVER_FOR_SMALLBUSINESS_V          0x00000023
#define PRODUCT_STANDARD_SERVER_V                   0x00000024
#define PRODUCT_DATACENTER_SERVER_V                 0x00000025
#define PRODUCT_ENTERPRISE_SERVER_V                 0x00000026
#define PRODUCT_DATACENTER_SERVER_CORE_V            0x00000027
#define PRODUCT_STANDARD_SERVER_CORE_V              0x00000028
#define PRODUCT_ENTERPRISE_SERVER_CORE_V            0x00000029
#define PRODUCT_HYPERV                              0x0000002A
#define PRODUCT_PROFESSIONAL                        0x00000030

#define VER_SUITE_WH_SERVER							0x00008000

class CWindowsHelper
{
public:
	CWindowsHelper(void);
	~CWindowsHelper(void);

	static void SeGetWindowsVersion(int& iWinVersion);
	static BOOL RtlGetOSVersionInfo(PRTL_OSVERSIONINFOEXW lpOsVersionInfo);
	static BOOL GetOnePIDBelongUserName(wstring wstrUserName, DWORD& dwPid, wstring& wstrErr);
	static BOOL EnablePrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	static BOOL GetProcessIdFromName(const std::wstring& processName, DWORD& dwPid);

	static BOOL GetSIDByUserName_Wmic(std::wstring& strUserSid, LPCTSTR pszUserName);
	static BOOL GetSIDByUserName_Lookup(std::wstring& strUserSid, LPCTSTR pszUserName);
	static BOOL GetSIDByUserName(std::wstring& strUserSid, LPCTSTR pszUserName);

	static wstring GetRunDir();
	static wstring GetSystemDir();

	static BOOL IsProcessExist(LPCTSTR pszProcessName);
	static BOOL StartProcess(LPCTSTR pszProcessName);
};