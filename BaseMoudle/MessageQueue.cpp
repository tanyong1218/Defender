#include "MessageQueue.h"

CWLMetaDataQueue::CWLMetaDataQueue(void)
{
	m_nTotalSize = 0;
	m_nSize = 0;
}

CWLMetaDataQueue::~CWLMetaDataQueue(void)
{
}

void CWLMetaDataQueue::Insert(IPC_MSG_DATA* pMetadata)
{
	{
		std::unique_lock<std::mutex> lock(m_QueueMutex);
		if (pMetadata)
		{
			m_Queue.push(pMetadata);
			m_nTotalSize ++;
		}
	}
}

IPC_MSG_DATA* CWLMetaDataQueue::GetHead()
{
	IPC_MSG_DATA* pMetadata = nullptr;
	{
		std::unique_lock<std::mutex> lock(m_QueueMutex);
		pMetadata = m_Queue.front();
		if (pMetadata)
		{
			m_nTotalSize --;
		}
		m_Queue.pop();
	}

	return pMetadata;
}

unsigned int CWLMetaDataQueue::GetCount()
{
	unsigned int nCount = 0;
	{
		std::unique_lock<std::mutex> lock(m_QueueMutex);
		nCount = (unsigned int)m_Queue.size();
	}

	return nCount;
}

unsigned int CWLMetaDataQueue::GetTotalSize()
{
	unsigned int nTotalSize = 0;
	{
		std::unique_lock<std::mutex> lock(m_QueueMutex);
		nTotalSize = m_nTotalSize;
	}

	return nTotalSize;
}

void CWLMetaDataQueue::RemoveAll()
{
	std::unique_lock<std::mutex> lock(m_QueueMutex);
	while (!m_Queue.empty())
	{
		IPC_MSG_DATA* pData = m_Queue.front();
		if (pData)
		{
			delete pData;
		}
		m_Queue.pop();
	}
}

