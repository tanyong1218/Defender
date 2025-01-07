#include "BaseThread.h"
#include <time.h>

CWLBaseThread::CWLBaseThread(LPCTSTR lpEventName)
:m_hThread(NULL)
,m_hStopEvent(NULL)
,m_dwThreadId(0)
{
	// The initialized state of the STOP event is non-signaled.
	m_hStopEvent = ::CreateEvent(NULL, TRUE, FALSE, lpEventName);
	m_stLastUpdateTime = time(NULL);
}

CWLBaseThread::~CWLBaseThread(void)
{
	if (m_hThread)
	{
		::CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	if (m_hStopEvent)
	{
		::CloseHandle(m_hStopEvent);
		m_hStopEvent = NULL;
	}
}

BOOL CWLBaseThread::CreateRunningThread(BOOL bRunNow)
{
	if (NULL == m_hThread)
	{
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, CWLBaseThread::ThreadRoutine, 
			reinterpret_cast<LPVOID>(this), bRunNow ? 0 : CREATE_SUSPENDED, (unsigned*)&m_dwThreadId);
		if (m_hThread) 
		{
			if (bRunNow) 
			{	// Resume running the thread.
				this->ResumeThread();
			}
		}
	}

	return (NULL != m_hThread);
}

BOOL CWLBaseThread::SecureStopThread(DWORD dwWaitTime, BOOL bForceStop)
{
	if (NULL == m_hThread)
	{
		return TRUE;
	}

	DWORD dwExitCode = 0;
	if (::GetExitCodeThread(m_hThread, &dwExitCode)) 
	{
		if (STILL_ACTIVE == dwExitCode)
		{	// Set the stop event to signaled state, then WorkRoutine() should exit.
			if (m_hStopEvent && ::SetEvent(m_hStopEvent))
			{
				// Wait the thread to exit successfully.
				DWORD dwWaitResult = ::WaitForSingleObject(m_hThread, dwWaitTime);
				if (WAIT_TIMEOUT == dwWaitResult || WAIT_FAILED == dwWaitResult)
				{
                    // 如果设置 bForceStop = TRUE，会强制杀死线程，有可能导致部分资源没有释放干净，造成异常。
					if (bForceStop)
					{	// Terminate the thread forcedly since it can't exit by itself.
						if (::TerminateThread(m_hThread, dwExitCode))
						{
							ResetEvent(m_hStopEvent);
							::CloseHandle(m_hThread);
							m_hThread = NULL;
							return TRUE;
						}
						else
						{

							return FALSE;
						}
						
					}
					else
					{	// can't stop the thread normally.

						return FALSE;
					}
				}
				else
				{

					ResetEvent(m_hStopEvent);
					::CloseHandle(m_hThread);
					m_hThread = NULL;
				}
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			ResetEvent(m_hStopEvent);
			::CloseHandle(m_hThread);
			m_hThread = NULL;
			
		}
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CWLBaseThread::ResumeThread()
{
	if (m_hThread)
	{
		DWORD dwResult = ::ResumeThread(m_hThread);
		return (-1 != dwResult);
	}

	return FALSE;
}


BOOL CWLBaseThread::SuspendThread()
{
	if (m_hThread) 
	{
		DWORD dwResult = ::SuspendThread(m_hThread);
		return (-1 != dwResult);
	}

	return FALSE;
}

BOOL CWLBaseThread::SetThreadPriority(int nPriority)
{
	if (m_hThread)
	{
		return ::SetThreadPriority(m_hThread, nPriority);
	}

	return FALSE;
}

BOOL CWLBaseThread::IsThreadActive()
{
	if (m_hThread)
	{
		DWORD dwExitCode = 0;
		if (::GetExitCodeThread(m_hThread, &dwExitCode)) {
			return (STILL_ACTIVE == dwExitCode);
		}
	}

	return FALSE;
}

BOOL CWLBaseThread::UpdateNotifyTime(DWORD m_dwNotifyID)
{
	return TRUE;
}


unsigned int __stdcall CWLBaseThread::ThreadRoutine(LPVOID lpParam)
{
	unsigned int nRetCode = 0;

	CWLBaseThread *pThread = (CWLBaseThread *)(lpParam);
	if (pThread)
	{
		nRetCode = pThread->MainThread(pThread);

		pThread->NotifyThreadStop();
	}

	return nRetCode;
}

BOOL CWLBaseThread::CheckThreadStatus(DWORD dwWaitTime)
{
	try
	{
		if (NULL == m_hStopEvent)
		{
			m_hStopEvent = ::CreateEventA(NULL, TRUE, FALSE, NULL);
		}

		if (m_hStopEvent)
		{		
			DWORD dwWaitResult = ::WaitForSingleObject(m_hStopEvent, dwWaitTime);
			if (WAIT_OBJECT_0 == dwWaitResult || WAIT_ABANDONED == dwWaitResult)
			{
				// This thread should exit now.
				
				return FALSE;
			}
		}

	}
	catch (...)
	{
		::Sleep(dwWaitTime);
	}

	return TRUE;
}

BOOL CWLBaseThread::NotifyThreadStop()
{
	if (m_hStopEvent)
	{
		return ::SetEvent(m_hStopEvent);
	}
	return FALSE;
}
