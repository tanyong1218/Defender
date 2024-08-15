#include "pch.h"
#include "MessageSender.h"

CWLMessageSender::CWLMessageSender(void)
{
	m_msgContainer = new CWLIPCMmf(IPC_CFG_MMF_NAME_SERVER, IPC_CFG_MUTEX_NAME_SERVER, NULL, DEFAULT_MMF_BUFFER_SIZE);
}

CWLMessageSender::~CWLMessageSender(void)
{
	if (m_msgContainer)
	{
		delete m_msgContainer;
		m_msgContainer = nullptr;
	}
}
DWORD CWLMessageSender::SendMsgToMmf(DWORD dwEventType, DWORD dwMsgCode, DWORD dwDataSize, BYTE* lpEventData)
{
	DWORD dwResult = ERROR_SUCCESS;
	BYTE* pMsgBuffer = nullptr;
	
	try
	{
		const DWORD dwMsgSize = IPC_MSG_DATA_HEADNER_LEN + dwDataSize;
		pMsgBuffer = new BYTE[dwMsgSize];
		memset(pMsgBuffer, 0, dwMsgSize);
		if (!pMsgBuffer)
		{
			dwResult =  ERROR_INVALID_DATA;
			goto END;
		}

		memset(pMsgBuffer, 0, dwMsgSize);

		IPC_MSG_DATA* pMsgData = (IPC_MSG_DATA*)pMsgBuffer;
		pMsgData->dwMsgCode = dwMsgCode;
		pMsgData->dwEventType = dwEventType;
		pMsgData->dwSize = dwDataSize;
		if (dwDataSize > 0)
		{
			if (!lpEventData)
			{
				dwResult = ERROR_INVALID_DATA;
				goto END;
			}
			else
			{
				memcpy(pMsgData->Data, lpEventData, dwDataSize);
			}
		}

		dwResult = m_msgContainer->WriteData(dwMsgSize, pMsgBuffer);
		if (ERROR_SUCCESS != dwResult)
		{
			dwResult = ERROR_WRITE_FAULT;
			goto END;
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
		dwResult = ERROR_INVALID_FUNCTION;
	}

END:

	return dwResult;
}
