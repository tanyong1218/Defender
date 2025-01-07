#pragma once
#include "BaseThread.h"

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

};