#include "Main.h"

//TODO:命令行模式
//#include "tclap/CmdLine.h"
using namespace std;
void CheckServiceExist()
{
	if (!CWindowsHelper::IsProcessExist(_T("Service.exe")))
	{
		WriteInfo("Service.exe is not exist!");
		CWindowsHelper::StartProcess(_T("Service.exe"));   //默认当前目录
	}
}

int main(int argc, char** argv)
{
	//Init阶段  提权、添加定时器、声明变量
	WriteInfo("===================Main Begin=====================");
	SetConsoleOutputCP(65001); // 设置为UTF-8
	CWindowsHelper::EnablePrivilege(SE_DEBUG_NAME, FALSE);

	Timer timer;
	timer.add(1000 * 60, true, CheckServiceExist);  //每隔1min检测一次服务程序是否存在

	string strJson = "Hello World";
	int iLen = strJson.length();
	BYTE* BytePlyData = new BYTE[iLen + 1];
	memset(BytePlyData, 0, iLen + 1);
	memcpy(BytePlyData, strJson.c_str(), iLen);
	CWLMessageSender::SendMsg(CLIENT_MSG_CODE_DEVICE_CONTROL, DEVICE_CONTROL_OPEN_ALL_FUNCTION, iLen, BytePlyData);
	CWLMessageSender::SendMsg(CLIENT_MSG_CODE_SYSTEMLOG_CONTROL, SYSTEMLOG_CONTROL_OPEN_ALL_FUNCTION, iLen, BytePlyData);

	for (;;)
	{
		Sleep(1000);
	}

	WriteInfo("===================Main End=====================");
	return 0;
}