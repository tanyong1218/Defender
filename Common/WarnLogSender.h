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

// 客户端管理员操作日志
typedef struct __ADMIN_OPERATION_LOG_STRUCT
{
	LONGLONG llTime;						// 时间
	WCHAR szUserName[MAX_PATH];				// 用户名称 （系统管理员：SuperAdmin 管理员：Admin)
	WCHAR szLogContent[MAX_INFO_LEN];		// 操作内容
	DWORD dwIsSuccess;						// 操作结果 (1：成功  0：失败)
}ADMIN_OPERATION_LOG_STRUCT, *PADMIN_OPERATION_LOG_STRUCT;