#pragma once
#include "BaseThread.h"
#include <netfw.h>
#define STATUS_DISABLE				0
#define STATUS_ENABLE				1
#define STATUS_NOCONFIG				2

//³éÏó¹¤³§
class CFirewallBase : public CWLBaseThread
{
public:	
	CFirewallBase(void);
    virtual ~CFirewallBase(void);
    static CFirewallBase *factory(const std::string &type);

	virtual BOOL InitCom()   = 0;
    virtual BOOL UnInitCom() = 0;
	virtual BOOL getFirewallPly(DWORD *dwFirewall) = 0;

	DWORD MainThread(LPVOID pParam);


	virtual BOOL ConfigFirewallEnable(BOOL bEnable) {return TRUE;}
	
	virtual BOOL StartMonitoring() = 0;

	virtual HRESULT WFCOMInitialize(INetFwPolicy2** ppNetFwPolicy2) = 0;

	virtual BOOL QueryFwService(DWORD &dwCurrentState, DWORD &dwStartType) = 0;
};