
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

void TimeDemo()
{
	Timer timer2;
	timer2.add(1000000, true, LogTime);

	Timer time;
	int iNumber = 100;
	time.add(1000000, false, [iNumber]() {
		WriteInfo("Current time: {}", iNumber);
		});

}

int main(int argc, char const* argv[])
{
	WriteInfo("===================Begin=====================");
	SetConsoleOutputCP(65001); // 设置为UTF-8
	CWindowsHelper::EnablePrivilege(SE_DEBUG_NAME, FALSE);
	TimeDemo();

	HMODULE hMoudle = ::LoadLibrary(DEVICECONTROL);
	//搜索**.dll中函数名为TestFuction的对外接口
	if (hMoudle)
	{
		TESTDLL lpproc = (TESTDLL)GetProcAddress(hMoudle, "EnableDeviceControl");
		if (lpproc)
		{
			lpproc();
		}
		
	}
	else
	{
		WriteError("LoadLibrary failed");
	}

	for (;;)
	{
		Sleep(1000);
	}
	WriteInfo("===================End=====================");
	return 0;
}




