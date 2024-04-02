
#include "Main.h"
int main(int argc, char const* argv[])
{
	WriteInfo("===================Begin=====================");
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
		WriteError("LoadLibrary failed");
	}

	WriteInfo("===================End=====================");
	return 0;
}




