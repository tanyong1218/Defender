
#include "Main.h"
#include <TimerHelper.h>
#include <chrono>
void LogTime()
{
	char buffer[26];
	auto currentTime = std::chrono::system_clock::now();
	std::time_t current_time_t = std::chrono::system_clock::to_time_t(currentTime);
	ctime_s(buffer, sizeof(buffer), &current_time_t);
	WriteInfo(("Current time: {}"), buffer);
}



int main(int argc, char const* argv[])
{
	WriteInfo("===================Begin=====================");
	SetConsoleOutputCP(65001); // 设置为UTF-8
	CWindowsHelper::EnablePrivilege(SE_DEBUG_NAME, FALSE);

	Timer timer2;
	timer2.add(1000, true, LogTime);


	HMODULE hMoudle = ::LoadLibrary(DEVICECONTROL);
	//搜索**.dll中函数名为TestFuction的对外接口
	if (hMoudle)
	{
		TESTDLL lpproc = (TESTDLL)GetProcAddress(hMoudle, "TestFuction");
		lpproc();
	}
	else
	{
		//WriteError("LoadLibrary failed");
	}

	WriteInfo("===================End=====================");
	Sleep(10000);
	timer2.remove(0);
	for (;;)
	{
		Sleep(1000);
	}
	return 0;
}




