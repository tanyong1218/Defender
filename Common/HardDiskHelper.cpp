#include "HardDiskHelper.h"
#include <strsafe.h>
#include <winioctl.h>
#include <devguid.h>    // for GUID_DEVCLASS_CDROM etc
#include <setupapi.h>
#include <cfgmgr32.h>   // for MAX_DEVICE_ID_LEN, CM_Get_Parent and CM_Get_Device_ID
#include <algorithm>    // transform
#include <tchar.h>
#include "WindowsHelper.h"
#pragma comment (lib, "setupapi.lib")

CGetHardDiskSerialNumber::CGetHardDiskSerialNumber(void)
{
}

CGetHardDiskSerialNumber::~CGetHardDiskSerialNumber(void)
{
}
//  function to decode the serial numbers of IDE hard drives
//  using the IOCTL_STORAGE_QUERY_PROPERTY command
char* flipAndCodeBytes(const char* str,
	int pos,
	int flip,
	char* buf)
{
	int i;
	int j = 0;
	int k = 0;

	buf[0] = '\0';
	if (pos <= 0)
		return buf;

	if (!j)
	{
		char p = 0;

		// First try to gather all characters representing hex digits only.
		j = 1;
		k = 0;
		buf[k] = 0;
		for (i = pos; j && str[i] != '\0'; ++i)
		{
			char c = tolower(str[i]);

			if (isspace(c))
				c = '0';

			++p;
			buf[k] <<= 4;

			if (c >= '0' && c <= '9')
				buf[k] |= (unsigned char)(c - '0');
			else if (c >= 'a' && c <= 'f')
				buf[k] |= (unsigned char)(c - 'a' + 10);
			else
			{
				j = 0;
				break;
			}

			if (p == 2)
			{
				if (buf[k] != '\0' && !isprint(buf[k]))
				{
					j = 0;
					break;
				}
				++k;
				p = 0;
				buf[k] = 0;
			}
		}
	}

	if (!j)
	{
		// There are non-digit characters, gather them as is.
		j = 1;
		k = 0;
		for (i = pos; j && str[i] != '\0'; ++i)
		{
			char c = str[i];

			if (!isprint(c))
			{
				j = 0;
				break;
			}

			buf[k++] = c;
		}
	}

	if (!j)
	{
		// The characters are not there or are not printable.
		k = 0;
	}

	buf[k] = '\0';

	if (flip)
		// Flip adjacent characters
		for (j = 0; j < k; j += 2)
		{
			char t = buf[j];
			buf[j] = buf[j + 1];
			buf[j + 1] = t;
		}

	// Trim any beginning and end space
	i = j = -1;
	for (k = 0; buf[k] != '\0'; ++k)
	{
		if (!isspace(buf[k]))
		{
			if (i < 0)
				i = k;
			j = k;
		}
	}

	if ((i >= 0) && (j >= 0))
	{
		for (k = i; (k <= j) && (buf[k] != '\0'); ++k)
			buf[k - i] = buf[k];
		buf[k - i] = '\0';
	}

	return buf;
}
int CGetHardDiskSerialNumber::GetHardDriveSerialNumber(string& HardDriveSerialNumber)
{
	int done = FALSE;
	int drive = 0;
	TCHAR path[MAX_PATH] = { 0 };
	HANDLE hVolume = INVALID_HANDLE_VALUE;
	TCHAR rawDiskName[MAX_PATH] = { 0 };
	BOOL retcode = FALSE;
	DWORD bytesReturned;
	STORAGE_DEVICE_NUMBER   deviceInfo;
	HANDLE hPhysicalDriveIOCTL = INVALID_HANDLE_VALUE;
	CWindowsHelper GetWindowsVersion;
	int nWindowsVersion = 0;
	BOOL bWin64 = FALSE;
	GetWindowsVersion.SeGetWindowsVersion(nWindowsVersion, bWin64);

	/*
	在xp和2003的真机环境下该函数无法执行成功
	vista真机环境没有安装成功，暂时跳过
	*/
	if (nWindowsVersion <= WIN_2003_SERVER_R2_X64 || (nWindowsVersion == WIN_VISTA_X32 || nWindowsVersion == WIN_VISTA_X64))
	{
		goto END;
	}

	//获取系统目录
	if (!GetWindowsDirectory(path, MAX_PATH))
	{
		goto END;
	}

	//设备命名空间名称"\\\\.\\C:",这种方式可以直接访问磁盘卷
	_sntprintf_s(rawDiskName, MAX_PATH, _T("\\\\.\\%c:"), path[0]);

	hVolume = CreateFile(rawDiskName, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);

	if (hVolume == INVALID_HANDLE_VALUE)
	{
		goto END;
	}

	retcode = DeviceIoControl(hVolume,
		IOCTL_STORAGE_GET_DEVICE_NUMBER,
		NULL,
		0,
		&deviceInfo,
		sizeof(deviceInfo),
		&bytesReturned,
		NULL);

	if (!retcode)
	{
		goto END;
	}

	drive = deviceInfo.DeviceNumber;

	do
	{
		STORAGE_PROPERTY_QUERY query;
		DWORD cbBytesReturned = 0;
		char buffer[10000] = { 0 };
		char driveName[MAX_PATH] = { 0 };

		//\\\\.\\PhysicalDrive1是直接访问物理磁盘的设备路径，通过这种设备路径，可以直接对物理磁盘进行读写操作，而不是访问磁盘上的卷。
		sprintf_s(driveName, "\\\\.\\PhysicalDrive%d", drive);

		//  Windows NT, Windows 2000, Windows XP - admin rights not required
		hPhysicalDriveIOCTL = CreateFileA(driveName, 0,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL);
		if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
		{
			goto END;
		}

		memset((void*)&query, 0, sizeof(query));
		query.PropertyId = StorageDeviceProperty;
		query.QueryType = PropertyStandardQuery;

		memset(buffer, 0, sizeof(buffer));

		if (DeviceIoControl(hPhysicalDriveIOCTL, IOCTL_STORAGE_QUERY_PROPERTY,
			&query,
			sizeof(query),
			&buffer,
			sizeof(buffer),
			&cbBytesReturned, NULL))
		{
			STORAGE_DEVICE_DESCRIPTOR* descrip = (STORAGE_DEVICE_DESCRIPTOR*)&buffer;
			char serialNumber[1000] = { 0 };
			char modelNumber[1000] = { 0 };
			char vendorId[1000] = { 0 };
			char productRevision[1000] = { 0 };
			BOOL flip = FALSE;

			wstring strEunmeratorName;

			if (nWindowsVersion < WIN_8_0_X32)//win8 后的系统对使用ata驱动的硬盘自动翻转
			{
				//查找硬盘枚举值
				if (!FindDiInfos(deviceInfo.DeviceType, deviceInfo.DeviceNumber, strEunmeratorName))
				{
					goto END;
				}

				transform(strEunmeratorName.begin(), strEunmeratorName.end(), strEunmeratorName.begin(), ::toupper);
				if (strEunmeratorName.compare(_T("IDE")) == 0)
				{
					flip = TRUE;
				}
			}

			flipAndCodeBytes(buffer,
				descrip->VendorIdOffset,
				0, vendorId);
			flipAndCodeBytes(buffer,
				descrip->ProductIdOffset,
				0, modelNumber);
			flipAndCodeBytes(buffer,
				descrip->ProductRevisionOffset,
				0, productRevision);
			flipAndCodeBytes(buffer,
				descrip->SerialNumberOffset,
				flip, serialNumber);

			if (isalnum(serialNumber[0]) || isalnum(serialNumber[19]))
			{
				HardDriveSerialNumber = serialNumber;
				done = TRUE;
			}
		}
	} while (FALSE);

END:

	if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hPhysicalDriveIOCTL);
	}

	if (hVolume != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hVolume);
	}

	return done;
}

