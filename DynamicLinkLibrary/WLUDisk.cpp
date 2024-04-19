// DynamicLinkLibrary.cpp : 定义 DLL 的导出函数。
//

#include "framework.h"
#include "WLUDisk.h"
#pragma comment(lib,"Setupapi.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib,"shlwapi.lib")


static HANDLE	g_DeviceNotifyHandle = NULL;
static HWND		g_SrvWnd = NULL;
static BOOL		g_StopMonitorThread = FALSE;

UINT WM_EXIT_APP = RegisterWindowMessage(L"WM_EXIT_APP");
CWLUDisk* CWLUDisk::m_instance = nullptr;

using TaskFunc = std::function<void()>;

vector<wstring> CWLUDisk::m_stcDevInfo =
{
	_T("cdrom") ,
	_T("net")  ,
	_T("bluetooth"),
	_T("ports"),
	_T("diskdrive"),
	_T("floppydisk"),
	_T("wpd"),
	_T("usb")
};

// 这是导出函数的一个示例。
DYNAMICLINKLIBRARY_API int fnDynamicLinkLibrary(void)
{
	return 0;
}

void CWLUDisk::GetSymbolicName(DEVINST devInst, wstring& strSymbolicName)
{
	CONFIGRET cr = CR_SUCCESS;
	WCHAR deviceID[MAX_DEVICE_ID_LEN];
	cr = CM_Get_Device_ID(devInst, deviceID, MAX_DEVICE_ID_LEN, 0);
	if (cr != CR_SUCCESS) {
		WriteError(("Failed to get device ID. Error: {}"), cr);
		return;
	}
	HKEY hKey;
	WCHAR regPath[MAX_PATH];
	swprintf(regPath, MAX_PATH, L"SYSTEM\\CurrentControlSet\\Enum\\%s\\Device Parameters", deviceID);

	LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS) {
		WriteError(("Failed to open device registry key. Error: {}"), result);
		return;
	}

	// Read the "Device Parameters" key to get the interface GUID
	WCHAR SymbolicName[MAX_PATH]{};
	DWORD valueType = 0;
	DWORD dataSize = sizeof(SymbolicName);
	result = RegQueryValueEx(hKey, L"SymbolicName", NULL, &valueType, (LPBYTE)SymbolicName, &dataSize);
	if (result != ERROR_SUCCESS)
	{
		WriteError(("Failed to retrieve device interface GUID. Error: {}"), result);
		RegCloseKey(hKey);
		return;
	}
	strSymbolicName = SymbolicName;
	RegCloseKey(hKey);


}

wstring CWLUDisk::getVendorNameByVid(unsigned short VendorID)
{
	//遍历USBVendorIDs找到vid对应的公司信息
	wstring wsVendorInfo = _T("N/A");
	auto USBVendorIDs = GetUSBVendorIDs();
	auto it = USBVendorIDs->find(VendorID);
	if (it != USBVendorIDs->end())
	{
		wsVendorInfo = CStrUtil::UTF8ToUnicode(it->second);
	}
	return wsVendorInfo;
}


/*
* @function		getUsbDevProductInfo
* @brief		获取USB设备的父节点的信息，包括端口号，Hub号，设备名。
* @param[in]    vectHubDevBaseInfo: USBHub的信息，包括devpath，devinst
* @param[in]    USBDeviceInfo: USB设备的信息，包括devpath，devinst
* @param[out]   wstrProductInfo: USB设备的设备名
* @return
* @date         2023-10-10
* @note
* @warning
*/
void CWLUDisk::getUsbDevProductInfo(vector<DevPathAndDevInst>& vectHubDevBaseInfo, DeviceInfoFull& USBDeviceInfo, wstring& wstrProductInfo)
{
	int			PortId = 0;
	int			hubId = 0;
	int			ret;
	wstring		strError;
	wstring		strPostionInfo;
	DEVINST	   dnUSBHubDevInst = 0;

	ret = CM_Get_Parent(&dnUSBHubDevInst, USBDeviceInfo.DeviceInfoData.DevInst, 0);//找到此USB设备的的父节点的devInst dnUSBHubDevInst。
	if (CR_SUCCESS != ret)
	{
		return;
	}
	const WCHAR* USBHubDevicePath = NULL;
	vector<DevPathAndDevInst>::iterator USBHubit;
	for (USBHubit = vectHubDevBaseInfo.begin(); USBHubit != vectHubDevBaseInfo.end(); ++USBHubit)
	{
		if (USBHubit->DevInst == dnUSBHubDevInst)//匹配USB设备的的父节点的devInst对应哪个HubDev
		{
			USBHubDevicePath = USBHubit->wstrDevicePath.c_str();
			USBHubDevicePath += 4;//找到Hub的devpath. 略过前四个字符"\\?\"
		}
	}
	//get CM_DRP_LOCATION_INFORMATION
	GetCMPropertybyDevinst(USBDeviceInfo.DeviceInfoData.DevInst, CM_DRP_LOCATION_INFORMATION, strPostionInfo);
	extractPortAndHub(strPostionInfo, PortId, hubId);
	//wstring wstrDeviceName;
	GetUsbDeviceDeviceName(USBHubDevicePath, PortId, wstrProductInfo, strError);

	USBDeviceInfo.ParentDevPort = PortId;
	USBDeviceInfo.ParentDevHub = hubId;
}


