#include "Clipboard.h"
#include "ClipboardManage.h"

#ifdef CLIPBOARDCONTROL_EXPORTS
#define CLIPBOARDCONTROL_API __declspec(dllexport)
#else
#define CLIPBOARDCONTROL_API __declspec(dllimport)
#endif
Clipboard* Clipboard::m_instance = nullptr;

Clipboard::Clipboard()
{

}

Clipboard::~Clipboard()
{

}

Clipboard* Clipboard::GetInstance()
{
	if (m_instance == NULL)
	{
		m_instance = new Clipboard();
	}

	return m_instance;
}

void Clipboard::ReleaseInstance()
{
	if (m_instance)
	{
		delete m_instance;
		m_instance = nullptr;  // 关键是要把静态成员置空
	}
}

DWORD Clipboard::UnRegister()
{
	return 0;
}

IComponent* Clipboard::Register()
{
	return (IComponent*)GetInstance();
}

BOOL Clipboard::EnableFunction()
{
	ClipboardManage::GetInstance().Init();
	return 0;
}

BOOL Clipboard::DisableFunction()
{
	return 0;
}

BOOL Clipboard::DispatchMessages(IPC_MSG_DATA* pIpcMsg)
{
	switch (pIpcMsg->dwMsgCode)
	{
	case CLIPBOARD_CONTROL_OPEN_ALL_FUNCTION:
		EnableFunction();
		break;
	case CLIPBOARD_CONTROL_CLOSE_ALL_FUNCTION:
		DisableFunction();
		break;
	default:
		break;
	}
	return 0;
}



CLIPBOARDCONTROL_API IComponent* GetComInstance()
{
	//WriteInfo("Welcome to DeviceControl!");
	Clipboard* instance = Clipboard::GetInstance();
	return instance;
}

CLIPBOARDCONTROL_API void ReleaseComInstance()
{
   Clipboard::ReleaseInstance();
}