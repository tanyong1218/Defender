#include "MessageSender.h"

CWLMessageSender::CWLMessageSender(void)
{
}

CWLMessageSender::~CWLMessageSender(void)
{
}

DWORD CWLMessageSender::SendMsg(DWORD dwEventType, DWORD dwMsgCode, DWORD dwDataSize, BYTE* lpEventData)
{
	DWORD dwResult = ERROR_SUCCESS;
	BYTE* pMsgBuffer = nullptr;

	try
	{
		CWLIPCMmf* msgContainer = new CWLIPCMmf(IPC_CFG_MMF_NAME_SERVER, IPC_CFG_MUTEX_NAME_SERVER, NULL, DEFAULT_MMF_BUFFER_SIZE);
		if (dwResult != ERROR_SUCCESS)
		{
			return dwResult;
		}

		const DWORD dwMsgSize = IPC_MSG_DATA_HEADNER_LEN + dwDataSize;
		pMsgBuffer = new BYTE[dwMsgSize];
		if (!pMsgBuffer) return 0xffffffef;

		memset(pMsgBuffer, 0, dwMsgSize);

		IPC_MSG_DATA* pMsgData = (IPC_MSG_DATA*)pMsgBuffer;
		pMsgData->dwMsgCode = dwMsgCode;
		pMsgData->dwEventType = dwEventType;
		pMsgData->dwSize = dwDataSize;
		if (dwDataSize > 0)
		{
			memcpy(pMsgData->Data, lpEventData, dwDataSize);
		}

		dwResult = msgContainer->WriteData(dwMsgSize, pMsgBuffer);
		if (ERROR_SUCCESS != dwResult)
		{
			dwResult = 0xfffffffe;
		}

		if (pMsgBuffer)
		{
			delete[] pMsgBuffer;
			pMsgBuffer = 0;
		}
	}
	catch (...)
	{
		if (pMsgBuffer)
		{
			delete[] pMsgBuffer;
			pMsgBuffer = 0;
		}
		dwResult = 0xffffffee;
	}

	return dwResult;
}