#pragma once
#include <iostream>
#include <Windows.h>
#include <winnt.h>
#include <string>

#define MAX_PATH_LEN									512
#define MAX_INFO_LEN									256
#define MAX_TIME_LEN									40
#define MAX_CONTENT_LEN									1024
#define MAX_VERSION_LEN									16

// �ͻ��˹���Ա������־
typedef struct __ADMIN_OPERATION_LOG_STRUCT
{
	LONGLONG llTime;						// ʱ��
	WCHAR szUserName[MAX_PATH];				// �û����� ��ϵͳ����Ա��SuperAdmin ����Ա��Admin)
	WCHAR szLogContent[MAX_INFO_LEN];		// ��������
	DWORD dwIsSuccess;						// ������� (1���ɹ�  0��ʧ��)
}ADMIN_OPERATION_LOG_STRUCT, *PADMIN_OPERATION_LOG_STRUCT;