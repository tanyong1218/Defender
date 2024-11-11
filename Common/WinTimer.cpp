#include "WinTimer.h"


WinTimerHelper::WinTimerHelper()
{
	m_hTimerQueue = CreateTimerQueue();
}

WinTimerHelper::~WinTimerHelper()
{
	for (auto hTimer : m_TimerHandleVector)
	{
		if (!DeleteTimerQueueTimer(m_hTimerQueue, hTimer, NULL))
		{
			//ɾ��ʧ��
			WriteError(("DeleteTimerQueueTimer fail, errno={}"), GetLastError());
		}
	}
	
	// ɾ����ʱ������
	if (!DeleteTimerQueue(m_hTimerQueue))
	{
		WriteError(("DeleteTimerQueue fail, errno={}"), GetLastError());
	}

}

BOOL WinTimerHelper::CreatTimerToQueue(WAITORTIMERCALLBACK Callback, PVOID Parameter, DWORD DueTime, DWORD Period, ULONG Flags)
{
	HANDLE hTimer = NULL;
	if (!CreateTimerQueueTimer(&hTimer, m_hTimerQueue, Callback, Parameter, DueTime, Period, Flags))
	{
		WriteError(("CreateTimerQueueTimer fail, errno={}"), GetLastError());
		return FALSE;
	}

	m_TimerHandleVector.push_back(hTimer);

	return TRUE;
}
