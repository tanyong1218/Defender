/********************************************************************************
*		Copyright (C) 2021 Beijing winicssec Technology
*		All rights reserved
*
*		filename : FileEaHelper
*		description : 文件额外属性帮助文件，包括读，写，删除等
*		此文件将用于白名单模块，在任意添加白名单到库中的地方都要写文件额外属性到文件中
*		文件额外属性包括"WLHASH" : xxxxxxxxx,文件Hash
*
*		created by yong.tan 2024-12-05
*
*****************************************************************************/

#include "FileEaHelper.h"
#pragma warning(disable:4996)
// 声明 NT API 函数原型
typedef NTSTATUS(NTAPI* PNtSetEaFile)(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID Buffer,
	IN ULONG Length
	);

typedef NTSTATUS(NTAPI* PNtQueryEaFile)(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID Buffer,
	IN ULONG Length,
	IN BOOLEAN ReturnSingleEntry,
	IN PVOID EaList OPTIONAL,
	IN ULONG EaListLength,
	IN PULONG EaIndex OPTIONAL,
	IN BOOLEAN RestartScan
	);
typedef NTSTATUS(NTAPI* PNtQueryInformationFile)(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	);

FileEAHelper::FileEAHelper()
{

}

FileEAHelper::~FileEAHelper()
{

}



// 写入扩展属性
BOOL FileEAHelper::WriteFileExAttr(string strFileName, const char *xattrName, string strAttrValue) 
{
	HANDLE hFile;
	DWORD bytesWritten;
	IO_STATUS_BLOCK io;
	NTSTATUS status;

	// 加载 ntdll.dll
	HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
	PNtSetEaFile NtSetEaFile = (PNtSetEaFile)GetProcAddress(hNtdll, "NtSetEaFile");


	hFile = CreateFileA(strFileName.c_str(),
		FILE_WRITE_EA,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		WriteError(("Could not open file {}. Error: {}"), strFileName.c_str(), GetLastError());
		return FALSE;
	}

	FILE_FULL_EA_INFORMATION *eaInfo;
	DWORD eaSize = sizeof(FILE_FULL_EA_INFORMATION) + strlen(xattrName) + strAttrValue.length() + 2;
	eaInfo = (FILE_FULL_EA_INFORMATION *)malloc(eaSize);

	eaInfo->NextEntryOffset = 0;
	eaInfo->Flags = 0;
	eaInfo->EaNameLength = strlen(xattrName);
	eaInfo->EaValueLength = strAttrValue.length();

	strcpy((char*)eaInfo->EaName,  xattrName);
	strcpy((char*)eaInfo->EaName + eaInfo->EaNameLength + 1, strAttrValue.c_str());

	status = NtSetEaFile(hFile,&io,eaInfo,eaSize);


	free(eaInfo);
	CloseHandle(hFile);

	return TRUE;
}

// 读取扩展属性
BOOL FileEAHelper::ReadFileExAttr(string strFileName, const char *xattrName) 
{
	HANDLE hFile;
	IO_STATUS_BLOCK ioStatus;
	NTSTATUS status;
	char buffer[4096];

	// 加载 ntdll.dll
	HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
	PNtQueryEaFile NtQueryEaFile = (PNtQueryEaFile)GetProcAddress(hNtdll, "NtQueryEaFile");

	hFile = CreateFileA(strFileName.c_str(),
		FILE_READ_EA,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		WriteError(("Could not open file {}. Error: {}"), strFileName.c_str(), GetLastError());
		return FALSE;
	}

	// 查询EA
	status = NtQueryEaFile(hFile,
		&ioStatus,
		buffer,
		sizeof(buffer),
		FALSE,
		NULL,
		0,
		NULL,
		TRUE);

	if (0 == status) 
	{
		PFILE_FULL_EA_INFORMATION eaInfo = (PFILE_FULL_EA_INFORMATION)buffer;
		while (eaInfo) 
		{
			char nameBuf[256] = { 0 };
			strncpy_s(nameBuf, eaInfo->EaName, eaInfo->EaNameLength);

			if (_stricmp(nameBuf, xattrName) == 0) 
			{
				char valueBuf[4096] = { 0 };
				strncpy_s(valueBuf, eaInfo->EaName + eaInfo->EaNameLength + 1, eaInfo->EaValueLength);
				WriteError(("EA {} = {}"), nameBuf, valueBuf);
				break;
			}

			if (eaInfo->NextEntryOffset == 0) 
			{
				break;
			}
			eaInfo = (PFILE_FULL_EA_INFORMATION)((char*)eaInfo + eaInfo->NextEntryOffset);
		}
	}
	else 
	{
		WriteError(("Failed to query EA. Status: {}"),status);
	}
	CloseHandle(hFile);

	return TRUE;
}

#define FILE_NEED_EA                    0x00000080
NTSTATUS FileEAHelper::DeleteEa(HANDLE FileHandle, const char* xattrName)
{
	NTSTATUS status;
	FILE_FULL_EA_INFORMATION *eaInfo;

	// 加载 ntdll.dll
	HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
	PNtSetEaFile NtSetEaFile = (PNtSetEaFile)GetProcAddress(hNtdll, "NtSetEaFile");

	DWORD eaSize = sizeof(FILE_FULL_EA_INFORMATION) + strlen(xattrName) + 1 ;
	eaInfo = (FILE_FULL_EA_INFORMATION *)malloc(eaSize);

	eaInfo->NextEntryOffset = 0;
	eaInfo->Flags = 0;
	eaInfo->EaNameLength = strlen(xattrName);
	eaInfo->EaValueLength = 0;

	strcpy_s((char*)eaInfo->EaName, eaInfo->EaNameLength, xattrName);

	IO_STATUS_BLOCK ioStatus;
	status =  NtSetEaFile(
		FileHandle,
		&ioStatus,
		eaInfo,
		eaSize
		);

	free(eaInfo);
	return status;
}
// 读取扩展属性
BOOL FileEAHelper::DeleteFileExAttr(const char *filename, const char *xattrName) 
{
	HANDLE hFile;
	NTSTATUS status;

	hFile = CreateFileA(filename,
		FILE_WRITE_EA|FILE_READ_EA,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		WriteError(("Could not open file '{}'. Error: {}"), filename, GetLastError());
		return FALSE;
	}

	// 查询EA
	status = DeleteEa(hFile,xattrName);

	if (0 == status) 
	{
		WriteInfo(("suc to DeleteEa EA. Status: {}"), status);
	}
	else 
	{
		WriteError(("suc to DeleteEa EA. Status: {}"), status);
	}

	CloseHandle(hFile);
	return TRUE;
}

