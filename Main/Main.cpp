#include "Main.h"
#include "WinTimer.h"
//TODO:命令行模式
//#include "tclap/CmdLine.h"
using namespace std;


VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	if (!CWindowsHelper::IsProcessExist(_T("Service.exe")))
	{
		WriteInfo("Service.exe is not exist!");
		CWindowsHelper::StartProcess(_T("Service.exe"));   //默认当前目录
	}
}


int main(int argc, char** argv)
{
	//Init阶段  提权、防止多开、声明变量
	WriteInfo("===================Main Begin=====================");

	SetConsoleOutputCP(65001); // 设置为UTF-8
	CWindowsHelper::EnablePrivilege(SE_DEBUG_NAME, FALSE);

	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, WL_MAIN_SINGTON_EVENT_NAME);
	if (!hEvent)
	{
		WriteError(("CreateEvent WL_MAIN_SINGTON_EVENT_NAME fail, errno={}"), GetLastError());
		return 0;
	}
	else if (ERROR_ALREADY_EXISTS == GetLastError())
	{
		WriteInfo(("MAIN ERROR_ALREADY_EXISTS"));
		return 0;
	}

	//定时器定时检测Service.exe程序
	WinTimerHelper::GetInstance().CreatTimerToQueue(TimerRoutine, NULL, 600 * 1000, 600 * 1000, 0);

	//Timer timer;
	//timer.add(1000 * 60 * 10, true, CheckServiceExist);  //每隔10min检测一次服务程序是否存在

	string strJson = "Hello World";
	int iLen = (int)strJson.length();
	BYTE* BytePlyData = new BYTE[iLen + 1];
	memset(BytePlyData, 0, iLen + 1);
	memcpy(BytePlyData, strJson.c_str(), iLen);
	
	CWLMessageSender *pSender = new CWLMessageSender();
	//pSender->SendMsgToMmf(CLIENT_MSG_CODE_FILESCAN_CONTROL, FILESCAN_CONTROL_OPEN_ALL_FUNCTION, iLen, BytePlyData);
	//pSender->SendMsgToMmf(CLIENT_MSG_CODE_FIREWALL_CONTROL, FIREWALL_CONTROL_OPEN_ALL_FUNCTION, 0, nullptr);

	pSender->SendMsgToMmf(CLIENT_MSG_CODE_CLIPBOARD_CONTROL, CLIPBOARD_CONTROL_OPEN_ALL_FUNCTION, 0, nullptr);

	for (;;)
	{
		Sleep(1000);
	}

	Sleep(10000);
	WriteInfo("===================Main End=====================");
	return 0;
}