#include "IPCMmf.h"

CWLIPCMmf::CWLIPCMmf(LPCWSTR lpMmfName, LPCWSTR lpMutexName, PSECURITY_ATTRIBUTES lpSa, DWORD dwMmfSize)
{
	m_hMutex = CreateMutex(lpSa, FALSE, lpMutexName);
	m_hMFF = CreateFileMapping(INVALID_HANDLE_VALUE, lpSa, PAGE_READWRITE, 0, sizeof(dwMmfSize) + dwMmfSize, lpMmfName);
	if (!m_hMFF || !m_hMutex)
	{
		m_dwErrorCode = ::GetLastError();
	}

	if (NULL != m_hMFF && NULL == m_pMapView)
	{
		m_pMapView = ::MapViewOfFile(m_hMFF, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (!m_pMapView)
		{
			m_dwErrorCode = ::GetLastError();
		}
	}

	if (m_hMFF && m_pMapView)
	{
		m_dwMmfSize = dwMmfSize;
	}
}

CWLIPCMmf::~CWLIPCMmf(void)
{
	try
	{
		if (m_pMapView)
		{
			::UnmapViewOfFile(m_pMapView);
			m_pMapView = NULL;
		}

		if (m_hMFF)
		{
			::CloseHandle(m_hMFF);
			m_hMFF = NULL;
		}

		if (m_hMutex)
		{
			::CloseHandle(m_hMutex);
			m_hMutex = NULL;
		}
	}
	catch (...)
	{
	}
}

BOOL CWLIPCMmf::WriteData(DWORD dwDataSize, const BYTE* pData)
{
	WaitForSingleObject(m_hMutex, INFINITE);
	WriteDataInternal(dwDataSize, pData);
	ReleaseMutex(m_hMutex);
	return TRUE;
}
BOOL CWLIPCMmf::WriteDataInternal(DWORD dwDataSize, const BYTE* pData)
{
	if (!m_pMapView)
	{
		m_dwErrorCode = MAPVIEWOFFILE_ERROR;
		return FALSE;
	}

	DWORD* pCurDataSize = (DWORD*)(BYTE*)m_pMapView;	//*pCurDataSize = 当前共享内存中还未被读取的字节数
	BYTE* pCurData = (BYTE*)m_pMapView + sizeof(DWORD);

	if (*pCurDataSize + dwDataSize > m_dwMmfSize)
	{
		m_dwErrorCode = WRITEDATA_OUTRANGE_ERROR;
		return FALSE;
	}

	//如果*pCurDataSize为0的话，他就会覆盖之前写入的内存。
	memcpy(pCurData + *pCurDataSize, pData, dwDataSize);
	*pCurDataSize += dwDataSize;

	return TRUE;
}
BOOL CWLIPCMmf::ReadDataInternal(DWORD& dwDataSize, BYTE*& pDataBuffer)
{
	if (!m_pMapView)
	{
		m_dwErrorCode = MAPVIEWOFFILE_ERROR;
		return FALSE;
	}

	pDataBuffer = NULL;
	DWORD* pCurDataSize = (DWORD*)(BYTE*)m_pMapView;
	BYTE* pCurData = (BYTE*)m_pMapView + sizeof(DWORD);

	if (0 == *pCurDataSize)
	{
		m_dwErrorCode = WRITEDATA_OUTRANGE_ERROR;
		return FALSE;
	}

	try
	{
		pDataBuffer = new BYTE[*pCurDataSize + 1];
		if (pDataBuffer)
		{
			memcpy(pDataBuffer, pCurData, *pCurDataSize);
			dwDataSize = *pCurDataSize;
			*pCurDataSize = 0;
			return TRUE;
		}
	}
	catch (...)
	{
		m_dwErrorCode = CATCH_ERROR;
		if (pDataBuffer)
		{
			delete pDataBuffer;
			pDataBuffer = 0;
		}
	}
	return FALSE;
}
BOOL CWLIPCMmf::ReadData(DWORD& dwDataSize, BYTE*& pDataBuffer)
{
	WaitForSingleObject(m_hMutex, INFINITE);
	ReadDataInternal(dwDataSize, pDataBuffer);
	ReleaseMutex(m_hMutex);
	return 0;
}