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

//WINXP以上系统，注册操作系统事件回调
typedef HANDLE(WINAPI* PEVTSUBSCRIBE) (EVT_HANDLE, HANDLE, LPCWSTR, LPCWSTR, EVT_HANDLE, PVOID, EVT_SUBSCRIBE_CALLBACK, DWORD);
//WINXP以上系统，读取操作系统事件
typedef HANDLE(WINAPI* PEVTRENDER) (EVT_HANDLE, EVT_HANDLE, DWORD, DWORD, PVOID, PDWORD, PDWORD);
//WINXP以上系统，注销操作系统事件回调
typedef HANDLE(WINAPI* PEVTCLOSE) (EVT_HANDLE);
//WINXP以上系统，获取用于读取指定提供程序元数据的句柄: 获取事件发布者句柄: EvtOpenPublisherMetadata
typedef HANDLE(WINAPI* PEVTOPENPUBLISHERMETADATA) (EVT_HANDLE, LPCWSTR, LPCWSTR, LCID, DWORD);
//WINXP以上系统，设置消息字符串的格式：用于得到事件中文描述信息: evtFormatMessage
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

//系统日志
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
	DWORD dwEventClass;		//日志信息包括类别  -- BASELINE_EVENT_LOG_CLASS,app,system,sec,setup
	DWORD dwEventType;		//事件类型 - BASELINE_EVENT_LOG_TYPE ,info,warning,error.
	DWORD dwEventID;		//事件ID
	TCHAR wsEventTime[20];	//时间
	TCHAR wsEventSourceName[128];	//来源
	TCHAR wsEventDescription[MAX_DESCRIPTION_SIZE];	//描述 -- 这个值非常大 1024太大了
	TCHAR wsEventComputerName[64];	//计算机名称
	DWORD dwEventRecordID;			//事件记录ID
	DWORD dwLogCount;				//日志数量
	DWORD dwEventTime;				//日志记录时间,内部使用，使用DWORD存储
}HOST_AD_SYSLOG_STRUCT, * PHOST_AD_SYSLOG_STRUCT;

class CSysLogFun : public Singleton<CSysLogFun> {
public:
	friend Singleton;   //声明为友元类，以便Singleton访问私有构造函数，为了防止外部构造对象
public:
	~CSysLogFun();
	void InitSysLogFun();
	BOOL GetSysLogByPsloglist(wstring wsStartDateTime, wstring wsEndDateTime, wstring wsLogClass);   //通过PslogList命令行获取
	BOOL GetSysLogByEvtSubscribe();																	 //通过订阅的方式实时获取
	BOOL GetSysLogByReadEventLog();																	 //通过读取事件日志的方式获取
	BOOL m_EvtSubscribeThreadExit;
	BOOL m_ReadSystemEventThreadExit;
public:
	PEVTSUBSCRIBE m_pEvtSubScript;							//订阅系统日志事件
	PEVTRENDER m_pEvtRender;								//用于从事件句柄中检索事件信息
	PEVTCLOSE m_pEvtClose;									//关闭事件句柄
	PEVTOPENPUBLISHERMETADATA m_pEvtOpenPublisherMetadata;	//获取用于读取指定提供程序元数据的句柄: 获取事件发布者句柄
	PEVTFORMATMESSAGE m_pEvtFormatMessage;					//设置消息字符串的格式：用于得到事件中文描述信息

	//订阅各系统日志事件句柄
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
