#include "stdafx.h"

#include <atlbase.h>
#include <atlstr.h>

#include "SCM.h"

///////////////////////////////////////////////////////////////
// class CNtDriverService

CNtDriverService::CNtDriverService()
{
	m_hSCManager = NULL;
}

CNtDriverService::CNtDriverService(LPCTSTR lpszName)
{
	m_hSCManager = NULL;
	m_strServiceName = lpszName;
}

CNtDriverService::~CNtDriverService()
{
	Destroy();
}

BOOL CNtDriverService::Initialize()
{
	m_hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(m_hSCManager == NULL)
		return FALSE;
	else
		return TRUE;
}

VOID CNtDriverService::Destroy()
{
	if(m_hSCManager != NULL) 
	{
		CloseServiceHandle(m_hSCManager);
		m_hSCManager = NULL;
	}
}

BOOL CNtDriverService::Install(LPCTSTR lpszExePath, LPCTSTR lpszDesc)
{
	if(lpszExePath == NULL)
		return FALSE;

	SC_HANDLE hNew = CreateService(m_hSCManager, m_strServiceName,
		lpszDesc,
		SC_MANAGER_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL, lpszExePath, NULL, NULL, NULL, NULL, NULL);
	if(hNew != NULL) {
		CloseServiceHandle(hNew);
		return TRUE;
	} else {
		if(GetLastError() == ERROR_SERVICE_EXISTS) {
			return UpdateExePath(lpszExePath, lpszDesc);
		}
		return FALSE;
	}
}

BOOL CNtDriverService::UpdateExePath(LPCTSTR lpszExePath, LPCTSTR lpszDesc)
{
	BOOL bResult = FALSE;
	SC_HANDLE hOpen = OpenService(m_hSCManager, m_strServiceName, SC_MANAGER_ALL_ACCESS);
	if(hOpen == NULL) {
		return FALSE;
	}

	bResult = ChangeServiceConfig(hOpen, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL, lpszExePath, NULL, NULL, NULL, NULL, NULL, lpszDesc);

	CloseServiceHandle(hOpen);
	return bResult;
}

VOID CNtDriverService::Uninstall()
{
	SC_HANDLE hOpen = OpenService(m_hSCManager, m_strServiceName, SC_MANAGER_ALL_ACCESS);
	if(hOpen == NULL) {
		return;
	}

	DeleteService(hOpen);
	CloseServiceHandle(hOpen);
}

void CNtDriverService::SetDriverName(LPCTSTR lpszName)
{
	m_strServiceName = lpszName;
}

BOOL CNtDriverService::Start()
{
	BOOL	  bRet		 = FALSE;
	SC_HANDLE schService = NULL;
	SERVICE_STATUS	ssStatus			= {0};
	DWORD			dwOldCheckPoint		= 0;
	DWORD			dwStartTickCount	= 0;
	DWORD			dwWaitTime			= 0;
	DWORD			dwStopStart			= ::GetTickCount();

	if (NULL == m_hSCManager)
	{
		WriteError(_T("m_hSCManager is NULL"));

		return bRet;
	}

	schService = OpenService(m_hSCManager, m_strServiceName, SC_MANAGER_ALL_ACCESS);
	if(schService == NULL) 
	{
		WriteError(_T("OpenService(%s) Failed! Err Code: %d"), m_strServiceName, ::GetLastError());

		return FALSE;
	}
	
	::QueryServiceStatus(schService, &ssStatus);	

	if (SERVICE_RUNNING == ssStatus.dwCurrentState)
	{
		WriteInfo(_T("Service is Running..."));
		::CloseServiceHandle(schService);
		return TRUE;
	}

	while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
	{
		WriteInfo(_T(" Start %s's is Pending , Wait for..."), m_strServiceName);
		::Sleep(500);
		if (!::QueryServiceStatus(schService, &ssStatus))
		{
			WriteError(_T("QueryServiceStatus(%s) Failed! Err Code: %d"), m_strServiceName, ::GetLastError());
			//::CloseServiceHandle(schService);
			break;
		}

		if (ssStatus.dwCurrentState == SERVICE_STOPPED)
		{
			WriteInfo(_T("Service %s is Stopped..."), m_strServiceName);
			//::CloseServiceHandle(schService);
			break;
		}

		// 超时检测
		if (::GetTickCount() - dwStopStart > 8000)
		{
			WriteError(_T("Wait for Service %s Time Out!"), m_strServiceName);
			//::CloseServiceHandle(schService);
			break;
		}
	}

	::QueryServiceStatus(schService, &ssStatus);

	if(SERVICE_STOP_PENDING == ssStatus.dwCurrentState)
	{
		WriteError(_T("CNtDriverService::Start():Service:%s is SERVICE_STOP_PENDING, return"), m_strServiceName);
		::CloseServiceHandle(schService);
		return FALSE;
	}

	if (!::StartService(schService, 0, NULL))
	{
		WriteError(_T("CNtDriverService::Start():StartService Failed! Err Code: %d"), ::GetLastError());
		::CloseServiceHandle(schService);
		return FALSE;
	}


	// Check the status until the service is no longer start pending.
	if (!::QueryServiceStatus(schService, &ssStatus))
	{
		WriteError(_T("QueryServiceStatus Failed! Err Code: %d"), ::GetLastError());
		::CloseServiceHandle(schService);
		return FALSE;
	}

	// Save the tick count and initial checkpoint.
	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;
	DWORD	dwWaitCount = 0;

	while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is
		// one tenth the wait hint, but no less than 1 second and no
		// more than 10 seconds.
		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
		{
			dwWaitTime = 1000;
		}
		else if (dwWaitTime > 10000)
		{
			dwWaitTime = 10000;
		}

		::Sleep(dwWaitTime);

		// Check the status again.
		if (!::QueryServiceStatus(schService, &ssStatus))
		{
			WriteError(_T("QueryServiceStatus Failed! Err Code: %d"), ::GetLastError());
			break;
		}
		
		if (dwWaitCount++ > 3)
		{
			// 等待三次循环以后，如果服务仍然没退出，则返回失败
			WriteError(_T("Service %s Start Error!"), m_strServiceName);
			break;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// The service is making progress.
			dwStartTickCount = ::GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (::GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				break;
			}
		}
	}

	if (ssStatus.dwCurrentState != SERVICE_RUNNING)
	{
		WriteError(_T("Invalid Service Status:%d!"), ssStatus.dwCurrentState);
		::CloseServiceHandle(schService);
		return FALSE;
	}

	::CloseServiceHandle(schService);
	WriteInfo(_T("Service is Running..."));
	return TRUE;
}

