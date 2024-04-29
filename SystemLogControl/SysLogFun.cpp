#include "SysLogFun.h"

std::map<std::string, HMODULE> g_MapDllHModule;

CSysLogFun::~CSysLogFun()
{
	RecycleSysLogResource();
}

CSysLogFun::CSysLogFun()
{
	InitSysLogFun();
}

void CSysLogFun::InitSysLogFun()
{
	m_stContextApp.wsChannelPath = L"Application";
	m_stContextApp.pCSysLogFun = this;

	m_stContextSec.wsChannelPath = L"Security";
	m_stContextSec.pCSysLogFun = this;

	m_stContextSys.wsChannelPath = L"System";
	m_stContextSys.pCSysLogFun = this;

	m_stContextSetup.wsChannelPath = L"Setup";
	m_stContextSetup.pCSysLogFun = this;

	m_EvtSubscribeThreadExit = FALSE;
	m_ReadSystemEventThreadExit = FALSE;
}

//---------------------GetSysLogByPsloglist-----------------------------
BOOL CSysLogFun::GetSysLogByPsloglist(wstring wsStartDateTime, wstring wsEndDateTime, wstring wsLogClass)
{
	//E:\\psloglist.exe  -a 02/28/2015 -b 04/01/2016 Setup -s
	//E:\\psloglist.exe  -a 02/28/2015 -b 04/01/2016 Application  -s
	WriteInfo(("GetSysLogByPsloglist Begin"));
	wstring wsFilePath = CWindowsHelper::GetRunDir();

	wchar_t FileName[MAX_PATH] = { 0 };
	swprintf_s(FileName, _T("%s/%s.txt"), wsFilePath.c_str(), wsLogClass.c_str());
	wsFilePath = FileName;	// 文件路径   ../wsLogClass.txt

	wstring wsCmdLine = CWindowsHelper::GetSystemDir() + _T("\\psloglist.exe ");
	if (wsStartDateTime.length() > 0)
	{
		wsCmdLine += _T(" -accepteula -a ");
		wsCmdLine += wsStartDateTime;
	}

	if (wsEndDateTime.length() > 0)
	{
		wsCmdLine += _T(" -b ");
		wsCmdLine += wsEndDateTime;
	}

	wsCmdLine += _T(" ");
	wsCmdLine += wsLogClass;
	wsCmdLine += _T(" -s");

	wsCmdLine += _T(" -n 3000");

	SECURITY_ATTRIBUTES sa = { sizeof(sa),NULL,TRUE };
	SECURITY_ATTRIBUTES* psa = NULL;
	DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	psa = &sa;
	dwShareMode |= FILE_SHARE_DELETE;

	//根据版本设置共享模式和安全属性
	HANDLE hConsoleRedirect = CreateFile(
		wsFilePath.c_str(),
		GENERIC_ALL,
		dwShareMode,
		psa,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hConsoleRedirect == INVALID_HANDLE_VALUE)
	{
		WriteError(("syslog-Audit, hConsoleRedirect==INVALID_HANDLE_VALUE"));
		return FALSE;
	}
	STARTUPINFO s = { sizeof(s) };
	s.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	//使用标准柄和显示窗口
	s.hStdOutput = hConsoleRedirect;//将文件作为标准输出句柄,还可以是GetStdHandle(STD_OUTPUT_HANDLE)当前控制台，匿名管道等
	s.wShowWindow = SW_HIDE;//隐藏控制台窗口
	PROCESS_INFORMATION pi = { 0 };

	static wstring wsWorkDir = CWindowsHelper::GetSystemDir();
	if (CreateProcess(NULL, (LPTSTR)wsCmdLine.c_str(), NULL, NULL, TRUE, NULL, NULL, wsWorkDir.c_str(), &s, &pi))
	{
		//创建进程,执行Ping程序,测试网络是否连通
		WaitForSingleObject(pi.hProcess, INFINITE);

		//等待进程执行完毕
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		CloseHandle(hConsoleRedirect);
		WriteInfo(("syslog-Audit, export syslog completed"));
		return TRUE;
	}
	else
	{
		WriteInfo(("syslog-Audit, CreateProcess Failed, wsCmdLine = {}  begin"), CStrUtil::UnicodeToUTF8(wsCmdLine));
		return FALSE;
	}
}