/*
* @fn           GetUsbDeviceDeviceName
* @brief        通过usb ioctl IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION 取出USB_STRING_DESCRIPTOR_TYPE 相关的信息。
* @param[in]    pszUSBHubDevicePath： usbhub（集线器）的devpath USB#ROOT_HUB30#4&192bb69c&0&0#{f18a0e88-c30c-11d0-8815-00a0c906bed8})
* @param[in]    ConnectIndex:usb设备在hub插入的端口。

* @param[out]   wstrUSBDeviceName 返回：USB设备的设备名。如USB Keyboard .Elements SE 2623
* @param[out]
*
* @author       dan.liu
* @date         2023-10-10
*/
BOOL CWLUDisk::GetUsbDeviceDeviceName(const WCHAR* pszUSBHubDevicePath,
	int					ConnectIndex,
	__out std::wstring& wstrUSBDeviceName,
	std::wstring& strError)
{
	DWORD						dwError = 0;
	wstring						strTemp;
	BOOL						bResult = false;
	wstring						cstr;
	HANDLE						hHubDevice = INVALID_HANDLE_VALUE;

	USB_NODE_CONNECTION_NAME	ConnectionName = { 0 };
	USB_NODE_CONNECTION_NAME* pconnectionName = NULL;

	if (NULL == pszUSBHubDevicePath)
	{
		strTemp = _T("NULL == pszDevicePath");
		goto END;
	}
	cstr = _T("\\\\.\\");
	cstr += pszUSBHubDevicePath;
	hHubDevice = CreateFile(cstr.c_str(),
		GENERIC_READ,// 访问权限
		FILE_SHARE_READ,  //共享模式
		NULL,// 使用默认的安全属性
		OPEN_EXISTING,// 打开存在的设备
		NULL,
		NULL);
	if (INVALID_HANDLE_VALUE == hHubDevice)
	{
		dwError = GetLastError();
		strTemp = _T("hDevice is INVALID_HANDLE_VALUE");
		WriteError("hDevice is INVALID_HANDLE_VALUE, error= {}", dwError);
		goto END;
	}

	{
		USB_NODE_CONNECTION_INFORMATION ConnectInfo;
		HubGetNodeConnectionInformation(hHubDevice, ConnectIndex, ConnectInfo, strError);
		HubGetNodeStringDescriptorProduct(hHubDevice, ConnectIndex, ConnectInfo.DeviceDescriptor, wstrUSBDeviceName, strError);
		bResult = true;
	}

END:
	strError = strTemp.data();

	if (pconnectionName)
	{
		free(pconnectionName);
		pconnectionName = NULL;
	}

	if (INVALID_HANDLE_VALUE != hHubDevice)
	{
		CloseHandle(hHubDevice);
		hHubDevice = INVALID_HANDLE_VALUE;
	}

	return bResult;
}

BOOL CWLUDisk::HubGetNodeStringDescriptorProduct(
	_In_ HANDLE							hHubDevice,
	_In_ ULONG							ConnectIndex,
	_In_ USB_DEVICE_DESCRIPTOR& DevDescInfo,
	__out wstring& WstrDesc,
	__out std::wstring& strError)
{
	return  GetStringDescriptor(hHubDevice, ConnectIndex, DevDescInfo.iProduct, 0x0409, WstrDesc, strError);

}

