#include "pch.h"
#include <Windows.h>
#include <tchar.h>
const TCHAR WLMINIDEVCTRL[] = _T("DynamicLinkLibrary.dll");
typedef int(_cdecl* TESTDLL)();
TEST(TestCaseName, TestName) 
{ 
	HMODULE hMoudle = ::LoadLibrary(WLMINIDEVCTRL);
	//����**.dll�к�����ΪTestFuction�Ķ���ӿ�
	if (hMoudle)
	{
		TESTDLL lpproc = (TESTDLL)GetProcAddress(hMoudle, "TestFuction");
		EXPECT_EQ(0, lpproc());
		EXPECT_TRUE(true);
	}
}