//---------------------GetSysLogByEvtSubscribe-----------------------------
BOOL CSysLogFun::GetSysLogByEvtSubscribe()
{
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, DoSystemEventThread, this, 0, NULL);
	return TRUE;
}
unsigned int WINAPI CSysLogFun::DoThreadSysLogExport(LPVOID lpParameter)
{
	CSysLogFun* pSysLogFun = (CSysLogFun*)lpParameter;
	//采用此方法是为了防止XP系统缺少一些静态库，导致启动不了该模块
	HMODULE hWevtapiModule = NULL;
	hWevtapiModule = ::LoadLibrary(L"wevtapi.dll");
	if (NULL == hWevtapiModule)
	{
		WriteInfo(("LoadLibrary wevtapi.dll Failed, ErrorNum = {}"), GetLastError());
	}
	else
	{
		pSysLogFun->m_pEvtSubScript = (PEVTSUBSCRIBE)GetProcAddress(hWevtapiModule, "EvtSubscribe");
		pSysLogFun->m_pEvtRender = (PEVTRENDER)GetProcAddress(hWevtapiModule, "EvtRender");
		pSysLogFun->m_pEvtClose = (PEVTCLOSE)GetProcAddress(hWevtapiModule, "EvtClose");
		pSysLogFun->m_pEvtOpenPublisherMetadata = (PEVTOPENPUBLISHERMETADATA)GetProcAddress(hWevtapiModule, "EvtOpenPublisherMetadata");
		pSysLogFun->m_pEvtFormatMessage = (PEVTFORMATMESSAGE)GetProcAddress(hWevtapiModule, "EvtFormatMessage");
	}

	//winXP以上
	if (NULL != pSysLogFun->m_pEvtSubScript && NULL != pSysLogFun->m_pEvtRender && NULL != pSysLogFun->m_pEvtOpenPublisherMetadata && NULL != pSysLogFun->m_pEvtFormatMessage)
	{
		pSysLogFun->ThreadSysLogExportInternal();
	}

	while (!pSysLogFun->m_EvtSubscribeThreadExit)
	{
		Sleep(5 * 1000);
		continue;
	}

	if (NULL != hWevtapiModule)
	{
		::FreeLibrary(hWevtapiModule);
		hWevtapiModule = NULL;
	}

	WriteInfo(("EvtSubscribeThread End"));

	_endthreadex(0);
	return TRUE;
}
void CSysLogFun::RecycleSysLogResource()
{
	m_EvtSubscribeThreadExit = TRUE;
	m_ReadSystemEventThreadExit = TRUE;
}
EVT_HANDLE CSysLogFun::RegisterEvtCallBack(IN PEVT_CALLBACK_CONTEXT pContext, IN EVT_SUBSCRIBE_CALLBACK pFuncCallBack)
{
	EVT_HANDLE hEvtHandle = NULL;

	if (NULL == pContext || NULL == pFuncCallBack)
	{
		return NULL;
	}

	/*
	NULL作为第一个参数，表示没有指定事件订阅会话，新的会话将被创建。
	NULL作为第二个参数，表示没有使用信号事件来通知订阅者，因此无法实现异步通知功能。订阅将是同步的，即需要主动使用EvtNext函数来检索事件。
	pContext->wsChannelPath.c_str()作为第三个参数，指定要订阅的事件日志通道路径。pContext->wsChannelPath是一个字符串，表示要订阅的事件通道的路径。
	NULL作为第四个参数，表示没有指定查询表达式，将订阅所有事件。
	NULL作为第五个参数，表示从当前时间点开始订阅事件，不指定起始的书签位置。
	(PVOID)pContext作为第六个参数，传递了一个用户自定义的上下文数据指针，该指针将在回调函数中使用。
	pFuncCallBack作为第七个参数，为订阅的回调函数，用于处理订阅到的事件。
	EvtSubscribeToFutureEvents作为第八个参数，指定订阅行为为订阅未来发生的事件。*/
	hEvtHandle = m_pEvtSubScript(NULL, NULL, pContext->wsChannelPath.c_str(), NULL, NULL, (PVOID)pContext, pFuncCallBack, EvtSubscribeToFutureEvents);//仅接收之后的事件
	if (NULL == hEvtHandle)
	{
		WriteError(("EvtSubscribe Failed, Error = {}"), GetLastError());
	}

	return hEvtHandle;
}
void GetEvtXmlDataNodes(IXMLDOMNodeListPtr pNodeList, std::vector<IXMLDOMNodePtr>& veXmlNodes)
{
	long llength = 0;
	pNodeList->get_length(&llength);
	for (int i = 0; i < llength; i++)
	{
		IXMLDOMNodePtr pNode = NULL;
		pNodeList->get_item(i, &pNode);
		if (pNode)
		{
			veXmlNodes.push_back(pNode);
		}

		IXMLDOMNodeListPtr childNodeList = NULL;
		pNode->get_childNodes(&childNodeList);
		if (childNodeList)
		{
			GetEvtXmlDataNodes(childNodeList, veXmlNodes);
		}
	}
}
BOOL AnalyEvtXmlData(PWCHAR pEvtXmlData, PEVT_CALLBACK_CONTEXT pEvtCBKContext, HOST_AD_SYSLOG_STRUCT* pOneSyslog)
{
	HRESULT hResult = S_OK;
	VARIANT_BOOL bLoadXML = VARIANT_TRUE;
	BSTR pNodeName = NULL;
	BSTR pNodeValue = NULL;
	BSTR pItemName = NULL;
	BSTR pItemValue = NULL;
	long lCount = 0;
	IXMLDOMNodeListPtr pNodeList = NULL;
	std::vector<IXMLDOMNodePtr> vecNode;
	int Index = 0;

	if (NULL == pEvtXmlData || NULL == pEvtCBKContext)
	{
		return FALSE;
	}

	MSXML2::IXMLDOMDocumentPtr pXmlDoc = NULL;
	hResult = pXmlDoc.CreateInstance(__uuidof(MSXML2::DOMDocument60));
	if (!SUCCEEDED(hResult) || NULL == pXmlDoc)
	{
		WriteError(("MsXml CreateInstance Failed, Error = {}"), GetLastError());
		return FALSE;
	}
	bLoadXML = pXmlDoc->loadXML(pEvtXmlData);
	if (VARIANT_TRUE != bLoadXML)
	{
		return FALSE;
	}

	vecNode.clear();

	pNodeList = pXmlDoc->selectNodes(L"*");

	GetEvtXmlDataNodes(pNodeList, vecNode);

	//遍历vecNode，提取出事件项
	for (int n = 0; n < vecNode.size(); n++)
	{
		IXMLDOMNamedNodeMapPtr pAttrMap = NULL;

		pNodeName = NULL;
		pNodeValue = NULL;

		//获取 节点名 和 text 值
		vecNode[n]->get_nodeName(&pNodeName);
		vecNode[n]->get_text(&pNodeValue);
		if (wcslen(pNodeName) > 0 && wcslen(pNodeValue) > 0
			&& wcslen(pNodeValue) < MAX_ITEMVALUE_SIZE
			&& 0 != wcscmp(pNodeName, _T("Data"))
			&& 0 != wcscmp(pNodeName, _T("#text"))
			&& 0 != wcscmp(pNodeName, _T("xmlns"))
			&& 0 != wcscmp(pNodeValue, _T("-")))
		{
			if (0 == wcscmp(pNodeName, _T("EventID")))
			{
				pOneSyslog->dwEventID = _wtoi(pNodeValue);
			}
			else if (0 == wcscmp(pNodeName, _T("Level")))//等级
			{
				pOneSyslog->dwEventType = _wtoi64(pNodeValue);
			}
			else if (0 == wcscmp(pNodeName, _T("EventRecordID")))//等级
			{
				pOneSyslog->dwEventRecordID = _wtoi64(pNodeValue);
			}
			else if (0 == wcscmp(pNodeName, _T("Channel")))//日志类型
			{
				if (0 == wcscmp(pNodeValue, _T("System")))
				{
					pOneSyslog->dwEventClass = EVENT_CLASS_System;
				}
				else if (0 == wcscmp(pNodeValue, _T("Security")))
				{
					pOneSyslog->dwEventClass = EVENT_CLASS_Security;
				}
				else if (0 == wcscmp(pNodeValue, _T("Application")))
				{
					pOneSyslog->dwEventClass = EVENT_CLASS_Application;
				}
			}
			else if (0 == wcscmp(pNodeName, _T("Computer")))
			{
				wstring wsValue = pNodeValue;
				_tcsncpy_s(pOneSyslog->wsEventComputerName, wsValue.c_str(), _countof(pOneSyslog->wsEventComputerName) - 1);
			}
		}

		/*
		<Provider Name='Microsoft-Windows-Security-Auditing' Guid='{54849625-5478-4994-a5ba-3e3b0328c30d}'/>
		Node为Provider，Name和Guid为AttrMap，Microsoft-Windows-Security-Auditing为Name的值
		<EventID>4673</EventID>
		Node为EventID，4673为text值
		*/
		hResult = vecNode[n]->get_attributes(&pAttrMap);
		if (!SUCCEEDED(hResult) || NULL == pAttrMap)
		{
			goto Continue;
		}

		hResult = pAttrMap->get_length(&lCount);
		if (!SUCCEEDED(hResult) || NULL == pAttrMap)
		{
			goto Continue;
		}

		//遍历pAttrMap，放入pAttrItem,目的为了获取Provider和TimeCreated的值
		for (int j = 0; j < lCount; j++)
		{
			IXMLDOMNodePtr pAttrItem = NULL;
			hResult = pAttrMap->get_item(j, &pAttrItem);
			if (!SUCCEEDED(hResult) || NULL == pAttrItem)
			{
				continue;
			}

			pItemName = NULL;
			pItemValue = NULL;

			// 得到pAttrItem的 节点名 和 text
			pAttrItem->get_nodeName(&pItemName);
			pAttrItem->get_text(&pItemValue);

			if (0 == wcscmp(pNodeName, _T("Provider")) && 0 == wcscmp(pItemName, _T("Name")))
			{
				_tcsncpy_s(pOneSyslog->wsEventSourceName, pItemValue, _countof(pOneSyslog->wsEventSourceName) - 1);
			}

			if (0 == wcscmp(pNodeName, _T("TimeCreated")) && 0 == wcscmp(pItemName, _T("SystemTime")))
			{
				_tcsncpy_s(pOneSyslog->wsEventTime, pItemValue, _countof(pOneSyslog->wsEventTime) - 1);
			}

			if (pItemName != NULL)
			{
				::SysFreeString(pItemName);
			}

			if (pItemValue != NULL)
			{
				::SysFreeString(pItemValue);
			}
		}
	Continue:
		if (pNodeName != NULL)
		{
			::SysFreeString(pNodeName);
		}

		if (pNodeValue != NULL)
		{
			::SysFreeString(pNodeValue);
		}
	}
	return TRUE;
}
void CSysLogFun::GetEventDescription(HOST_AD_SYSLOG_STRUCT* pOneSysLog, EVT_HANDLE hEvtHandle)
{
	//拿到发布者元数据句柄
	EVT_HANDLE hPublisherMetadata = NULL;
	hPublisherMetadata = m_pEvtOpenPublisherMetadata(NULL, pOneSysLog->wsEventSourceName, NULL, 0, 0);
	if (hPublisherMetadata == NULL)
	{
		WriteError(("hPublisherMetadata =NULL; {}"), GetLastError());
		return;
	}
	DWORD bufferSize = 0;
	LPWSTR pFormattedMessage = NULL;

	//翻译事件简述
	if (!m_pEvtFormatMessage(hPublisherMetadata, hEvtHandle, 0, 0, NULL, EvtFormatMessageEvent, bufferSize, NULL, &bufferSize))
	{
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			pFormattedMessage = new WCHAR[bufferSize + 1];
			if (!pFormattedMessage)
			{
				WriteError(("get EvtFormatMessage buffer error = {}"), GetLastError());
				goto CLEAN;
			}
			memset(pFormattedMessage, 0, (bufferSize + 1));

			if (m_pEvtFormatMessage(hPublisherMetadata, hEvtHandle, 0, 0, NULL, EvtFormatMessageEvent, bufferSize, pFormattedMessage, &bufferSize))
			{
				std::wstring wstrDescription = pFormattedMessage;
				std::replace(wstrDescription.begin(), wstrDescription.end(), '\r', ' ');
				std::replace(wstrDescription.begin(), wstrDescription.end(), '\n', ' ');
				std::replace(wstrDescription.begin(), wstrDescription.end(), '\t', ' ');
				_tcsncpy_s(pOneSysLog->wsEventDescription, wstrDescription.c_str(), _countof(pOneSysLog->wsEventDescription) - 1);
			}
			else
			{
				WriteError(("get EvtFormatMessagedata error = {}"), GetLastError());
			}
		}
		else
		{
			WriteError(("get EvtFormatMessage size error = {}"), GetLastError());
		}
	}

