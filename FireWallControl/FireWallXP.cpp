#pragma once
#include "FireWallXP.h"

CFireWallXP::CFireWallXP()
{
}

CFireWallXP::~CFireWallXP()
{
}

BOOL CFireWallXP::InitCom()
{
	return 0;
}

BOOL CFireWallXP::UnInitCom()
{
	return 0;
}

BOOL CFireWallXP::getFirewallPly(DWORD* dwFirewall)
{
	return 0;
}

HRESULT CFireWallXP::WFCOMInitialize(INetFwPolicy2** ppNetFwPolicy2)
{
	return E_NOTIMPL;
}

BOOL CFireWallXP::QueryFwService(DWORD& dwCurrentState, DWORD& dwStartType)
{
	return 0;
}

BOOL CFireWallXP::ConfigFirewallEnable(BOOL bEnable)
{
	return 0;
}

BOOL CFireWallXP::StartMonitoring()
{
	return 0;
}