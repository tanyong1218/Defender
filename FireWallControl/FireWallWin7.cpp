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

	// ��WFP����
	FWPM_SESSION session = { 0 };
	session.flags = FWPM_SESSION_FLAG_DYNAMIC;

	DWORD status = FwpmEngineOpen0(
		NULL,                   // ���ػ���
		RPC_C_AUTHN_WINNT,     // ��֤����
		NULL,                   // ��ȫ������
		&session,              // �Ự��Ϣ
		&m_hEngine            // ������
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

	//������ǽ����״̬
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

	//����
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

	//˽��
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

	//����
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

	// �����¼�������
	FWPM_NET_EVENT_ENUM_TEMPLATE0 enumTemplate = { 0 };
	enumTemplate.numFilterConditions = 0;  // ��ʹ�ù�����������������¼�

	// ���������¼�
	FWPM_NET_EVENT_SUBSCRIPTION0 subscription = { 0 };
	subscription.enumTemplate = &enumTemplate;

	HANDLE hSubscription = NULL;
	DWORD status = FwpmNetEventSubscribe0(
		m_hEngine,
		&subscription,
		OnNetworkEvent,        // �ص�����
		this,                  // ������
		&hSubscription
	);

	// ���WFP����������������
	FWPM_FILTER0 filter = { 0 };
	FWPM_FILTER_CONDITION0 condition[1] = { 0 };

	// ���ɹ�����GUID
	status = UuidCreate(&filter.filterKey);
	if (status != RPC_S_OK)
	{
		printf("UuidCreate failed with status %lu\n", status);
		return FALSE;
	}

	// ���ù���������
	filter.displayData.name = (wchar_t*)L"Connection Monitor Filter";
	filter.displayData.description = (wchar_t*)L"Monitors all connections";

	// ���ò�
	filter.layerKey = FWPM_LAYER_INBOUND_TRANSPORT_V4_GUID;  // ���IPv4����
	filter.action.type = FWP_ACTION_PERMIT;  // �������Ӳ���¼

	// ��ӹ�����
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

	printf("�ɹ����WFP������\n");
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

	// ��¼������Ϣ
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