CLEAN:
	delete[] pFormattedMessage;
	if (hPublisherMetadata)
	{
		m_pEvtClose(hPublisherMetadata);
	}
}
// 订阅系统日志事件的回调函数
DWORD WINAPI EvtCallbackFunction(EVT_SUBSCRIBE_NOTIFY_ACTION EvtAction, PVOID pContext, EVT_HANDLE hEvtHandle)
{
	DWORD dwBufferSize = 0;
	DWORD dwBufferUsed = 0;
	DWORD dwPropertyCount = 0;
	PWCHAR pEvtData = NULL;
	BOOL bRet = FALSE;
	if (NULL == hEvtHandle || NULL == pContext || EvtSubscribeActionError == EvtAction)
	{
		return 0;
	}

	PEVT_CALLBACK_CONTEXT pEvtCBKContext = (PEVT_CALLBACK_CONTEXT)pContext;
	//得到此次事件的XML数据
	if (!pEvtCBKContext->pCSysLogFun->m_pEvtRender(NULL, hEvtHandle, EvtRenderEventXml, dwBufferSize, pEvtData, &dwBufferUsed, &dwPropertyCount))
	{
		if (ERROR_INSUFFICIENT_BUFFER != GetLastError() || 0 == dwBufferUsed)
		{
			goto END;
		}

		dwBufferSize = dwBufferUsed + 1;
		pEvtData = (PWCHAR)malloc(dwBufferSize);
		if (NULL == pEvtData)
		{
			WriteError(("malloc Failed, Size = {}"), dwBufferSize);
			goto END;
		}

		/*
		用于将事件句柄中的事件信息渲染为XML格式，并将其存储在指定的缓冲区中。下面对函数调用参数进行详细解释：
		第一个参数 NULL：表示未使用，它是一个保留参数，此处设为NULL即可。
		第二个参数 hEvtHandle：表示事件句柄，是一个EVT_HANDLE类型的变量，用于指定要渲染的事件。
		第三个参数 EvtRenderEventXml：表示渲染标志，指示将事件渲染为XML格式。
		第四个参数 dwBufferSize：表示缓冲区大小，用于指定接收渲染后的事件信息所需的缓冲区大小。
		第五个参数 pEvtData：表示缓冲区指针，用于接收渲染后的事件信息。
		第六个参数 &dwBufferUsed：表示输出参数，指向一个DWORD类型的变量，用于返回实际使用的缓冲区大小。
		第七个参数 &dwPropertyCount：表示输出参数，指向一个DWORD类型的变量，用于返回渲染后的事件拥有的属性数量。
		*/
		if (!pEvtCBKContext->pCSysLogFun->m_pEvtRender(NULL, hEvtHandle, EvtRenderEventXml, dwBufferSize, pEvtData, &dwBufferUsed, &dwPropertyCount))
		{
			goto END;
		}
		//解析xml数据
		CoInitialize(NULL);
		HOST_AD_SYSLOG_STRUCT oneSyslog;
		bRet = AnalyEvtXmlData(pEvtData, pEvtCBKContext, &oneSyslog);

		// 获取事件消息描述
		pEvtCBKContext->pCSysLogFun->GetEventDescription(&oneSyslog, hEvtHandle);
		wstring wsEventDescription = oneSyslog.wsEventDescription;
		wstring wsEventTime = oneSyslog.wsEventTime;
		wstring wsEventComputerName = oneSyslog.wsEventComputerName;
		wstring wsEventSourceName = oneSyslog.wsEventSourceName;
		WriteInfo(("SysLog EventID = {} EventComputerName = {} EventTime = {} EventSourceName = {}"),
			oneSyslog.dwEventID, CStrUtil::ConvertW2A(wsEventComputerName), CStrUtil::ConvertW2A(wsEventTime), CStrUtil::ConvertW2A(wsEventSourceName));
	}

END:
	if (pEvtData)
	{
		free(pEvtData);
		pEvtData = NULL;
	}

	return 0;
}
BOOL CSysLogFun::ThreadSysLogExportInternal()
{
	m_hEvtHandleApp = RegisterEvtCallBack(&(m_stContextApp), EvtCallbackFunction);
	if (NULL == m_hEvtHandleApp)
	{
		WriteInfo(("RegisterEvtCallBack Application Failed, Error = {}"), GetLastError());
	}

	m_hEvtHandleSec = RegisterEvtCallBack(&(m_stContextSec), EvtCallbackFunction);
	if (NULL == m_hEvtHandleSec)
	{
		WriteError(("RegisterEvtCallBack Security Failed, Error = {}"), GetLastError());
	}

	m_hEvtHandleSys = RegisterEvtCallBack(&(m_stContextSys), EvtCallbackFunction);
	if (NULL == m_hEvtHandleSys)
	{
		WriteError(("RegisterEvtCallBack System Failed, Error = {}"), GetLastError());
	}

	m_hEvtHandleSetup = RegisterEvtCallBack(&(m_stContextSetup), EvtCallbackFunction);
	if (NULL == m_hEvtHandleSetup)
	{
		WriteError(("RegisterEvtCallBack Setup Failed, Error = {}"), GetLastError());
	}

	if (NULL != m_pEvtClose)
	{
		if (NULL != m_hEvtHandleApp)
		{
			m_pEvtClose(m_hEvtHandleApp);
		}

		if (NULL != m_hEvtHandleSec)
		{
			m_pEvtClose(m_hEvtHandleSec);
		}

		if (NULL != m_hEvtHandleSys)
		{
			m_pEvtClose(m_hEvtHandleSys);
		}

		if (NULL != m_hEvtHandleSetup)
		{
			m_pEvtClose(m_hEvtHandleSetup);
		}
	}

	return TRUE;
}

