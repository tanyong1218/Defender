#pragma once
#include "CommonHeader.h"
class CWLBaseThread
{
public:
	CWLBaseThread(LPCTSTR lpEventName = NULL);
	virtual ~CWLBaseThread(void);

	BOOL CreateRunningThread(BOOL bRunNow = TRUE);
	BOOL SecureStopThread(DWORD dwWaitTime = INFINITE, BOOL bForceStop = FALSE);
	BOOL ResumeThread();
	BOOL SuspendThread();
	BOOL SetThreadPriority(int nPriority = THREAD_PRIORITY_NORMAL);
	BOOL IsThreadActive();
	BOOL UpdateNotifyTime(DWORD m_dwNotifyID);
	
	virtual BOOL CheckThreadStatus(DWORD dwWaitTime = 100);
	BOOL NotifyThreadStop();
	DWORD GetThreadId() const { return m_dwThreadId; }

protected:
	static unsigned int __stdcall ThreadRoutine(LPVOID lpParam);
	virtual DWORD MainThread(LPVOID pParam) = 0;

protected:
	HANDLE 	m_hStopEvent;
	HANDLE 	m_hThread;
	DWORD 	m_dwThreadId;
	time_t 	m_stLastUpdateTime;
};