/*
* @fn           GetStringDescriptor
* @brief        通过usb ioctl IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION 取出USB_STRING_DESCRIPTOR_TYPE 相关的信息。
* @param[in]    hHubDevice： usbhub（集线器）（createfile(\\\\.\\USB#ROOT_HUB30#4&192bb69c&0&0#{f18a0e88-c30c-11d0-8815-00a0c906bed8})可打开）的handle
* @param[in]    ConnectionIndex:usb设备在hub插入的端口。
* @param[in]    DescriptorIndex 希望读取的哪一类Desc信息。此信息从HubGetNodeConnectionInformation 取出来。
* @param[in]    LanguageID = 0就可以。//0x0409  "English (United States)"  ; 0x0804   "Chinese (PRC)"

* @param[out]   WstrOutDesc 返回：插入在usbhub的port口（ConnectionIndex）设备DescriptorIndex对应的信息
* @param[out]
*
* @author       usbview
* @date         2023-10-10
*/
#define MAXIMUM_USB_STRING_LENGTH 512
BOOL CWLUDisk::GetStringDescriptor(
	_In_ HANDLE		hHubDevice,
	ULONG				ConnectionIndex,
	UCHAR				DescriptorIndex,
	USHORT				LanguageID,
	__out wstring& WstrOutDesc,
	__out std::wstring& strError
)
{
	BOOL    bRet = FALSE;
	BOOL    success = 0;
	ULONG   nBytes = 0;
	ULONG   nBytesReturned = 0;
	wstring cstrTemp;
	UCHAR   stringDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST) + MAXIMUM_USB_STRING_LENGTH];
	DWORD	len = 0;

	PUSB_DESCRIPTOR_REQUEST stringDescReq = NULL;
	PUSB_STRING_DESCRIPTOR  stringDesc = NULL;
	//PSTRING_DESCRIPTOR_NODE stringDescNode = NULL;

	nBytes = sizeof(stringDescReqBuf);

	stringDescReq = (PUSB_DESCRIPTOR_REQUEST)stringDescReqBuf;
	stringDesc = (PUSB_STRING_DESCRIPTOR)(stringDescReq + 1);

	// Zero fill the entire request structure
	//
	memset(stringDescReq, 0, nBytes);

	// Indicate the port from which the descriptor will be requested
	//
	stringDescReq->ConnectionIndex = ConnectionIndex;

	stringDescReq->SetupPacket.wValue = (USB_STRING_DESCRIPTOR_TYPE << 8)
		| DescriptorIndex;

	stringDescReq->SetupPacket.wIndex = LanguageID;

	stringDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

	// Now issue the get descriptor request.
	//
	success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
		stringDescReq,
		nBytes,
		stringDescReq,
		nBytes,
		&nBytesReturned,
		NULL);

	//
	// Do some sanity checks on the return from the get descriptor request.
	//

	if (!success)
	{
		WriteError(("DeviceIoControl failed, GetLastError={}. "), GetLastError());
		goto END;
	}

	if (nBytesReturned < 2)
	{
		WriteError(("nBytesReturned < 2, GetLastError={}. "), GetLastError());
		goto END;
	}

	if (stringDesc->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE)
	{
		WriteError(("stringDesc->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE, GetLastError={} "), GetLastError());
		goto END;
	}

	// 	if (stringDesc->bLength != nBytesReturned - sizeof(USB_DESCRIPTOR_REQUEST))
	// 	{
	// 		cstrTemp.Format(_T("stringDesc->bLength != nBytesReturned - sizeof(USB_DESCRIPTOR_REQUEST), GetLastError=%d. "),GetLastError());
	// 		goto END;
	// 	}

	if (stringDesc->bLength % 2 != 0)
	{
		WriteError(("stringDesc->bLength % 2 != 0, GetLastError={}. "), GetLastError());
		goto END;
	}

	len = (DWORD)stringDesc->bLength - (DWORD)sizeof(*stringDesc);

	if (len > 0)
	{
		WstrOutDesc.assign(stringDesc->bString, len);
		bRet = TRUE;
	}
END:
	strError += cstrTemp.data();
	return bRet;
}


/*
* @fn           HubGetNodeConnectionInformation
* @brief        通过usb ioctl IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION 取出USB_STRING_DESCRIPTOR_TYPE 相关的信息。
* @param[in]    hHubDevice： usbhub（集线器）（createfile(\\\\.\\USB#ROOT_HUB30#4&192bb69c&0&0#{f18a0e88-c30c-11d0-8815-00a0c906bed8})可打开）的handle
* @param[in]    ConnectIndex:usb设备在hub插入的端口。

* @param[out]   connectinfo 返回：插入在usbhub的port口（ConnectionIndex）设备的相关信息。
* @param[out]
*
* @author       usbview
* @date         2023-10-10
*/
BOOL CWLUDisk::HubGetNodeConnectionInformation(HANDLE							hHubDevice,
	int							ConnectIndex,
	USB_NODE_CONNECTION_INFORMATION& connectinfo,
	std::wstring& strError)
{
	BOOL		bRet = FALSE;
	DWORD		nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION) + sizeof(USB_PIPE_INFO) * 30;
	wstring		strTemp;

	PUSB_NODE_CONNECTION_INFORMATION pConnectionInfo = (PUSB_NODE_CONNECTION_INFORMATION)malloc(nBytes);
	if (pConnectionInfo == NULL)
	{
		strError = _T("malloc PUSB_NODE_CONNECTION_INFORMATION failed!");
		return FALSE;
	}

	pConnectionInfo->ConnectionIndex = ConnectIndex;

	BOOL success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
		pConnectionInfo,
		nBytes,
		pConnectionInfo,
		nBytes,
		&nBytes,
		NULL);

	if (!success)
	{
		strTemp = (_T("DeviceIoControl IOCTL_USB_GET_NODE_CONNECTION_INFORMATION failed! "));
		WriteError("{} DeviceIoControl IOCTL_USB_GET_NODE_CONNECTION_INFORMATION failed! ErrorCode= {}", __LINE__, GetLastError());
		goto END;

	}

	memcpy(&connectinfo, pConnectionInfo, sizeof(connectinfo));
	bRet = TRUE;
END:
	if (pConnectionInfo)
	{
		free(pConnectionInfo);
		pConnectionInfo = NULL;
	}
	strError += strTemp.data();
	return bRet;
}


void CWLUDisk::GetUsbDeviceProductInfo(vector<DevPathAndDevInst>& vectHubDevBaseInfo, DeviceInfoFull& USBDeviceInfo)
{
	USBDeviceInfo.wsVendorInfo = getVendorNameByVid(USBDeviceInfo.UsbVid);
	if (m_nWinVersion >= WIN_2008_SERVER_X32)
	{
		getUsbDevProductInfo(vectHubDevBaseInfo, USBDeviceInfo, USBDeviceInfo.wsProductInfo);
	}
	else
	{
		GetCMPropertybyDevinst(USBDeviceInfo.DeviceInfoData.DevInst, CM_DRP_LOCATION_INFORMATION, USBDeviceInfo.wsProductInfo);
	}

	if (USBDeviceInfo.wsProductInfo.empty())
	{
		//getCM_DRP_DEVICEDESC
		GetCMPropertybyDevinst(USBDeviceInfo.DeviceInfoData.DevInst, CM_DRP_DEVICEDESC, USBDeviceInfo.wsProductInfo);
	}

}

