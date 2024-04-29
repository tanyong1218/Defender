#pragma once
#include "Common.h"
#include "SingletonClass.h"
#include <MsXml2.h>
#include <winevt.h>
#include <msxml6.h>
#include <map>
#import"C:\Windows\System32\msxml6.dll"

#define MAX_ITEMVALUE_SIZE (100)
#define MAX_DESCRIPTION_SIZE 1024 * 6

//WINXP����ϵͳ��ע�����ϵͳ�¼��ص�
typedef HANDLE(WINAPI* PEVTSUBSCRIBE) (EVT_HANDLE, HANDLE, LPCWSTR, LPCWSTR, EVT_HANDLE, PVOID, EVT_SUBSCRIBE_CALLBACK, DWORD);
//WINXP����ϵͳ����ȡ����ϵͳ�¼�
typedef HANDLE(WINAPI* PEVTRENDER) (EVT_HANDLE, EVT_HANDLE, DWORD, DWORD, PVOID, PDWORD, PDWORD);
//WINXP����ϵͳ��ע������ϵͳ�¼��ص�
typedef HANDLE(WINAPI* PEVTCLOSE) (EVT_HANDLE);
//WINXP����ϵͳ����ȡ���ڶ�ȡָ���ṩ����Ԫ���ݵľ��: ��ȡ�¼������߾��: EvtOpenPublisherMetadata
typedef HANDLE(WINAPI* PEVTOPENPUBLISHERMETADATA) (EVT_HANDLE, LPCWSTR, LPCWSTR, LCID, DWORD);
//WINXP����ϵͳ��������Ϣ�ַ����ĸ�ʽ�����ڵõ��¼�����������Ϣ: evtFormatMessage
typedef HANDLE(WINAPI* PEVTFORMATMESSAGE) (EVT_HANDLE, EVT_HANDLE, DWORD, DWORD, PEVT_VARIANT, DWORD, DWORD, LPWSTR, PDWORD);

class CSysLogFun;
typedef struct _EVT_CALLBACK_CONTEXT
{
	std::wstring wsChannelPath;
	CSysLogFun* pCSysLogFun;
}
EVT_CALLBACK_CONTEXT, * PEVT_CALLBACK_CONTEXT;

enum HOST_AD_SYSLOG_CLASS
{
	EVENT_CLASS_ALL = 0,
	EVENT_CLASS_Application,
	EVENT_CLASS_Security,
	EVENT_CLASS_System,
	EVENT_CLASS_Setup
};

//ϵͳ��־
typedef struct  __HOST_AD_SYSLOG_STRUCT
{
	__HOST_AD_SYSLOG_STRUCT()
	{
		dwEventClass = 0;
		dwEventID = 0;
		dwEventType = 0;
		memset(wsEventTime, 0, sizeof(wsEventTime));

		memset(wsEventSourceName, 0, sizeof(wsEventSourceName));
		memset(wsEventDescription, 0, sizeof(wsEventDescription));
		memset(wsEventComputerName, 0, sizeof(wsEventComputerName));
		dwEventRecordID = 0;
		dwLogCount = 0;
	};
#if 0
	__HOST_AD_SYSLOG_STRUCT(__HOST_AD_SYSLOG_STRUCT& t)
	{
		dwEventClass = t.dwEventClass;
		dwEventID = t.dwEventID;
		dwEventType = t.dwEventType;
		memcpy(wsEventTime, t.wsEventTime, sizeof(wsEventTime));

		memcpy(wsEventSourceName, t.wsEventSourceName, sizeof(wsEventSourceName));
		memcpy(wsEventDescription, t.wsEventDescription, sizeof(wsEventDescription));
		memcpy(wsEventComputerName, t.wsEventComputerName, sizeof(wsEventComputerName)); \
			dwEventRecordID = t.dwEventRecordID;
	};
#endif
	DWORD dwEventClass;		//��־��Ϣ�������  -- BASELINE_EVENT_LOG_CLASS,app,system,sec,setup
	DWORD dwEventType;		//�¼����� - BASELINE_EVENT_LOG_TYPE ,info,warning,error.
	DWORD dwEventID;		//�¼�ID
	TCHAR wsEventTime[20];	//ʱ��
	TCHAR wsEventSourceName[128];	//��Դ
	TCHAR wsEventDescription[MAX_DESCRIPTION_SIZE];	//���� -- ���ֵ�ǳ��� 1024̫����
	TCHAR wsEventComputerName[64];	//���������
	DWORD dwEventRecordID;			//�¼���¼ID
	DWORD dwLogCount;				//��־����
	DWORD dwEventTime;				//��־��¼ʱ��,�ڲ�ʹ�ã�ʹ��DWORD�洢
}HOST_AD_SYSLOG_STRUCT, * PHOST_AD_SYSLOG_STRUCT;

class CSysLogFun : public Singleton<CSysLogFun> {
public:
	friend Singleton;   //����Ϊ��Ԫ�࣬�Ա�Singleton����˽�й��캯����Ϊ�˷�ֹ�ⲿ�������
public:
	~CSysLogFun();
	void InitSysLogFun();
	BOOL GetSysLogByPsloglist(wstring wsStartDateTime, wstring wsEndDateTime, wstring wsLogClass);   //ͨ��PslogList�����л�ȡ
	BOOL GetSysLogByEvtSubscribe();																	 //ͨ�����ĵķ�ʽʵʱ��ȡ
	BOOL GetSysLogByReadEventLog();																	 //ͨ����ȡ�¼���־�ķ�ʽ��ȡ
	BOOL m_EvtSubscribeThreadExit;
	BOOL m_ReadSystemEventThreadExit;
public:
	PEVTSUBSCRIBE m_pEvtSubScript;							//����ϵͳ��־�¼�
	PEVTRENDER m_pEvtRender;								//���ڴ��¼�����м����¼���Ϣ
	PEVTCLOSE m_pEvtClose;									//�ر��¼����
	PEVTOPENPUBLISHERMETADATA m_pEvtOpenPublisherMetadata;	//��ȡ���ڶ�ȡָ���ṩ����Ԫ���ݵľ��: ��ȡ�¼������߾��
	PEVTFORMATMESSAGE m_pEvtFormatMessage;					//������Ϣ�ַ����ĸ�ʽ�����ڵõ��¼�����������Ϣ

	//���ĸ�ϵͳ��־�¼����
	EVT_HANDLE m_hEvtHandleApp;
	EVT_HANDLE m_hEvtHandleSec;
	EVT_HANDLE m_hEvtHandleSys;
	EVT_HANDLE m_hEvtHandleSetup;

	EVT_CALLBACK_CONTEXT m_stContextApp;
	EVT_CALLBACK_CONTEXT m_stContextSec;
	EVT_CALLBACK_CONTEXT m_stContextSys;
	EVT_CALLBACK_CONTEXT m_stContextSetup;
public:

	void RecycleSysLogResource();
	EVT_HANDLE RegisterEvtCallBack(IN PEVT_CALLBACK_CONTEXT pContext, IN EVT_SUBSCRIBE_CALLBACK pFuncCallBack);
	void GetEventDescription(HOST_AD_SYSLOG_STRUCT* pOneSysLog, EVT_HANDLE hEvtHandle);
	BOOL ThreadSysLogExportInternal();
	static unsigned int	WINAPI DoThreadSysLogExport(LPVOID lpParameter);
	static unsigned int	WINAPI DoSystemEventThread(LPVOID lpParameter);

private:
	CSysLogFun();
};
