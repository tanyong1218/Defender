#pragma once
#include "CommonHeader.h"
#define MAX_MMF_BUFFER_SIZE			(256*1024)	// 256K
#define DEFAULT_MMF_BUFFER_SIZE		(64*1024)	// 64K

#define IPC_CFG_MMF_NAME_SERVER					_T("Global-17264ded-16e3-41b4-bc13-64b5444c2129")
#define	WL_SERVICE_SINGTON_EVENT_NAME			_T("Global-CCD8AB05-FE62-4100-BDAA-1DAAC3273377")
#define	WL_MAIN_SINGTON_EVENT_NAME				_T("Global-CCD8AB05-FE62-4100-BDAA-2BDAGASH8807")
#define IPC_CFG_MUTEX_NAME_SERVER				_T("Global-bdd9fe5a-0800-4071-b559-a7a55b75964b")

#define IPC_CFG_DATA_HEADNER_LEN	2*sizeof(DWORD)
typedef struct _IPC_CFG_DATA
{
	DWORD dwMsgCode;
	DWORD dwEventType;
}IPC_CFG_DATA, * PIPC_CFG_DATA;

#define IPC_MSG_DATA_HEADNER_LEN	3*sizeof(DWORD)
typedef struct _IPC_MSG_DATA
{
	DWORD dwMsgCode;
	DWORD dwEventType;
	DWORD dwSize;			// The size of Data member, in bytes.
	BYTE Data[1];			// The data buffer.
}IPC_MSG_DATA, * PIPC_MSG_DATA;

enum IPC_ERROE_CODE
{
	MAPVIEWOFFILE_ERROR,
	WRITEDATA_OUTRANGE_ERROR,
	CATCH_ERROR,
};
class CWLIPCMmf
{
public:
	CWLIPCMmf(LPCWSTR lpMmfName, LPCWSTR lpMutexName, PSECURITY_ATTRIBUTES lpSa = NULL, DWORD dwMmfSize = MAX_MMF_BUFFER_SIZE);
	virtual ~CWLIPCMmf(void);

	BOOL WriteData(DWORD dwDataSize, const BYTE* pData);
	BOOL WriteDataInternal(DWORD dwDataSize, const BYTE* pData);
	BOOL ReadDataInternal(DWORD& dwDataSize, BYTE*& pDataBuffer);
	BOOL ReadData(DWORD& dwDataSize, BYTE*& pDataBuffer);

	DWORD m_dwErrorCode;
private:
	HANDLE m_hMFF;
	PVOID m_pMapView;
	DWORD m_dwMmfSize;
	HANDLE m_hMutex;
};