void CWLUDisk::GetCMPropertybyDevinst(const DEVINST& devInst, DWORD cm_drp_flag, wstring& WstRet)
{
	CONFIGRET cr = CR_SUCCESS;
	ULONG regDataType = 0;
	ULONG dataSize = 0;

	//// Step 1: Get the device instance ID
	//TCHAR deviceInstanceID[MAX_DEVICE_ID_LEN];
	//cr = CM_Get_Device_ID(devInst, deviceInstanceID, MAX_DEVICE_ID_LEN, 0);
	//if (cr != CR_SUCCESS) {
	//	std::cerr << "CM_Get_Device_ID failed. Error: " << cr << std::endl;
	//	return;
	//}

	// Step 2: Get the hardware ID
	cr = CM_Get_DevNode_Registry_PropertyW(devInst, cm_drp_flag, &regDataType, NULL, &dataSize, 0);
	if (cr != CR_BUFFER_SMALL)
	{
		WriteError("CM_Get_DevNode_Registry_PropertyW (1st call) failed. Error:  {}", cr);
		return;
	}

	// Allocate memory for the hardware ID
	WCHAR* PropertyInfo = new WCHAR[dataSize / sizeof(WCHAR)];
	if (!PropertyInfo)
	{
		WriteError("Memory allocation failed.");
		return;
	}

	// Retrieve the hardware ID
	cr = CM_Get_DevNode_Registry_PropertyW(devInst, cm_drp_flag, &regDataType, (PBYTE)PropertyInfo, &dataSize, 0);
	if (cr != CR_SUCCESS)
	{
		delete[] PropertyInfo;
		WriteError("CM_Get_DevNode_Registry_PropertyW (2nd call) failed. Error:  {}", cr);
		return;
	}
	WstRet = PropertyInfo;
	delete[] PropertyInfo;
}


BOOL CWLUDisk::GetDeviceFullInfo(
	HDEVINFO hDevInfo,
	PSP_DEVINFO_DATA pDeviceInfoData,
	vector<boost::shared_ptr<DeviceInfoFull>>& VectAllDevice,
	vector<DevPathAndDevInst>& vectHubPathAndDevinst)
{
	boost::shared_ptr<DeviceInfoFull> pDeviceInfo(new DeviceInfoFull);
	if (pDeviceInfo.get() == NULL)
	{
		return FALSE;
	}
	DeviceInfoFull& DeviceInfo = *(pDeviceInfo.get());
	DeviceInfo.SelfDevInst = pDeviceInfoData->DevInst;
	DEVINST		dnUSBHubDevInst = 0;
	WCHAR wcsDeviceInstanceId[256] = { 0 };
	wstring wstrDeviceName;
	DWORD ParentVid = 0;
	DWORD ParentPid = 0;
	DWORD SelfVid = 0;
	DWORD SelfPid = 0;
	//获取外设的属性(bluetooth，wpd，usb......)
	WCHAR wszClassName[256] = { 0 };

	SetupDiGetDeviceRegistryProperty(
		hDevInfo,
		pDeviceInfoData,
		SPDRP_CLASS, NULL,
		(PBYTE)wszClassName,
		sizeof(wszClassName),
		NULL);


	wstring wstrClassName = wszClassName;
	transform(wstrClassName.begin(), wstrClassName.end(), wstrClassName.begin(), ::tolower);
	DeviceInfo.SelfClassName = wstrClassName;
	//DeviceInfo.hDevInfo = hDevInfo;
	DeviceInfo.DeviceInfoData = *pDeviceInfoData;

	if (CM_Get_DevNode_Status(&DeviceInfo.dwDevStatus, &DeviceInfo.dwProblem, pDeviceInfoData->DevInst, 0) != CR_SUCCESS)
	{
		DeviceInfo.isInsert = FALSE;
		return FALSE;
	}
	else
	{
		DeviceInfo.isInsert = TRUE;
	}

	//只获取我们关注的类型软盘、蓝牙、USB......
	BOOL TypeisFound = FALSE;
	for (int i = 0; i < DEV_OTHER_MAX; i++)
	{
		if (m_stcDevInfo[i].compare(DeviceInfo.SelfClassName) == 0)
		{
			DeviceInfo.SelfClassType = (ENUM_DEV_TYPE)i;
			TypeisFound = TRUE;
			break;
		}
	}

	//获取父节点的信息
	CM_Get_Parent(&DeviceInfo.ParentDevInst, pDeviceInfoData->DevInst, 0);
	CM_Get_Device_ID(DeviceInfo.ParentDevInst, wcsDeviceInstanceId, 256, 0);
	DeviceInfo.ParentDevInstanceId = wcsDeviceInstanceId;

	//如果父节点InstanceId是USB开头，代表Parent节点为USB设备
	std::wstring wstrUSB = _T("USB");
	if (DeviceInfo.ParentDevInstanceId.length() >= wstrUSB.length())
	{
		DeviceInfo.isParentUSB = (DeviceInfo.ParentDevInstanceId.compare(0, wstrUSB.length(), wstrUSB) == 0);
		ParseUsbVendorProductIdFromUsbParentId(wcsDeviceInstanceId, ParentVid, ParentPid);
	}

	memset(wcsDeviceInstanceId, 0, sizeof(wcsDeviceInstanceId));
	CM_Get_Device_ID(DeviceInfo.SelfDevInst, wcsDeviceInstanceId, 256, 0);
	DeviceInfo.SelfDeviceInstanceId = wcsDeviceInstanceId;
	ParseUsbVendorProductIdFromUsbParentId(wcsDeviceInstanceId, SelfVid, SelfPid);

	//有一种情况是父节点是Hub.子节点是设备，两者不是一个设备。所以VID与PID不同。
	if (SelfVid == 0 && SelfPid == 0)
	{
		DeviceInfo.UsbVid = ParentVid;
		DeviceInfo.UsbPid = ParentPid;
	}
	else
	{
		DeviceInfo.UsbVid = SelfVid;
		DeviceInfo.UsbPid = SelfPid;
	}
	int PortId = 0;
	int HubId = 0;
	if (DeviceInfo.isParentUSB)
	{
		GetUsbDeviceProductInfo(vectHubPathAndDevinst, DeviceInfo);
		GetSymbolicName(pDeviceInfoData->DevInst, DeviceInfo.wsDevicePath);

		//判断设备类型是不是HOST_CONTROLLER和USB_HUB，类似于设备树的root
		if (!DeviceInfo.wsDevicePath.empty())
		{
			DevicePathToGUID(DeviceInfo.wsDevicePath, DeviceInfo.devGuid);

			if (DeviceInfo.devGuid == GUID_DEVINTERFACE_USB_HOST_CONTROLLER || DeviceInfo.devGuid == GUID_DEVINTERFACE_USB_HUB)
			{
				return FALSE;
			}
		}
		GetCMPropertybyDevinst(DeviceInfo.ParentDevInst, CM_DRP_LOCATION_INFORMATION, wstrDeviceName);
		extractPortAndHub(wstrDeviceName, PortId, HubId);
		DeviceInfo.ParentDevHub = HubId;
		DeviceInfo.ParentDevPort = PortId;
	}

	if (TypeisFound || DeviceInfo.isParentUSB)
	{
		VectAllDevice.push_back(pDeviceInfo);
	}
	return TRUE;
}


