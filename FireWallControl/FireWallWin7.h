#pragma once
#include <netfw.h>
#include "FireWallBase.h"


class CFireWallWin7 : public CFirewallBase 
{
public:

    //friend�������CFirewallBase���Է���CFireWallWin7��˽�кͱ����ĳ�Ա�����ͺ���
    friend class CFirewallBase;
    CFireWallWin7();
    ~CFireWallWin7();

    BOOL InitCom() override;
    BOOL UnInitCom() override;
    BOOL getFirewallPly(DWORD *dwFirewall) override;


protected:
    BOOL ConfigFirewallEnable(BOOL bEnable) override;
};