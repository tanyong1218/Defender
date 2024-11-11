#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include "SingletonClass.h"
#include "LogHelper.h"
using namespace std;

class WinTimerHelper : public Singleton<WinTimerHelper>
{
public:
	friend Singleton;
	~WinTimerHelper();
public:
	WinTimerHelper();

	BOOL CreatTimerToQueue(
		__in        WAITORTIMERCALLBACK Callback,
		__in_opt    PVOID Parameter,
		__in        DWORD DueTime,
		__in        DWORD Period,
		__in        ULONG Flags);

private:
	HANDLE m_hTimerQueue;
	vector<HANDLE> m_TimerHandleVector;
};
