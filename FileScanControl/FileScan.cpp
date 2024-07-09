#include "FileScan.h"

CFileScan::CFileScan()
{
	return;
}

CFileScan::~CFileScan()
{
}

CFileScan& CFileScan::GetInstance()
{
	static CFileScan instance;
	return instance;
}

DWORD CFileScan::UnRegister()
{
	return 0;
}

IComponent* CFileScan::Register()
{
	return nullptr;
}

BOOL CFileScan::EnableFunction()
{
	auto FileScan = new CFileScanFun();
	FileScan->EnableScanFileFunction();
	return 0;
}

BOOL CFileScan::DisableFunction()
{
	return 0;
}

BOOL CFileScan::DispatchMessages(IPC_MSG_DATA* pIpcMsg)
{
	std::string strData(reinterpret_cast<char*>(pIpcMsg->Data), pIpcMsg->dwSize);
	switch (pIpcMsg->dwMsgCode)
	{
	case FILESCAN_CONTROL_OPEN_ALL_FUNCTION:
		EnableFunction();
		break;
	case FILESCAN_CONTROL_CLOSE_ALL_FUNCTION:
		DisableFunction();
		break;
	}
	return 0;
}

// ���ǵ���������һ��ʾ����
FILESCANCONTROL_EXPORTS IComponent* GetComInstance()
{
	WriteInfo("Welcome to FileScanControl!");
	return &CFileScan::GetInstance();
}