/*
* @fn           extractPortAndHub
* @brief        从字符口中Port_#0009.Hub_#0001
* @param[in]    input  like is Port_#0009.Hub_#0001
* @param[out]   port
* @param[out]   hub
*
* @author       chatgpt
* @date         2023-12-29
*/
bool CWLUDisk::extractPortAndHub(const std::wstring& input, int& port, int& hub)
{
	wstring portStr = L"Port_#";
	wstring hubStr = L"Hub_#";
	int pos = input.find(portStr);
	if (pos != -1)
	{
		wstring portNum = input.substr(pos + portStr.length(), 4);
		port = _wtoi(portNum.c_str());
	}
	pos = input.find(hubStr);
	if (pos != -1)
	{
		wstring hubNum = input.substr(pos + hubStr.length(), 4);
		hub = _wtoi(hubNum.c_str());
	}
	return true;
}



BOOL CWLUDisk::GetDevicePathAndDevInst(LPGUID lpGuid, vector<DevPathAndDevInst>& vectDevInfo, wstring& strError, DWORD flags)
{
	HDEVINFO hDevInfo = 0;
	SP_DEVICE_INTERFACE_DATA  DeviceInterfaceData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
	SP_DEVINFO_DATA  DeviceInfoData = { sizeof(SP_DEVINFO_DATA) };
	DWORD dwRequiredSize = 0;
	DWORD dwIndex = 0;
	BOOL bRet = FALSE;
	DevPathAndDevInst DevInfo;
	//获取设备信息集句柄
	hDevInfo = SetupDiGetClassDevs(lpGuid, NULL, 0, flags);
	if (INVALID_HANDLE_VALUE == hDevInfo)
	{
		strError = L"SetupDiGetClassDevs failed";
		return FALSE;
	}

	for (dwIndex = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, lpGuid, dwIndex, &DeviceInterfaceData); dwIndex++)
	{
		//获取设备接口详细信息
		bRet = SetupDiGetDeviceInterfaceDetail(hDevInfo, &DeviceInterfaceData, NULL, 0, &dwRequiredSize, NULL);
		if (!bRet && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			strError = L"SetupDiGetDeviceInterfaceDetail failed";
			break;
		}
		//获取设备接口详细信息
		PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(dwRequiredSize);
		if (NULL == pDeviceInterfaceDetailData)
		{
			strError = L"malloc failed";
			break;
		}
		pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		bRet = SetupDiGetDeviceInterfaceDetail(hDevInfo, &DeviceInterfaceData, pDeviceInterfaceDetailData, dwRequiredSize, NULL, &DeviceInfoData);
		if (!bRet)
		{
			free(pDeviceInterfaceDetailData);
			strError = L"SetupDiGetDeviceInterfaceDetail failed";
			break;
		}
		DevInfo.DevInst = DeviceInfoData.DevInst;
		DevInfo.wstrDevicePath = pDeviceInterfaceDetailData->DevicePath;
		vectDevInfo.push_back(DevInfo);
		free(pDeviceInterfaceDetailData);
	}
	//关闭设备信息集句柄
	if (hDevInfo)
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		hDevInfo = NULL;
	}

	if (strError.empty())
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CWLUDisk::EnumAllDeviceFullInfo(vector<boost::shared_ptr<DeviceInfoFull>>& vecDeviceLastSnap)
{
	HDEVINFO hDevInfo = 0;
	SP_DEVINFO_DATA  DeviceInfoData = { sizeof(SP_DEVINFO_DATA) };

	vector<DevPathAndDevInst> vectHubDevBaseInfo;

	if (m_nWinVersion >= WIN_2008_SERVER_X32)
	{
		wstring strError;
		GUID usbHubDev = GUID_DEVINTERFACE_USB_HUB;
		GetDevicePathAndDevInst(&usbHubDev, vectHubDevBaseInfo, strError, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	}

	vecDeviceLastSnap.swap(g_vecDevice);

	hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES);//GUID_DEVINTERFACE_USB_DEVICE 参数无效。
	if (INVALID_HANDLE_VALUE == hDevInfo)
	{
		return EVT_FAIL;
	}

	for (DWORD DeviceId = 0; SetupDiEnumDeviceInfo(hDevInfo, DeviceId, &DeviceInfoData); DeviceId++)
	{
		//获取外设的完整消息<DeviceInfoFull> 
		GetDeviceFullInfo(hDevInfo, &DeviceInfoData, g_vecDevice, vectHubDevBaseInfo);
	}
	//关闭设备信息集句柄
	if (hDevInfo)
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		hDevInfo = NULL;
	}

	return TRUE;
}