BOOL CGetHardDiskSerialNumber::FindDiInfos(__in DEVICE_TYPE DeviceType, __in DWORD DeviceNumber, __out wstring& strEunmeratorName)
{
	BOOL bRes = FALSE;
	HDEVINFO hDeviceInfoSet = NULL;
	int nIndex = 0;
	PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = NULL;
	HANDLE hDev = NULL;
	int nMax = 100;//只用来终止循环，防止死循环

	//获取所有磁盘类型的设备
	hDeviceInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (INVALID_HANDLE_VALUE == hDeviceInfoSet)
	{
		hDeviceInfoSet = NULL;
		goto END;
	}

	//枚举设备
	for (nIndex = 0; nIndex < nMax; nIndex++)
	{
		SP_DEVICE_INTERFACE_DATA interfaceData;
		SP_DEVINFO_DATA deviceInfoData;
		DWORD dwDataType = 0;
		DWORD dwRequiredSize = 0;
		STORAGE_DEVICE_NUMBER sdn;
		DWORD cbBytesReturned = 0;
		BOOL bSuccess = FALSE;

		//获取设备的接口信息
		ZeroMemory(&interfaceData, sizeof(interfaceData));
		interfaceData.cbSize = sizeof(interfaceData);
		bSuccess = SetupDiEnumDeviceInterfaces(hDeviceInfoSet, NULL, &GUID_DEVINTERFACE_DISK, nIndex, &interfaceData);
		if (!bSuccess)
		{
			DWORD dwErrorCode = GetLastError();
			if (dwErrorCode == ERROR_NO_MORE_ITEMS)
			{
				break;
			}
			else
			{
				goto END;
			}
		}

		//获取所需的缓冲区大小
		bSuccess = SetupDiGetDeviceInterfaceDetail(hDeviceInfoSet, &interfaceData, NULL, 0, &dwRequiredSize, NULL);
		if ((!bSuccess && GetLastError() != ERROR_INSUFFICIENT_BUFFER) || dwRequiredSize == 0)
		{
			goto END;
		}

		pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) new char[dwRequiredSize];
		if (!pInterfaceDetailData)
		{
			goto END;
		}
		//获取接口详细信息
		pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		ZeroMemory(&deviceInfoData, sizeof(deviceInfoData));
		deviceInfoData.cbSize = sizeof(deviceInfoData);
		bSuccess = SetupDiGetDeviceInterfaceDetail(hDeviceInfoSet, &interfaceData, pInterfaceDetailData, dwRequiredSize, &dwRequiredSize, &deviceInfoData);
		if (!bSuccess)
		{
			goto END;
		}

		hDev = CreateFile(pInterfaceDetailData->DevicePath,
			0,                                   // no access to the drive
			FILE_SHARE_READ | FILE_SHARE_WRITE,  // share mode
			NULL,                                // default security attributes
			OPEN_EXISTING,                       // disposition
			0,                                   // file attributes
			NULL);

		if (INVALID_HANDLE_VALUE == hDev)
		{
			goto END;
		}

		bSuccess = DeviceIoControl(hDev,                           // device to be queried
			IOCTL_STORAGE_GET_DEVICE_NUMBER,
			NULL, 0,                        // no input buffer
			(LPVOID)&sdn, sizeof(sdn),      // output buffer
			&cbBytesReturned,               // # bytes returned
			(LPOVERLAPPED)NULL);           // synchronous I/O

		if (!bSuccess)
		{
			goto END;
		}

		if (sdn.DeviceType == DeviceType &&
			sdn.DeviceNumber == DeviceNumber)
		{
			TCHAR szBuffer[100] = { 0 };

			bSuccess = SetupDiGetDeviceRegistryProperty(hDeviceInfoSet, &deviceInfoData, SPDRP_ENUMERATOR_NAME, &dwDataType,
				(PBYTE)szBuffer, sizeof(szBuffer), &dwRequiredSize);
			if (!bSuccess)
			{
				goto END;
			}

			//输出枚举值

			strEunmeratorName = szBuffer;
		}

		if (hDev)
		{
			CloseHandle(hDev);
			hDev = NULL;
		}

		if (pInterfaceDetailData)
		{
			delete[](char*)pInterfaceDetailData;
			pInterfaceDetailData = NULL;
		}
	}

	bRes = TRUE;
END:

	if (hDev)
	{
		CloseHandle(hDev);
	}

	if (pInterfaceDetailData)
	{
		delete[](char*)pInterfaceDetailData;
		pInterfaceDetailData = NULL;
	}

	if (hDeviceInfoSet)
	{
		SetupDiDestroyDeviceInfoList(hDeviceInfoSet);
		hDeviceInfoSet = NULL;
	}
	return bRes;
}