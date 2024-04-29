#pragma once
#ifdef SYSTEMLOGCONTROL_EXPORTS
#define SYSTEMLOGCONTROL_API __declspec(dllexport)
#else
#define SYSTEMLOGCONTROL_API __declspec(dllimport)
#endif

#include "Common.h"
#include "SysLogFun.h"
#include "IComponent.h"



class CSystemLogControl : public IComponent {
public:
	~CSystemLogControl();
	CSystemLogControl(const CSystemLogControl&) = delete;
	CSystemLogControl& operator=(const CSystemLogControl&) = delete;
	static CSystemLogControl& GetInstance();
public:
	DWORD UnRegister();
	IComponent* Register();
	BOOL EnableFunction();
	BOOL DisableFunction();
	BOOL DispatchMessages(IPC_MSG_DATA* pIpcMsg);
private:
	CSystemLogControl(void);
};

extern "C" __declspec(dllexport) IComponent* GetComInstance();