void CWLUDisk::DealDeviceChangeMsg()
{
	vector<boost::shared_ptr<DeviceInfoFull>> vecDeviceLastSnap;
	EnumAllDeviceFullInfo(vecDeviceLastSnap);

	BuildDeviceRelation();

	//同步设备状态。完成子到父，父到子 的过程  - 递归。 （PS:小米无线鼠标）
	//EG:设备树的深度为3的情况下
	/*     E为忽略状态需要同步到ABCDEFG
					   A
					/  |  \
					B  C   D
					/  \   |
					E   F  G
	*/
	std::vector<boost::shared_ptr<DeviceInfoFull>>::iterator it;
	for (it = g_vecDevice.begin(); it != g_vecDevice.end(); ++it)
	{
		RsynUSBParentAndChlidignoreState(*(it->get()));
	}

	//遍历打印出g_vecDevice的信息
	for (it = g_vecDevice.begin(); it != g_vecDevice.end(); ++it)
	{
		DeviceInfoFull& DeviceInfo = *(it->get());
		if (DeviceInfo.isParentUSB)
		{
			WriteInfo(("VID: {} PID: {}  ProductInfo: {}  VendorInfo: {}"),
				DeviceInfo.UsbVid,
				DeviceInfo.UsbPid,
				CStrUtil::UnicodeToUTF8(DeviceInfo.wsProductInfo),
				CStrUtil::UnicodeToUTF8(DeviceInfo.wsVendorInfo));
		}
	}
}

CWLUDisk::CWLUDisk()
{
	//获取当前的计算机系统版本,存储在m_nWinVersion中
	CWindowsHelper::SeGetWindowsVersion(m_nWinVersion);

}

CWLUDisk::~CWLUDisk()
{
	if (NULL != m_instance)
	{
		delete m_instance;
		m_instance = nullptr;
	}
}

CWLUDisk* CWLUDisk::GetInstance()
{
	if (NULL == m_instance)
	{
		m_instance = new CWLUDisk();
	}
	return m_instance;
}

DWORD CWLUDisk::UnRegister()
{
	return 0;
}

IComponent* CWLUDisk::Register()
{
	return (IComponent*)GetInstance();
}

BOOL CWLUDisk::EnableFunction()
{
	CreatMonitorThread();
	return TRUE;
}

BOOL CWLUDisk::DisableFunction()
{
	StopMonitorThread();
	return TRUE;
}


void CWLUDisk::Destroy()
{
	if (NULL != m_instance)
	{
		delete m_instance;
		m_instance = nullptr;
	}
}


