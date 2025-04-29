#pragma once
#include <string>
#include <Windows.h>
#include "IComponent.h"

using namespace std;
class Clipboard : public IComponent
{
public:

	~Clipboard();

	static Clipboard* GetInstance();
	static void ReleaseInstance();

	DWORD UnRegister();
	IComponent* Register();
	BOOL EnableFunction();
	BOOL DisableFunction();
	BOOL DispatchMessages(IPC_MSG_DATA* pIpcMsg);

private:
	Clipboard();
	static Clipboard* m_instance;

	HANDLE m_hClipboard;

};

extern "C" __declspec(dllexport) IComponent * GetComInstance();
extern "C" __declspec(dllexport) void ReleaseComInstance();