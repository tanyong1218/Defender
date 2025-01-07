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

protected:
    BOOL ConfigFirewallEnable(BOOL bEnable) override;
};