/*
* @fn           GetVUsbVendorProductIdFromUsbParentId
* @brief        从USB盘符的父设备名中解析出VID与PID。
* @param[in]    UsbDeviceId : //USB\VID_1058&PID_2623\57584E324537303541463932
				还可能是//USB\VID1058&PID2623\57584E324537303541463932 - Win11 USBHub
* @return       VID，PID号
*
* @detail
* @author       dan.liu
* @date         2021-6-1 新建。
*/
void CWLUDisk::ParseUsbVendorProductIdFromUsbParentId(wstring UsbDeviceId, ULONG& ulVendorId, ULONG& ulProductId)
{
	wstring			wstrRet = L"";

	transform(UsbDeviceId.begin(), UsbDeviceId.end(), UsbDeviceId.begin(), ::toupper);
	UsbDeviceId.erase(std::remove(UsbDeviceId.begin(), UsbDeviceId.end(), '_'), UsbDeviceId.end());

	const WCHAR* strVIDKey = L"VID";
	std::size_t found = UsbDeviceId.rfind(strVIDKey);
	if (found != std::wstring::npos)
	{
		found += wcslen(strVIDKey);
		wstrRet = UsbDeviceId.substr(found, 4);
		if (wstrRet.size() > 0)
		{
			ulVendorId = strtoul(CStrUtil::ConvertW2A(wstrRet).c_str(), NULL, 16);
		}
	}

	const WCHAR* strPIDKey = L"PID";
	found = UsbDeviceId.rfind(strPIDKey);
	if (found != std::wstring::npos)
	{
		found += wcslen(strPIDKey);
		wstrRet = UsbDeviceId.substr(found, 4);
		if (wstrRet.size() > 0)
		{
			ulProductId = strtoul(CStrUtil::ConvertW2A(wstrRet).c_str(), NULL, 16);
		}
	}

	return;

}


/*
* @fn           DevicePathToGUID
* @brief        根据SymbolicName获取outGuid
* @param[in]    SymbolicName	 \??\USB#Vid_0e0f&Pid_0003#6&63561a1&0&1#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
* @param[out]   outGuid			{A5DCBF10-6530-11D2-901F-00C04FB951ED}
* @return
* @date
*/
int CWLUDisk::DevicePathToGUID(wstring& SymbolicName, GUID& outGuid)
{
	const wchar_t* wstrDevicePath = SymbolicName.c_str();
	const wchar_t* startMarker = L"{";
	const wchar_t* endMarker = L"}";
	wchar_t extractedGuid[40]; // Assuming maximum possible GUID length is 38 characters

	// Find the position of the start marker '{'
	const wchar_t* startPos = wcsstr(wstrDevicePath, startMarker);
	if (startPos == NULL)
	{
		WriteError(("Start marker not found in the given string."));
		return 1;
	}

	// Find the position of the end marker '}' starting from the position of '{'
	const wchar_t* endPos = wcsstr(startPos, endMarker);
	if (endPos == NULL)
	{
		WriteError(("End marker not found in the given string."));
		return 1;
	}

	// Calculate the length of the extracted GUID
	size_t guidLength = endPos - startPos + 1;

	if (guidLength >= sizeof(extractedGuid) / sizeof(extractedGuid[0]))
	{
		WriteError(("GUID length exceeds the buffer size."));
		return 1;
	}

	// Copy the extracted GUID into a separate buffer
	wcsncpy_s(extractedGuid, startPos, guidLength);
	extractedGuid[guidLength] = L'\0'; // Null-terminate the GUID
	GUID guid;
	if (CLSIDFromString(extractedGuid, &guid) != S_OK)
	{
		WriteError(("Failed to convert string GUID to GUID structure."));
		return 1;
	}
	outGuid = guid;
	return 0;
}


/*
* @fn           buildDeviceRelation
* @brief		g_vecDeviceUSB 仅为USB设备以及相关的设备 建立父子关系。
* @param[in]
*
* @author       dan.liu
* @date         2023-11-8
*/
void CWLUDisk::BuildDeviceRelation()
{
	//找到当节点的子节点vector - 根据子节点的父节点ParentDevInst == 父节点的SelfDevInst （可能有多个子节点）
	//数据结构类似于多叉树  USB设备树
	std::vector<boost::shared_ptr<DeviceInfoFull>>::iterator it;
	for (it = g_vecDevice.begin(); it != g_vecDevice.end(); ++it)
	{
		DeviceInfoFull& USBDevInfo = *(it->get());
		std::vector<boost::shared_ptr<DeviceInfoFull>>::iterator it2;
		for (it2 = g_vecDevice.begin(); it2 != g_vecDevice.end(); ++it2)
		{
			DeviceInfoFull& ChildInfo = *(it2->get());
			if (!ChildInfo.isParentUSB)
			{
				continue;
			}

			if (ChildInfo.ParentDevInst == USBDevInfo.SelfDevInst)
			{
				USBDevInfo.vectChild.push_back(*it2);

				//叶子的父节点应该唯一且不为空
				if (ChildInfo.parent != NULL && ChildInfo.parent != *it)
				{
					WriteError(("ChildInfo.parent is NULL or ChildInfo.parent is not unique"));
				}
				ChildInfo.parent = *it;
				continue;
			}
		}
	}
}


