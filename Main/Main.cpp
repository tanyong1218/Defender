
#include "Main.h"
#include <TimerHelper.h>
#include <chrono>
#include <ThreadPoolHelper.h>
#include <FileOperationHelper.h>

//TODO:命令行模式
//#include "tclap/CmdLine.h"

using namespace std;

void LogTime()
{
	char buffer[26];
	auto currentTime = std::chrono::system_clock::now();
	std::time_t current_time_t = std::chrono::system_clock::to_time_t(currentTime);
	ctime_s(buffer, sizeof(buffer), &current_time_t);
	WriteInfo(("Current time: {}"), buffer);
}
void LogNumner(int iNumber)
{
	WriteInfo(("Current iNumber: {}"), iNumber);
}

void TimeDemo()
{
	Timer timer2;
	timer2.add(10000, true, LogTime);

	Timer time;
	int iNumber = 100;
	time.add(10000, false, [iNumber]() {
		WriteInfo("Current time: {}", iNumber);
		});
}

int main(int argc, char** argv)
{
	WriteInfo("===================Begin=====================");
	SetConsoleOutputCP(65001); // 设置为UTF-8
	CWindowsHelper::EnablePrivilege(SE_DEBUG_NAME, FALSE);
	IComponent* pIComponent = nullptr;

	//定时器
	TimeDemo();
	//线程池
	const size_t numThreads = 8;
	ThreadPool threadPool(numThreads);
	threadPool.enqueue(LogTime);
	threadPool.enqueue(LogNumner, 100);

	//获取当前用户的SID
	wstring wstrSID;
	CWindowsHelper::GetSIDByUserName(wstrSID, _T("Administrator"));

	for (auto& wstDLLName : g_LoadMoudleVector)
	{
		HMODULE hMoudle = ::LoadLibrary(wstDLLName.c_str());
		TESTDLL lpproc = (TESTDLL)GetProcAddress(hMoudle, "GetComInstance");
		if (lpproc)
		{
			auto Instance = lpproc();
			g_IComponentVector.push_back(std::make_shared<IComponent*>(Instance));
		}
		else
		{
			WriteError("LoadLibrary failed");
		}
	}

	for (const auto& ComponentPtr : g_IComponentVector)
	{
		IComponent* Component = *ComponentPtr;
		Component->EnableFunction();
	}

	for (;;)
	{
		Sleep(1000);
	}

	for (const auto& ComponentPtr : g_IComponentVector)
	{
		IComponent* Component = *ComponentPtr;
		Component->DisableFunction();
	}


	WriteInfo("===================End=====================");
	return 0;
}




