#include "SysLogFun.h"
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
}

BOOL CSysLogFun::GetSysLogByPsloglist(wstring wsStartDateTime, wstring wsEndDateTime, wstring wsLogClass)
{
	//E:\\psloglist.exe  -a 02/28/2015 -b 04/01/2016 Setup -s
	//E:\\psloglist.exe  -a 02/28/2015 -b 04/01/2016 Application  -s
	WriteInfo(("GetSysLogByPsloglist Begin"));
	wstring wsFilePath = CWindowsHelper::GetRunDir();

	wchar_t FileName[MAX_PATH] = { 0 };
	swprintf_s(FileName, _T("%s/%s.txt"), wsFilePath.c_str(),wsLogClass.c_str());
	wsFilePath = FileName;	// �ļ�·��   ../wsLogClass.txt

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

	//���ݰ汾���ù���ģʽ�Ͱ�ȫ����
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
	//ʹ�ñ�׼������ʾ����
	s.hStdOutput = hConsoleRedirect;//���ļ���Ϊ��׼������,��������GetStdHandle(STD_OUTPUT_HANDLE)��ǰ����̨�������ܵ���
	s.wShowWindow = SW_HIDE;//���ؿ���̨����
	PROCESS_INFORMATION pi = { 0 };

	static wstring wsWorkDir = CWindowsHelper::GetSystemDir();
	if (CreateProcess(NULL, (LPTSTR)wsCmdLine.c_str(), NULL, NULL, TRUE, NULL, NULL, wsWorkDir.c_str(), &s, &pi))
	{
		//��������,ִ��Ping����,���������Ƿ���ͨ
		WaitForSingleObject(pi.hProcess, INFINITE);

		//�ȴ�����ִ�����
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

BOOL CSysLogFun::GetSysLogByEvtSubscribe()
{
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadSysLogExport, this, 0, NULL);

	return 0;
}


void CSysLogFun::RecycleSysLogResource()
{
	m_EvtSubscribeThreadExit = TRUE;

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

}

EVT_HANDLE CSysLogFun::RegisterEvtCallBack(IN PEVT_CALLBACK_CONTEXT pContext, IN EVT_SUBSCRIBE_CALLBACK pFuncCallBack)
{
	EVT_HANDLE hEvtHandle = NULL;

	if (NULL == pContext || NULL == pFuncCallBack)
	{
		return NULL;
	}

	/*
	NULL��Ϊ��һ����������ʾû��ָ���¼����ĻỰ���µĻỰ����������
	NULL��Ϊ�ڶ�����������ʾû��ʹ���ź��¼���֪ͨ�����ߣ�����޷�ʵ���첽֪ͨ���ܡ����Ľ���ͬ���ģ�����Ҫ����ʹ��EvtNext�����������¼���
	pContext->wsChannelPath.c_str()��Ϊ������������ָ��Ҫ���ĵ��¼���־ͨ��·����pContext->wsChannelPath��һ���ַ�������ʾҪ���ĵ��¼�ͨ����·����
	NULL��Ϊ���ĸ���������ʾû��ָ����ѯ���ʽ�������������¼���
	NULL��Ϊ�������������ʾ�ӵ�ǰʱ��㿪ʼ�����¼�����ָ����ʼ����ǩλ�á�
	(PVOID)pContext��Ϊ������������������һ���û��Զ��������������ָ�룬��ָ�뽫�ڻص�������ʹ�á�
	pFuncCallBack��Ϊ���߸�������Ϊ���ĵĻص����������ڴ����ĵ����¼���
	EvtSubscribeToFutureEvents��Ϊ�ڰ˸�������ָ��������ΪΪ����δ���������¼���*/
	hEvtHandle = m_pEvtSubScript(NULL, NULL, pContext->wsChannelPath.c_str(), NULL, NULL, (PVOID)pContext, pFuncCallBack, EvtSubscribeToFutureEvents);//������֮����¼�
	if (NULL == hEvtHandle)
	{
		WriteError(("EvtSubscribe Failed, Error = {}"), GetLastError());
	}

	return hEvtHandle;
}
void GetEvtXmlDataNodes(IXMLDOMNodeListPtr pNodeList, std::vector<IXMLDOMNodePtr> &veXmlNodes)
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

	//����vecNode����ȡ���¼���
	for (int n = 0; n < vecNode.size(); n++)
	{
		IXMLDOMNamedNodeMapPtr pAttrMap = NULL;

		pNodeName = NULL;
		pNodeValue = NULL;

		//��ȡ �ڵ��� �� text ֵ
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
			else if (0 == wcscmp(pNodeName, _T("Level")))//�ȼ�
			{
				pOneSyslog->dwEventType = _wtoi64(pNodeValue);
			}
			else if (0 == wcscmp(pNodeName, _T("EventRecordID")))//�ȼ�
			{
				pOneSyslog->dwEventRecordID = _wtoi64(pNodeValue);
			}
			else if (0 == wcscmp(pNodeName, _T("Channel")))//��־����
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
		NodeΪProvider��Name��GuidΪAttrMap��Microsoft-Windows-Security-AuditingΪName��ֵ
		<EventID>4673</EventID>
		NodeΪEventID��4673Ϊtextֵ
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

		//����pAttrMap������pAttrItem,Ŀ��Ϊ�˻�ȡProvider��TimeCreated��ֵ
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

			// �õ�pAttrItem�� �ڵ��� �� text
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
	//�õ�������Ԫ���ݾ��
	EVT_HANDLE hPublisherMetadata = NULL;
	hPublisherMetadata = m_pEvtOpenPublisherMetadata(NULL, pOneSysLog->wsEventSourceName, NULL, 0, 0);
	if (hPublisherMetadata == NULL)
	{
		WriteError(("hPublisherMetadata =NULL; {}"), GetLastError());
	}
	DWORD bufferSize = 0;
	LPWSTR pFormattedMessage = NULL;

	//�����¼�����
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
// ����ϵͳ��־�¼��Ļص�����
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
	//�õ��˴��¼���XML����
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
		���ڽ��¼�����е��¼���Ϣ��ȾΪXML��ʽ��������洢��ָ���Ļ������С�����Ժ������ò���������ϸ���ͣ�
		��һ������ NULL����ʾδʹ�ã�����һ�������������˴���ΪNULL���ɡ�
		�ڶ������� hEvtHandle����ʾ�¼��������һ��EVT_HANDLE���͵ı���������ָ��Ҫ��Ⱦ���¼���
		���������� EvtRenderEventXml����ʾ��Ⱦ��־��ָʾ���¼���ȾΪXML��ʽ��
		���ĸ����� dwBufferSize����ʾ��������С������ָ��������Ⱦ����¼���Ϣ����Ļ�������С��
		��������� pEvtData����ʾ������ָ�룬���ڽ�����Ⱦ����¼���Ϣ��
		���������� &dwBufferUsed����ʾ���������ָ��һ��DWORD���͵ı��������ڷ���ʵ��ʹ�õĻ�������С��
		���߸����� &dwPropertyCount����ʾ���������ָ��һ��DWORD���͵ı��������ڷ�����Ⱦ����¼�ӵ�е�����������
		*/
		if (!pEvtCBKContext->pCSysLogFun->m_pEvtRender(NULL, hEvtHandle, EvtRenderEventXml, dwBufferSize, pEvtData, &dwBufferUsed, &dwPropertyCount))
		{
			goto END;
		}
		//����xml����
		CoInitialize(NULL);
		HOST_AD_SYSLOG_STRUCT oneSyslog;
		bRet = AnalyEvtXmlData(pEvtData, pEvtCBKContext, &oneSyslog);

		// ��ȡ�¼���Ϣ����
		pEvtCBKContext->pCSysLogFun->GetEventDescription(&oneSyslog, hEvtHandle);
		wstring wsEventDescription = oneSyslog.wsEventDescription;
		wstring wsEventTime = oneSyslog.wsEventTime;
		wstring wsEventComputerName = oneSyslog.wsEventComputerName;
		wstring wsEventSourceName = oneSyslog.wsEventSourceName;
		WriteInfo(("SysLog EventID = {} EventComputerName = {} EventTime = {} EventSourceName = {}"), 
			oneSyslog.dwEventID,CStrUtil::ConvertW2A(wsEventComputerName),CStrUtil::ConvertW2A(wsEventTime),CStrUtil::ConvertW2A(wsEventSourceName));
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

	return TRUE;
}

unsigned int WINAPI CSysLogFun::ThreadSysLogExport(LPVOID lpParameter)
{
	CSysLogFun* pSysLogFun = (CSysLogFun*)lpParameter;
	//���ô˷�����Ϊ�˷�ֹXPϵͳȱ��һЩ��̬�⣬�����������˸�ģ��
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

	//winXP����
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