//---------------------GetSysLogByReadEventLog-----------------------------
BOOL CSysLogFun::GetSysLogByReadEventLog()
{
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, DoSystemEventThread, this, 0, NULL);
	return 0;
}
BOOL GetEventDescriptionDllPath(PCHAR pLogName, PCHAR pLogSource, std::string& sDllPath)
{
	HKEY hkey = NULL;
	CHAR pRegKey[MAX_PATH] = { 0 };
	CHAR pDllPath[MAX_PATH] = { 0 };
	DWORD dwRet;
	BOOL bRet = FALSE;

	if (NULL == pLogName || NULL == pLogSource)
	{
		return FALSE;
	}

	sprintf_s(pRegKey, "System\\CurrentControlSet\\Services\\EventLog\\%s\\%s", pLogName, pLogSource);

	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, pRegKey, 0, KEY_ALL_ACCESS, &hkey) != ERROR_SUCCESS)
	{
		//WriteError((" RegOpenKeyExA %S Failed"), pRegKey);
		goto END;
	}

	dwRet = MAX_PATH - 1;
	if (RegQueryValueExA(hkey, "EventMessageFile", NULL, NULL, (LPBYTE)pDllPath, &dwRet) != ERROR_SUCCESS)
	{
		//WriteError((" RegQueryValueExA %S EventMessageFile Failed"), pRegKey);
		goto END;
	}

	sDllPath = "";
	sDllPath = pDllPath;

	if (0 == sDllPath.length())
	{
		//WriteError(("DllPath.c_str() is NULL Dll = %S"), sDllPath.c_str());
		goto END;
	}

	bRet = TRUE;

