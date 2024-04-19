// SystemLogControl.cpp : 定义 DLL 的导出函数。
//

#include "pch.h"
#include "framework.h"
#include "SystemLogControl.h"



std::mutex CSystemLogControl::m_MutexInstance;
CSystemLogControl::CSystemLogControl()
{
    return;
}

CSystemLogControl::~CSystemLogControl()
{
}

CSystemLogControl& CSystemLogControl::GetInstance()
{
    std::lock_guard<std::mutex> lock(m_MutexInstance);
    static CSystemLogControl instance;
    return instance;
}

DWORD CSystemLogControl::UnRegister()
{
    return 0;
}

IComponent* CSystemLogControl::Register()
{
    return nullptr;
}

BOOL CSystemLogControl::EnableFunction()
{
    CSysLogFun::GetInstance().GetSysLogByPsloglist(_T("02/28/2015"), _T("04/01/2024"), _T("System"));
    //CSysLogFun::GetInstance().GetSysLogByPsloglist(_T("02/28/2015"), _T("04/01/2024"), _T("Security"));
    //CSysLogFun::GetInstance().GetSysLogByPsloglist(_T("02/28/2015"), _T("04/01/2024"), _T("Application"));
    //CSysLogFun::GetInstance().GetSysLogByPsloglist(_T("02/28/2015"), _T("04/01/2024"), _T("Setup"));
    CSysLogFun::GetInstance().GetSysLogByEvtSubscribe();

    return 0;
}

BOOL CSystemLogControl::DisableFunction()
{
    return 0;
}


// 这是导出函数的一个示例。
SYSTEMLOGCONTROL_API IComponent * GetComInstance()
{
    WriteInfo("Welcome to SystemLogControl!");
    return &CSystemLogControl::GetInstance();
}
