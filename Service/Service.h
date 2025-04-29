#pragma once
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <tchar.h>
#include <vector>
#include <map>
#include <userenv.h>
#include <LogHelper.h>
#include <WindowsHelper.h>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include "IComponent.h"
#include <TimerHelper.h>
#include <IPCMmf.h>
#include <queue>
#include <MessageQueue.h>
#include <MessageSender.h>
#include <HardDiskHelper.h>
#include <FileOperationHelper.h>
#include <JsonParse.h>
#include <PETools.h>
#include <png.h>
#define DEVICECONTROL		_T("DeviceControl.dll")
#define SYSTEMLOGCONTROL	_T("SystemLogControl.dll")
#define	FILESCANCONTROL		_T("FileScanControl.dll")
#define	FIREWALLCONTROL		_T("FireWallControl.dll")
#define	CLIPBOARDCONTROL	_T("ClipboardControl.dll")
vector<wstring> g_LoadMoudleVector
{
	DEVICECONTROL,
	SYSTEMLOGCONTROL,
	FILESCANCONTROL,
	FIREWALLCONTROL,
	CLIPBOARDCONTROL
};

class CMessageHelper : public Singleton<CMessageHelper>
{
public:
	friend Singleton;

	~CMessageHelper();
public:
	BOOL InitMessageHelper();
	static unsigned int	WINAPI GetMessageThread(LPVOID lpParameter);
	static unsigned int	WINAPI DispatchMessageThread(LPVOID lpParameter);
	BOOL DispatchMessageFun(IPC_MSG_DATA* MessageData);

private:
	CWLMetaDataQueue* m_pCWLMetaDataQueue;
	CMessageHelper();
};

typedef IComponent* (_cdecl* ICOMFUNCTION)();

typedef void (_cdecl* RELEASECOMINSTANCE)();

std::map<wstring, std::shared_ptr<IComponent*>> g_IComponentVector;