END:
	if (NULL != hkey)
	{
		RegCloseKey(hkey);
	}

	return bRet;
}
BOOL FormatEventDescription(DWORD dwEventID, PCHAR pLogName, PCHAR pLogSource, PCHAR* pFormatString, std::string& sDescription)
{
	DWORD dwFlags = 0;
	std::string sDllPath = "";

	CHAR pRealDllPath[MAX_PATH] = { 0 };

	PCHAR pCurStr = NULL;
	PCHAR pNextStr = NULL;

	PCHAR pFormatedData = NULL;

	HMODULE hModule = NULL;

	dwFlags = FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY;

	std::string strSource = pLogName;
	strSource += "\\";
	strSource += pLogSource;

	std::map<std::string, HMODULE>::iterator it = g_MapDllHModule.find(strSource);
	if (it != g_MapDllHModule.end())
	{
		hModule = it->second;
		if (NULL == hModule)
		{
			return FALSE;
		}
	}

	//已加载对应库，直接转换
	if (NULL != hModule)
	{
		if (!FormatMessageA(dwFlags, hModule, dwEventID, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPSTR)&pFormatedData, 0, (va_list*)pFormatString))
		{
			WriteError(("FormatMessageA Failed, hModule {}, Error = {}"), reinterpret_cast<uintptr_t>(hModule), GetLastError());
			pFormatedData = NULL;
		}

		if (NULL == pFormatedData || 0 == strlen(pFormatedData))
		{
			WriteError(("FormatMessageA Failed, Dll = {}, Error = {}"), strSource.c_str(), GetLastError());
			FreeLibrary(hModule);
			g_MapDllHModule.erase(it);
			goto RENEW;
		}
		else
		{
			sDescription = pFormatedData;
			LocalFree(pFormatedData);
			return TRUE;
		}
	}

	//没有加载对应库，尝试加载