void CWLUDisk::RsynUSBParentAndChlidignoreState(DeviceInfoFull& childDevInfo)
{
	if (!childDevInfo.vectChild.empty())
	{
		RsynUSBChildignoreState(childDevInfo);
	}
	if (childDevInfo.parent.get() == NULL)
	{
		return;
	}
	DeviceInfoFull& USBDeviceInfo = *(childDevInfo.parent.get());
	//子设备反向同步给USB设备。
	if (childDevInfo.bIgnore == TRUE && !USBDeviceInfo.bIgnore)
	{
		USBDeviceInfo.bIgnore = TRUE;
		if (USBDeviceInfo.iAction != DEVICE_KEEP)
		{
			USBDeviceInfo.iAction = DEVICE_KEEP;//此判断调试要用。以后给出打印信息。
		}
		RsynUSBParentAndChlidignoreState(USBDeviceInfo);
	}
}
/*

USB->hidclass->mouse. 三级对象。
			|->printer
			|->双因子KEY。

其中hidclass也不应该是ignore.

*/
/*
* @fn           RsynUSBChildignoreState
* @brief		同步ignore状态。 .其中有USB的CDROM，USB的手机，USB的wifi等等。
* @param[in]
* @param[in]
*
* @author       dan.liu
* @date         2023-11-8
*/
void CWLUDisk::RsynUSBChildignoreState(DeviceInfoFull& USBDeviceInfo)
{
	//DevStatus 0 表示禁用状态  1表示启用状态

	//ParseUsbVendorProductIdFromUsbParentId(USBDeviceInfo.SelfDeviceInstanceId,UsbVid,UsbPid);

	std::vector<boost::shared_ptr<DeviceInfoFull>>::iterator it;
	for (it = USBDeviceInfo.vectChild.begin(); it != USBDeviceInfo.vectChild.end(); ++it)
	{
		DeviceInfoFull& childDevInfo = *(it->get());
		DWORD dwdefailInfoPid = 0;
		DWORD dwdefailInfoVid = 0;

		//ParseUsbVendorProductIdFromUsbParentId(devInfo.ParentDevInstanceId,dwdefailInfoVid,dwdefailInfoPid);


		//step 1 同步"bIgnore"信息。父向子同步。
		if (USBDeviceInfo.bIgnore && !childDevInfo.bIgnore)
		{
			childDevInfo.bIgnore = TRUE;
			if (childDevInfo.iAction != DEVICE_KEEP)
			{
				childDevInfo.iAction = DEVICE_KEEP;//此判断调试要用。以后给出打印信息。
			}
			continue;
		}

		//设备树的深度 > 2 深度遍历
		if (!childDevInfo.vectChild.empty())
		{
			RsynUSBChildignoreState(childDevInfo);
		}
	}
}



LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DEVICECHANGE)
	{
		//设备插入和设备移除
		if (DBT_DEVICEARRIVAL == wParam || DBT_DEVICEREMOVECOMPLETE == wParam)
		{
			PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
			//检测设备是否是磁盘卷
			if (pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
			{
				PDEV_BROADCAST_VOLUME pVolume = (PDEV_BROADCAST_VOLUME)lParam;
				WriteInfo(("Volume changed,dbcv_size: {} dbcv_devicetype: {} dbcv_reserved: {}  dbcv_unitmask: {}  dbcv_flags: {}"),
					pVolume->dbcv_size, pVolume->dbcv_devicetype, pVolume->dbcv_reserved, pVolume->dbcv_unitmask, pVolume->dbcv_flags);
				CWLUDisk::GetInstance()->DealDeviceChangeMsg();
			}
			else if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
			{
				WriteInfo(("dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE"));
			}
		}
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


unsigned int WINAPI CWLUDisk::MonitorThread(LPVOID lpParameter)
{
	CWLUDisk* pUDisk = (CWLUDisk*)lpParameter;

	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = TEXT("VolumeChangeWnd");
	if (!RegisterClass(&wc))
	{
		WriteInfo(("RegisterClass faild"));
		return 0;
	}

	// 创建窗口并注册接收设备变更消息
	//g_SrvWnd = CreateWindow(wc.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

	g_SrvWnd = CreateWindowEx(0, TEXT("VolumeChangeWnd"), _T(""), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,       // Parent window    
		NULL,       // Menu
		GetModuleHandle(NULL),  // Instance handle
		NULL        // Additional application data
	);

	if (g_SrvWnd == NULL)
	{
		WriteInfo(("CreateWindow faild"));
		return 0;
	}

	for (int i = 0; i < sizeof(GUID_DEVINTERFACE_LIST) / sizeof(GUID); i++)
	{
		DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
		ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
		NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
		NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_LIST[i];
		g_DeviceNotifyHandle = RegisterDeviceNotification(g_SrvWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

		if (g_DeviceNotifyHandle == NULL)
		{
			WriteInfo(("RegisterDeviceNotification faild {}"), GetLastError());
			return 0;
		}
	}

	//TODO: GetMessage -> 通过PostQuitMessage(0);退出
	MSG msg;
	while (!g_StopMonitorThread)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Sleep(1000); // 休眠 100 毫秒
		}
	}
	
	PostQuitMessage(0);
	_endthreadex(0);
	return 0;
}

void CWLUDisk::CreatMonitorThread()
{
	m_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, NULL);
}

void CWLUDisk::StopMonitorThread()
{
	if (g_DeviceNotifyHandle != NULL)
	{
		UnregisterDeviceNotification(g_DeviceNotifyHandle);
		g_DeviceNotifyHandle = NULL;
	}

	if (g_SrvWnd != NULL)
	{
		DestroyWindow(g_SrvWnd);
		g_SrvWnd = NULL;
	}

	if (m_hMonitorThread != NULL)
	{
		CloseHandle(m_hMonitorThread);
		m_hMonitorThread = NULL;
	}

	//退出监控线程
	g_StopMonitorThread = TRUE;
}

DYNAMICLINKLIBRARY_API IComponent* GetComInstance()
{
	WriteInfo("Welcome to DeviceControl!");
	CWLUDisk* instance = CWLUDisk::GetInstance();
	return instance;
}