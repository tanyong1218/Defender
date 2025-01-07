#include "FireWallBase.h"
#include "FireWallWin7.h"
#include "FireWallXP.h"
CFirewallBase::CFirewallBase(void)
{
}

CFirewallBase::~CFirewallBase(void)
{
}

CFirewallBase* CFirewallBase::factory(const std::string& type)
{
	if (type == "Win7")
	{
		return new CFireWallWin7;
	}
	else if (type == "WinXP")
	{
		return new CFireWallXP;
	}
	return nullptr;
}

DWORD CFirewallBase::MainThread(LPVOID pParam)
{
	return 0;
}