BOOL CNtDriverService::Stop()
{
	BOOL bResult = FALSE;
    SERVICE_STATUS	ssStatus = {0};

	if (NULL == m_hSCManager)
	{
		WriteError(_T("m_hSCManager is NULL"));

		return bResult;
	}

	SC_HANDLE schService = OpenService(m_hSCManager, m_strServiceName, SC_MANAGER_ALL_ACCESS);
	if(schService == NULL) 
	{
		WriteError(_T("OpenService(%s) Failed! Err Code: %d"), m_strServiceName, ::GetLastError());

		goto DONE;
	}

	DWORD	dwStartTime = GetTickCount();
	DWORD	dwTimeOut = 10000;	// 10秒超时
	DWORD	dwWaitCount = 0;

	// Make sure the service is not already stopped
	if (!::QueryServiceStatus(schService, &ssStatus))
	{
		WriteError(_T("QueryServiceStatus error, errno:%d"), GetLastError());
		goto DONE;
	}

	WriteInfo(_T("%s's Current is %d..."), m_strServiceName, ssStatus.dwCurrentState);

	if (ssStatus.dwCurrentState == SERVICE_STOPPED)
	{
		WriteInfo(_T("%s CurrentState is SERVICE_STOPPED"), m_strServiceName);
		bResult = TRUE;
		goto DONE;
	}

	if (!::ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus))
	{
		WriteError(_T("ControlService(%s) SERVICE_CONTROL_STOP Failed! Err Code: %d"), m_strServiceName, ::GetLastError());
		goto DONE;
	}	

	// Wait for the service to stop
	WriteInfo(_T("dwCurrentState:%d, 1-stop, 3-stopping"), ssStatus.dwCurrentState);
	//while (ssStatus.dwCurrentState != SERVICE_STOPPED)
	//{
	//	::Sleep(1000);

	//	if (ssStatus.dwCurrentState == SERVICE_STOPPED)
	//	{
	//		WriteInfo(_T("Service %s is Stopped..."), m_strServiceName);
	//		break;
	//	}
	//	if (dwWaitCount++ > 5)
	//	{
	//		// 等待三次循环以后，如果服务仍然没退出，则返回失败
	//		bResult = FALSE;
	//		WriteError(_T("Wait for Service %s Stopped Time out!, errno:%d"), m_strServiceName, GetLastError());
	//		goto DONE;
	//	}
	//}

	WriteInfo(_T("Stop Success State:%d"), ssStatus.dwCurrentState);

	bResult = TRUE;


DONE:
	if(schService != NULL)
	{
		::CloseServiceHandle(schService);
		schService = NULL;
	}

	return bResult;
}
