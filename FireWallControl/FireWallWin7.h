#pragma once
#include <netfw.h>
#include "FireWallBase.h"


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


protected:
    BOOL ConfigFirewallEnable(BOOL bEnable) override;
};