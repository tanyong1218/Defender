#include "Service.h"


CMessageHelper::CMessageHelper()
{
	m_pCWLMetaDataQueue = new CWLMetaDataQueue();
}

CMessageHelper::~CMessageHelper()
{
	delete m_pCWLMetaDataQueue;
}

BOOL CMessageHelper::InitMessageHelper()
{
	HANDLE hGetMessageThread = (HANDLE)_beginthreadex(NULL, 0, GetMessageThread, this, 0, NULL);
	HANDLE hDispatchMessageThread = (HANDLE)_beginthreadex(NULL, 0, DispatchMessageThread, this, 0, NULL);
	if (hGetMessageThread)
	{
		CloseHandle(hGetMessageThread);
	}
	if (hDispatchMessageThread)
	{
		CloseHandle(hDispatchMessageThread);
	}
	return 0;
}

//创建消息采集线程，负责将消息从共享内存中取出来存放到Queue中
unsigned int WINAPI CMessageHelper::GetMessageThread(LPVOID lpParameter)
{
	CMessageHelper* pMessageHelper = (CMessageHelper*)lpParameter;
	while (true)
	{
		CWLIPCMmf* pIPCContainer = new CWLIPCMmf(IPC_CFG_MMF_NAME_SERVER, IPC_CFG_MUTEX_NAME_SERVER, NULL, DEFAULT_MMF_BUFFER_SIZE);
		BYTE* pMsgData = NULL;
		DWORD dwSize = 0;

		if (ERROR_SUCCESS == pIPCContainer->ReadData(dwSize, pMsgData))
		try
		{
			DWORD dwSizeDone = 0;
			while (dwSizeDone + IPC_MSG_DATA_HEADNER_LEN <= dwSize)
			{
				IPC_MSG_DATA* pMsg = reinterpret_cast<IPC_MSG_DATA*>(pMsgData + dwSizeDone);
				pMessageHelper->m_pCWLMetaDataQueue->Insert(pMsg);
				dwSizeDone += IPC_MSG_DATA_HEADNER_LEN + pMsg->dwSize;
			}
		}
		catch (exception e)
		{
			WriteError(("catch exception {}"), e.what());
		}
		catch (...)
		{
			WriteError(("unexception catch error."));
		}

		Sleep(1000);
	}
	_endthreadex(0);
	return 0;
}

unsigned int WINAPI CMessageHelper::DispatchMessageThread(LPVOID lpParameter)
{
	CMessageHelper* pMessageHelper = (CMessageHelper*)lpParameter;

	while (true)
	{
		if (pMessageHelper->m_pCWLMetaDataQueue->GetCount() > 0)
		{
			IPC_MSG_DATA* MessageData = pMessageHelper->m_pCWLMetaDataQueue->GetHead();
			pMessageHelper->DispatchMessageFun(MessageData);
		}
	}

	_endthreadex(0);
	return 0;

}

//消息分发模块
BOOL CMessageHelper::DispatchMessageFun(IPC_MSG_DATA* MessageData)
{
	IComponent* Component = nullptr;
	switch (MessageData->dwMsgCode)
	{
	case CLIENT_MSG_CODE_DEVICE_CONTROL:
		Component = *g_IComponentVector[DEVICECONTROL];
		Component->DispatchMessages(MessageData);
		break;
	case CLIENT_MSG_CODE_SYSTEMLOG_CONTROL:
		Component = *g_IComponentVector[SYSTEMLOGCONTROL];
		Component->DispatchMessages(MessageData);
		break;
	default:
		break;
	}
	return 0;
}
int main(int argc, char** argv)
{
	WriteInfo("===================Service Begin=====================");
	
	IComponent* pIComponent = nullptr;
	for (auto& wstDLLName : g_LoadMoudleVector)
	{
		HMODULE hMoudle = ::LoadLibrary(wstDLLName.c_str());
		ICOMFUNCTION lpproc = (ICOMFUNCTION)GetProcAddress(hMoudle, "GetComInstance");
		if (lpproc)
		{
			auto Instance = lpproc();
			g_IComponentVector[wstDLLName] = std::make_shared<IComponent*>(Instance);
		}
		else
		{
			WriteError("LoadLibrary failed");
		}
	}

	CMessageHelper::GetInstance().InitMessageHelper();

	/*
	for (const auto& ComponentPtr : g_IComponentVector)
	{
		IComponent* Component = *ComponentPtr;
		Component->EnableFunction();
	}
	*/

	/*
	// 使用 lambda 表达式执行 EnableFunction 操作
	std::for_each(g_IComponentVector.begin(), g_IComponentVector.end(), [](std::shared_ptr<IComponent*> componentPtr) {
		IComponent* component = *(componentPtr.get());
		component->EnableFunction();
		});
	*/

	for (;;)
	{
		Sleep(1000);
	}

	for (const auto& ComponentPtr : g_IComponentVector)
	{
		IComponent* Component = *ComponentPtr.second;
		Component->DisableFunction();
	}

	return 0;
}