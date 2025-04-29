#pragma once
#include <netfw.h>
#include "FireWallBase.h"
#include <fwpmu.h>
#include <fwpmtypes.h>

#pragma comment(lib, "fwpuclnt.lib")
#pragma comment(lib, "rpcrt4.lib")

class CFireWallWin7 : public CFirewallBase 
{
public:

    //friend代表可以CFirewallBase可以访问CFireWallWin7的私有和保护的成员变量和函数
    friend class CFirewallBase;

    CFireWallWin7();
    ~CFireWallWin7();

    BOOL InitCom() override;
    BOOL UnInitCom() override;
    BOOL getFirewallPly(DWORD *dwFirewall) override;

	HRESULT WFCOMInitialize(INetFwPolicy2** ppNetFwPolicy2) override;
	BOOL QueryFwService(DWORD &dwCurrentState, DWORD &dwStartType) override;

    BOOL StartMonitoring() override;

    static void CALLBACK OnNetworkEvent(_Inout_ void* context,_In_ const FWPM_NET_EVENT1* event);
    void HandleNetworkEvent(const FWPM_NET_EVENT1* event);
protected:
    BOOL ConfigFirewallEnable(BOOL bEnable) override;
private:
	HANDLE m_hEngine;
};