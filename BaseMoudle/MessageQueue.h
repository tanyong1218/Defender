#pragma once
#include "CommonHeader.h"
#include "SingletonClass.h"
#include "IPCMmf.h"

#include <queue>

enum DATA_TYPE { NORMAL_LOG = 1, VALIOTION_LOG, ALARM_LOG, INFO, MESSAGE };

class CWLMetaDataQueue
{
public:
	CWLMetaDataQueue(void);
	virtual ~CWLMetaDataQueue(void);

	void Insert(IPC_MSG_DATA* pMetadata);
	IPC_MSG_DATA* GetHead();
	unsigned int GetCount();
	unsigned int GetTotalSize();
	void RemoveAll();
protected:
	std::queue<IPC_MSG_DATA* > m_Queue;
	unsigned int m_nTotalSize;
	unsigned int m_nSize;
	std::mutex m_QueueMutex;
};
