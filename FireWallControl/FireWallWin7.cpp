#pragma once
#include "FireWallWin7.h"
#include <LogHelper.h>
#include <WindowsHelper.h>

static const GUID FWPM_LAYER_INBOUND_TRANSPORT_V4_GUID = 
{ 0x5926dfc8, 0xe3cf, 0x4426, { 0xa2, 0x83, 0xdc, 0x39, 0x3f, 0x5d, 0x0f, 0x9d } };


CFireWallWin7::CFireWallWin7()
{
}

CFireWallWin7::~CFireWallWin7()
{
}

BOOL CFireWallWin7::InitCom()
{
	// Initialize COM.
	HRESULT hrComInit = CoInitializeEx(
		0,
		COINIT_APARTMENTTHREADED
	);

	// Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
	// initialized with a different mode. Since we don't care what the mode is,
	// we'll just use the existing mode.
	if (hrComInit != RPC_E_CHANGED_MODE)
	{
		if (FAILED(hrComInit))
		{
			return FALSE;
		}
	}

	// 打开WFP引擎
	FWPM_SESSION session = { 0 };
	session.flags = FWPM_SESSION_FLAG_DYNAMIC;

	DWORD status = FwpmEngineOpen0(
		NULL,                   // 本地机器
		RPC_C_AUTHN_WINNT,     // 认证服务
		NULL,                   // 安全上下文
		&session,              // 会话信息
		&m_hEngine            // 引擎句柄
	);

	return TRUE;
}

BOOL CFireWallWin7::UnInitCom()
{
	CoUninitialize();
	return TRUE;
}

BOOL CFireWallWin7::getFirewallPly(DWORD* dwFirewall)
{
	HRESULT                 hr = S_OK;
	INetFwPolicy2* pNetFwPolicy2 = NULL;
	VARIANT_BOOL            bIsEnabled = VARIANT_FALSE;
	DWORD                   dwRet = STATUS_ENABLE;
	BOOL                    bRet = FALSE;
	long                    lProfileTypes = 0;
	int nWindowsVersion = 0;
	BOOL bWin64 = FALSE;
	DWORD dwCurrentState = 0;
	DWORD dwStartType = 0;

	//检测防火墙服务状态
	if (!QueryFwService(dwCurrentState, dwStartType))
	{
		goto Cleanup;
	}

	if (dwCurrentState != SERVICE_RUNNING)
	{
		bRet = TRUE;
		dwRet = STATUS_DISABLE;
		goto Cleanup;
	}

	hr = WFCOMInitialize(&pNetFwPolicy2);
	if (FAILED(hr))
	{
		goto Cleanup;
	}

	//域网
	if (SUCCEEDED(pNetFwPolicy2->get_FirewallEnabled(NET_FW_PROFILE2_DOMAIN, &bIsEnabled)))
	{
		if (VARIANT_TRUE == bIsEnabled)
		{
			bRet = TRUE;
			goto Cleanup;
		}
	}
	else
	{
		goto Cleanup;
	}

	//私网
	if (SUCCEEDED(pNetFwPolicy2->get_FirewallEnabled(NET_FW_PROFILE2_PRIVATE, &bIsEnabled)))
	{
		if (VARIANT_TRUE == bIsEnabled)
		{
			bRet = TRUE;
			goto Cleanup;
		}
	}
	else
	{
		goto Cleanup;
	}

	//公网
	if (SUCCEEDED(pNetFwPolicy2->get_FirewallEnabled(NET_FW_PROFILE2_PUBLIC, &bIsEnabled)))
	{
		if (VARIANT_TRUE == bIsEnabled)
		{
			bRet = TRUE;
			goto Cleanup;
		}
	}
	else
	{
		goto Cleanup;
	}

	dwRet = STATUS_DISABLE;

Cleanup:

	*dwFirewall = dwRet;
	if (pNetFwPolicy2)
	{
		pNetFwPolicy2->Release();
	}

	return bRet;
}

HRESULT CFireWallWin7::WFCOMInitialize(INetFwPolicy2** ppNetFwPolicy2)
{
	HRESULT hr = S_OK;
	hr = CoCreateInstance(
		__uuidof(NetFwPolicy2),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(INetFwPolicy2),
		(void**)ppNetFwPolicy2);

	if (FAILED(hr))
	{
		goto Cleanup;
	}

Cleanup:
	return hr;
}

BOOL CFireWallWin7::QueryFwService(DWORD& dwCurrentState, DWORD& dwStartType)
{
	BOOL bRes = FALSE;
	wstring strErr;

	wstring strSrvName = _T("MpsSvc");

	if (!CWindowsHelper::QueryServiceInfoByName(strSrvName, dwCurrentState, dwStartType, &strErr))
	{
		goto END;
	}

	bRes = TRUE;
END:
	return bRes;
}

BOOL CFireWallWin7::StartMonitoring()
{
	if (!m_hEngine)
		return FALSE;

	// 创建事件过滤器
	FWPM_NET_EVENT_ENUM_TEMPLATE0 enumTemplate = { 0 };
	enumTemplate.numFilterConditions = 0;  // 不使用过滤条件，监控所有事件

	// 订阅连接事件
	FWPM_NET_EVENT_SUBSCRIPTION0 subscription = { 0 };
	subscription.enumTemplate = &enumTemplate;

	HANDLE hSubscription = NULL;
	DWORD status = FwpmNetEventSubscribe0(
		m_hEngine,
		&subscription,
		OnNetworkEvent,        // 回调函数
		this,                  // 上下文
		&hSubscription
	);

	// 添加WFP过滤器来捕获连接
	FWPM_FILTER0 filter = { 0 };
	FWPM_FILTER_CONDITION0 condition[1] = { 0 };

	// 生成过滤器GUID
	status = UuidCreate(&filter.filterKey);
	if (status != RPC_S_OK)
	{
		printf("UuidCreate failed with status %lu\n", status);
		return FALSE;
	}

	// 设置过滤器参数
	filter.displayData.name = (wchar_t*)L"Connection Monitor Filter";
	filter.displayData.description = (wchar_t*)L"Monitors all connections";

	// 设置层
	filter.layerKey = FWPM_LAYER_INBOUND_TRANSPORT_V4_GUID;  // 监控IPv4连接
	filter.action.type = FWP_ACTION_PERMIT;  // 允许连接并记录

	// 添加过滤器
	status = FwpmFilterAdd0(
		m_hEngine,
		&filter,
		NULL,
		NULL);

	if (status != ERROR_SUCCESS)
	{
		printf("FwpmFilterAdd0 failed with status %lu\n", status);
		return FALSE;
	}

	printf("成功添加WFP过滤器\n");
	return TRUE;

	return (status == ERROR_SUCCESS);
}

void CFireWallWin7::OnNetworkEvent(_Inout_ void* context, _In_ const FWPM_NET_EVENT1* event)
{
	CFireWallWin7* pThis = (CFireWallWin7*)context;
	pThis->HandleNetworkEvent(event);

}

void CFireWallWin7::HandleNetworkEvent(const FWPM_NET_EVENT1* event)
{
	if (!event)
		return;

	// 记录连接信息
	switch (event->type)
	{
	case FWPM_NET_EVENT_TYPE_IKEEXT_MM_FAILURE:
	{
		break;
	}
	}
}


BOOL CFireWallWin7::ConfigFirewallEnable(BOOL bEnable)
{
	return 0;
}


