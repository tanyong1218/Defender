﻿// SystemLogControl.cpp : 定义 DLL 的导出函数。
//

#include "pch.h"
#include "framework.h"
#include "SystemLogControl.h"

CSystemLogControl::CSystemLogControl()
{
	return;
}

CSystemLogControl::~CSystemLogControl()
{
	WriteInfo("CSystemLogControlClass  Buffer Free");
}

CSystemLogControl& CSystemLogControl::GetInstance()
{
	static CSystemLogControl instance;
	return instance;
}


DWORD CSystemLogControl::UnRegister()
{
	return 0;
}

IComponent* CSystemLogControl::Register()
{
	return &GetInstance();
}

BOOL CSystemLogControl::EnableFunction()
{
	WriteInfo("EnableFunction CSystemLogControl");
	CSysLogFun::GetInstance().GetSysLogByPsloglist(_T("02/28/2015"), _T("04/01/2024"), _T("System"));
	CSysLogFun::GetInstance().GetSysLogByEvtSubscribe();
	CSysLogFun::GetInstance().GetSysLogByReadEventLog();

	return 0;
}

BOOL CSystemLogControl::DisableFunction()
{
	CSysLogFun::GetInstance().RecycleSysLogResource();
	return 0;
}

BOOL CSystemLogControl::DispatchMessages(IPC_MSG_DATA* pIpcMsg)
{
	if (!pIpcMsg)
	{
		return FALSE;
	}

	std::string strData(reinterpret_cast<char*>(pIpcMsg->Data), pIpcMsg->dwSize);
	switch (pIpcMsg->dwMsgCode)
	{
	case SYSTEMLOG_CONTROL_OPEN_ALL_FUNCTION:
		EnableFunction();
		break;
	case SYSTEMLOG_CONTROL_CLOSE_ALL_FUNCTION:
		DisableFunction();
		break;
	default:
		break;
	}
	return TRUE;
}

// 这是导出函数的一个示例。
SYSTEMLOGCONTROL_API IComponent* GetComInstance()
{
	WriteInfo("Welcome to SystemLogControl!");
	return &CSystemLogControl::GetInstance();
}