RENEW:
	//取注册表项
	if (!GetEventDescriptionDllPath(pLogName, pLogSource, sDllPath))
	{
		g_MapDllHModule[strSource] = NULL;
		return FALSE;
	}

	pCurStr = (PCHAR)sDllPath.c_str();

	while (pNextStr = strchr(pCurStr, ';'))
	{
		*pNextStr = '\0';

		memset(pRealDllPath, 0, MAX_PATH);
		ExpandEnvironmentStringsA(pCurStr, pRealDllPath, MAX_PATH - 1);

		*pNextStr = ';';

		hModule = LoadLibraryExA(pRealDllPath, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
		if (hModule)
		{
			//格式化失败
			if (!FormatMessageA(dwFlags, hModule, dwEventID, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPSTR)&pFormatedData, 0, (va_list*)pFormatString))
			{
				WriteError(("FormatMessageA Failed, Dll = {}, Error = {}"), pRealDllPath, GetLastError());
				pFormatedData = NULL;
			}

			//格式化成功，保存当前句柄
			if (NULL != pFormatedData && strlen(pFormatedData) > 0)
			{
				sDescription = pFormatedData;
				LocalFree(pFormatedData);
				g_MapDllHModule[strSource] = hModule;
				WriteInfo(("found dll {}, hModule {} "), pRealDllPath, reinterpret_cast<uintptr_t>(hModule));
				return TRUE;
			}
			else
			{
				FreeLibrary(hModule);
			}
		}
		else
		{
			WriteError(("LoadLibraryExA Dll = {}, Error = {}"), pRealDllPath, GetLastError());
		}

		pCurStr = pNextStr + 1;
	}

	//循环完成后最后还有一个路径
	memset(pRealDllPath, 0, MAX_PATH);
	ExpandEnvironmentStringsA(pCurStr, pRealDllPath, MAX_PATH - 1);
	hModule = LoadLibraryExA(pRealDllPath, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
	if (hModule)
	{
		if (!FormatMessageA(dwFlags, hModule, dwEventID, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPSTR)&pFormatedData, 0, (va_list*)pFormatString))
		{
			//格式化失败
			DWORD dwErr = GetLastError();
			WriteError(("FormatMessageA Failed, Dll = {}, Error = {}"), pRealDllPath, GetLastError());
			pFormatedData = NULL;
		}

		//格式化成功，保存当前句柄
		if (NULL != pFormatedData && strlen(pFormatedData) > 0)
		{
			sDescription = pFormatedData;
			LocalFree(pFormatedData);
			g_MapDllHModule[strSource] = hModule;
			return TRUE;
		}
		else
		{
			WriteError(("FormatMessageA Failed, Dll = {}, Error = {}"), pRealDllPath, GetLastError());
			FreeLibrary(hModule);
		}
	}
	else
	{
		WriteError(("LoadLibraryExA Failed, Dll = {}, Error = {}"), pRealDllPath, GetLastError());
	}
	return FALSE;
}
BOOL GetEventDescription(PCHAR pEventString, DWORD dwNumStrings, PCHAR pLogName, DWORD dwEventID, PCHAR pLogSource, std::string& sDescription)
{
	BOOL bRet = FALSE;
	PCHAR pFormatString = NULL;
	PCHAR pTempString = NULL;
	int iLeftSize = MAX_DESCRIPTION_SIZE;
	int iStringSize = 0;

	PCHAR pStringArray[MAX_PATH] = { 0 };

	if (NULL == pEventString || 0 == dwNumStrings || NULL == pLogName || NULL == pLogSource)
	{
		bRet = FALSE;
		goto END;
	}

	pFormatString = (PCHAR)malloc(MAX_DESCRIPTION_SIZE + 1);
	if (NULL == pFormatString)
	{
		bRet = FALSE;
		goto END;
	}
	memset(pFormatString, 0, MAX_DESCRIPTION_SIZE + 1);

	for (int i = 0; i < dwNumStrings; i++) {
		iStringSize = strlen(pEventString);

		if (iStringSize >= MAX_PATH - 1)
		{
			bRet = FALSE;
			goto END;
		}

		if (iLeftSize > iStringSize) {
			strncat_s(pFormatString, MAX_DESCRIPTION_SIZE + 1, pEventString, iLeftSize);
		}

		pTempString = strchr(pFormatString, '\0');
		if (pTempString) {
			*pTempString = ' ';
			pTempString++;
			*pTempString = '\0';
		}

		iLeftSize -= iStringSize + 2;

		if (i <= 92)
		{
			pStringArray[i] = pEventString;
			pStringArray[i + 1] = NULL;
		}

		pEventString = strchr(pEventString, '\0');
		if (pEventString)
		{
			pEventString++;
		}
		else
		{
			break;
		}
	}

	if (!FormatEventDescription(dwEventID, pLogName, pLogSource, pStringArray, sDescription))
	{
		bRet = FALSE;
		goto END;
	}

	bRet = TRUE;
END:
	if (pFormatString)
	{
		free(pFormatString);
	}

	return bRet;
}
BOOL AnalyEventData(PCHAR pEvtData, DWORD dwDataSize, PCHAR pChannel, HOST_AD_SYSLOG_STRUCT* pOneSysLog)
{
	PCHAR pLogSource = nullptr;
	PCHAR pLogComputer = nullptr;
	std::string strDescription;
	BOOL bRet = FALSE;
	if (NULL == pEvtData || 0 == dwDataSize || NULL == pChannel || NULL == pOneSysLog)
	{
		return FALSE;
	}

	EVENTLOGRECORD* pEvtRecord = (EVENTLOGRECORD*)pEvtData;
	pOneSysLog->dwEventType = pEvtRecord->EventType;

	pOneSysLog->dwEventID = pEvtRecord->EventID;
	pOneSysLog->dwEventRecordID = pEvtRecord->RecordNumber;

	time_t time = pEvtRecord->TimeGenerated;
	struct tm tm_time;
	memset(&tm_time, 0, sizeof(tm_time));
	localtime_s(&tm_time, &time);
	swprintf_s(pOneSysLog->wsEventTime, _T("%4d-%02d-%02d %02d:%02d:%02d"), tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);

	if (0 == strcmp(pChannel, "System"))
	{
		pOneSysLog->dwEventClass = EVENT_CLASS_System;
	}
	else if (0 == strcmp(pChannel, "Security"))
	{
		pOneSysLog->dwEventClass = EVENT_CLASS_Security;
	}
	else if (0 == strcmp(pChannel, "Application"))
	{
		pOneSysLog->dwEventClass = EVENT_CLASS_Application;
	}
	else if (0 == strcmp(pChannel, "Setup"))
	{
		pOneSysLog->dwEventClass = EVENT_CLASS_Setup;
	}
	else
	{
		pOneSysLog->dwEventClass = EVENT_CLASS_ALL;
	}

	pOneSysLog->dwEventTime = pEvtRecord->TimeGenerated;

	pLogSource = (PCHAR)((LPBYTE)pEvtData + sizeof(EVENTLOGRECORD));
	wstring wstrLogSource = CStrUtil::ConvertA2W(string(pLogSource));

	_tcscpy_s(pOneSysLog->wsEventSourceName, wstrLogSource.c_str());

	pLogComputer = pLogSource + strlen(pLogSource) + 1;
	wstring wstrLogComputer = CStrUtil::ConvertA2W(string(pLogComputer));
	_tcscpy_s(pOneSysLog->wsEventComputerName, wstrLogComputer.c_str());

	if (!GetEventDescription((PCHAR)pEvtData + pEvtRecord->StringOffset, pEvtRecord->NumStrings, pChannel, pEvtRecord->EventID, pLogSource, strDescription))
	{
		char pBuf[512] = { 0 };
		PCHAR p1 = NULL;
		if (pEvtRecord->NumStrings)
		{
			p1 = (PCHAR)pEvtData + pEvtRecord->StringOffset;
			for (int i = 0; i < pEvtRecord->NumStrings; i++)
			{
				if (strlen(pBuf) + strlen(p1) + 2 < 512)
				{
					strcat_s(pBuf, 512, p1);
					strcat_s(pBuf, 512, ";");
				}
				else
				{
					break;
				}

				p1 += strlen(p1);
				p1++;
			}
		}

		char tmpFormatSring[1000] = { 0 };

		std::wstring wsContent = _T("无法找到来自源 System 的事件 ID %d 的描述。本地计算机上未安装引发此事件的组件，或者安装已损坏。可以安装或修复本地计算机上的组件。\
			如果该事件产生于另一台计算机，则必须在该事件中保存显示信息。以下是包含在事件中的信息: %s 消息资源存在，但在消息表中找不到该消息。");
		std::string strContent = CStrUtil::ConvertW2A(wsContent);
		sprintf_s(tmpFormatSring, strContent.c_str(), pEvtRecord->EventID, pBuf);
		strDescription = tmpFormatSring;
	}
	if (0 == strDescription.length())
	{
		WriteInfo(("sDescription == 0 {}"), GetLastError());
		goto END;
	}

	std::replace(strDescription.begin(), strDescription.end(), '\r', ' ');
	std::replace(strDescription.begin(), strDescription.end(), '\n', ' ');
	std::replace(strDescription.begin(), strDescription.end(), '\t', ' ');
	{
		std::wstring wsDescription = CStrUtil::ConvertA2W(strDescription);
		if (MAX_DESCRIPTION_SIZE < _tcslen(wsDescription.c_str()))
		{
			goto END;
		}
		_tcsncpy_s(pOneSysLog->wsEventDescription, _countof(pOneSysLog->wsEventDescription), wsDescription.c_str(), _countof(pOneSysLog->wsEventDescription) - 1);
	}

	bRet = TRUE;
END:
	return bRet;
}
unsigned int __stdcall CSysLogFun::DoSystemEventThread(LPVOID lpParameter)
{
	CSysLogFun* pSysLogFun = (CSysLogFun*)lpParameter;
	HANDLE hSysEvent = nullptr;
	PVOID Buffer = nullptr;
	DWORD BufferSize = 4096;
	DWORD dwRead = 0;
	DWORD dwNeeded = 0;

	CHAR LogName[] = "System";

	Buffer = malloc(BufferSize);
	if (!Buffer)
	{
		return 0;
	}

	// 打开系统日志
	hSysEvent = OpenEventLogA(NULL, "System");
	if (NULL == hSysEvent)
	{
		WriteError(("OpenEventLog System Failed, Error = {}"), GetLastError());
		return 0;
	}

	while (1)
	{
		// 有退出命令
		if (pSysLogFun->m_ReadSystemEventThreadExit)
		{
			break;
		}

		// 读取系统事件
		if (!ReadEventLogA(hSysEvent, EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ, 0, Buffer, BufferSize, &dwRead, &dwNeeded))
		{
			if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
			{
				// BufferSize 可能不够，重新申请
				if (Buffer)
				{
					free(Buffer);
					Buffer = NULL;
				}

				BufferSize = dwNeeded;
				Buffer = (EVENTLOGRECORD*)malloc(BufferSize);
				if (!Buffer)
				{
					break;
				}
				// 失败记录日志
				if (!ReadEventLogA(hSysEvent, EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ, 0, Buffer, BufferSize, &dwRead, &dwNeeded))
				{
					// log
					WriteInfo(("!!!! Failed to read event log record. Error code: {}  ThreadSecuritySysLog Exit"), GetLastError());
					break;
				}
			}
			else if (ERROR_HANDLE_EOF == GetLastError())
			{
				// 读取完毕,等待一段时间后继续读取 5s
				Sleep(5 * 1000);
				continue;
			}
			else if (GetLastError() == ERROR_EVENTLOG_FILE_CHANGED)
			{
				WriteInfo(("!!!! Failed to read event log record. Error code: {} Need To Reload OpenEventLogA"), GetLastError());
				Sleep(1000);
				CloseHandle(hSysEvent);
				hSysEvent = OpenEventLogA(NULL, "System");
				if (hSysEvent)
				{
					break;
				}
				continue;
			}
			else
			{
				// log
				WriteInfo(("!!!! Failed to read event log record. Error code: {}  ThreadSecuritySysLog Exit"), GetLastError());
				break;
			}
		}

		PEVENTLOGRECORD pevlr = (EVENTLOGRECORD*)Buffer;
		HOST_AD_SYSLOG_STRUCT oneSysLog;

		if (AnalyEventData((PCHAR)pevlr, pevlr->Length, LogName, &oneSysLog))
		{
			wstring wsEventDescription = oneSysLog.wsEventDescription;
			wstring wsEventTime = oneSysLog.wsEventTime;
			wstring wsEventComputerName = oneSysLog.wsEventComputerName;
			wstring wsEventSourceName = oneSysLog.wsEventSourceName;
			WriteInfo(("SysLog EventID = {} EventComputerName = {} EventTime = {} EventSourceName = {} RecodeNumber = {}"),
				oneSysLog.dwEventID, CStrUtil::ConvertW2A(wsEventComputerName), CStrUtil::ConvertW2A(wsEventTime), CStrUtil::ConvertW2A(wsEventSourceName), oneSysLog.dwEventRecordID);
		}
	}
	return 0;
}