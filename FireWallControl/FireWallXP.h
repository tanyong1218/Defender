#pragma once
#include <netfw.h>
#include "FireWallBase.h"

class CFireWallXP : public CFirewallBase 
{
public:
	friend class CFirewallBase;
	CFireWallXP();

    ~CFireWallXP();

    BOOL InitCom() override;
    BOOL UnInitCom() override;
    BOOL getFirewallPly(DWORD *dwFirewall) override;

	HRESULT WFCOMInitialize(INetFwPolicy2** ppNetFwPolicy2) override;

    BOOL StartMonitoring() override;

    BOOL QueryFwService(DWORD& dwCurrentState, DWORD& dwStartType) override;
protected:
    BOOL ConfigFirewallEnable(BOOL bEnable) override;
};