#pragma once

class CNtDriverService
{
public:
	CNtDriverService();
	CNtDriverService(LPCTSTR lpszName);
	~CNtDriverService();

	BOOL Initialize();
	VOID Destroy();

	void SetDriverName(LPCTSTR lpszName);
	BOOL Install(LPCTSTR lpszExePath, LPCTSTR lpszDesc);
	BOOL UpdateExePath(LPCTSTR lpszExePath, LPCTSTR lpszDesc);
	VOID Uninstall();

	BOOL Start();
	BOOL Stop();

	BOOL UnloadDriver();

private:
	SC_HANDLE m_hSCManager;
	CString m_strServiceName;
};
