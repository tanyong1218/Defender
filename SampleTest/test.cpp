#include "pch.h"
#include <Windows.h>
#include <tchar.h>
const TCHAR WLMINIDEVCTRL[] = _T("DynamicLinkLibrary.dll");
typedef int(_cdecl* TESTDLL)();
TEST(TestCaseName, TestName) 
{ 
	HMODULE hMoudle = ::LoadLibrary(WLMINIDEVCTRL);
	//搜索**.dll中函数名为TestFuction的对外接口
	if (hMoudle)
	{
		TESTDLL lpproc = (TESTDLL)GetProcAddress(hMoudle, "TestFuction");
		EXPECT_EQ(0, lpproc());
		EXPECT_TRUE(true);
	}
}
