// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 DYNAMICLINKLIBRARY_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// DYNAMICLINKLIBRARY_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef DYNAMICLINKLIBRARY_EXPORTS
#define DYNAMICLINKLIBRARY_API __declspec(dllexport)
#else
#define DYNAMICLINKLIBRARY_API __declspec(dllimport)
#endif

#define FMT_HEADER_ONLY
#include<boost/algorithm/string.hpp>
#include<boost/format.hpp>
#include <boost/program_options.hpp>
#include<boost/smart_ptr.hpp>
#include<boost/version.hpp>
#include <cfgmgr32.h>
#include <combaseapi.h>
#include <LogHelper.h>
#include <Setupapi.h>
#include <StringHelper.h>
#include <tchar.h>
#include <TimerHelper.h>
#include <vector>
#include <Windows.h>
#include <WindowsHelper.h>
#include <unordered_map>
#include <Dbt.h>
#include <winioctl.h>
#include "VndrList.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
using namespace std;

/////////////////////以下自己 COPY usbioctl.h中内容 /////////////
#pragma pack(1)
typedef struct _USB_NODE_CONNECTION_NAME {
	ULONG ConnectionIndex;
	ULONG ActualLength;
	WCHAR NodeName[1];
} USB_NODE_CONNECTION_NAME, * PUSB_NODE_CONNECTION_NAME;
//
//typedef struct _USB_NODE_INFORMATION {
//	USB_HUB_NODE NodeType;
//	union {
//		USB_HUB_INFORMATION       HubInformation;
//		USB_MI_PARENT_INFORMATION MiParentInformation;
//	} u;
//} USB_NODE_INFORMATION, *PUSB_NODE_INFORMATION;

#define FILE_DEVICE_UNKNOWN             0x00000022
#define FILE_DEVICE_USB					FILE_DEVICE_UNKNOWN


#define USB_GET_NODE_INFORMATION                    258
#define USB_GET_NODE_CONNECTION_INFORMATION         259
#define USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION     260
#define USB_GET_NODE_CONNECTION_NAME                261
#define USB_DIAG_IGNORE_HUBS_ON                     262
#define USB_DIAG_IGNORE_HUBS_OFF                    263
#define USB_GET_NODE_CONNECTION_DRIVERKEY_NAME      264
#define USB_GET_HUB_CAPABILITIES                    271
#define USB_GET_NODE_CONNECTION_ATTRIBUTES          272
#define USB_HUB_CYCLE_PORT                          273
#define USB_GET_NODE_CONNECTION_INFORMATION_EX      274
#define USB_RESET_HUB                               275
#define USB_GET_HUB_CAPABILITIES_EX                 276

#define IOCTL_USB_GET_NODE_CONNECTION_NAME    \
	CTL_CODE(FILE_DEVICE_USB,  \
	USB_GET_NODE_CONNECTION_NAME,  \
	METHOD_BUFFERED,  \
	FILE_ANY_ACCESS)

#define IOCTL_USB_GET_NODE_CONNECTION_INFORMATION  \
	CTL_CODE(FILE_DEVICE_USB,  \
	USB_GET_NODE_CONNECTION_INFORMATION,  \
	METHOD_BUFFERED,  \
	FILE_ANY_ACCESS)

#define IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION   \
	CTL_CODE(FILE_DEVICE_USB,  \
	USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,  \
	METHOD_BUFFERED,  \
	FILE_ANY_ACCESS)




typedef struct _USB_DEVICE_DESCRIPTOR {
	UCHAR  bLength;
	UCHAR  bDescriptorType;
	USHORT bcdUSB;
	UCHAR  bDeviceClass;
	UCHAR  bDeviceSubClass;
	UCHAR  bDeviceProtocol;
	UCHAR  bMaxPacketSize0;
	USHORT idVendor;
	USHORT idProduct;
	USHORT bcdDevice;
	UCHAR  iManufacturer;
	UCHAR  iProduct;
	UCHAR  iSerialNumber;
	UCHAR  bNumConfigurations;
} USB_DEVICE_DESCRIPTOR, * PUSB_DEVICE_DESCRIPTOR;

typedef enum _USB_CONNECTION_STATUS {
	NoDeviceConnected,
	DeviceConnected,
	DeviceFailedEnumeration,
	DeviceGeneralFailure,
	DeviceCausedOvercurrent,
	DeviceNotEnoughPower,
	DeviceNotEnoughBandwidth,
	DeviceHubNestedTooDeeply,
	DeviceInLegacyHub,
	DeviceEnumerating,
	DeviceReset
} USB_CONNECTION_STATUS, * PUSB_CONNECTION_STATUS;

typedef struct _USB_ENDPOINT_DESCRIPTOR {
	UCHAR  bLength;
	UCHAR  bDescriptorType;
	UCHAR  bEndpointAddress;
	UCHAR  bmAttributes;
	USHORT wMaxPacketSize;
	UCHAR  bInterval;
} USB_ENDPOINT_DESCRIPTOR, * PUSB_ENDPOINT_DESCRIPTOR;

typedef struct _USB_PIPE_INFO {
	USB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
	ULONG                   ScheduleOffset;
} USB_PIPE_INFO, * PUSB_PIPE_INFO;

typedef struct _USB_NODE_CONNECTION_INFORMATION {
	ULONG                 ConnectionIndex;
	USB_DEVICE_DESCRIPTOR DeviceDescriptor;
	UCHAR                 CurrentConfigurationValue;
	BOOLEAN               LowSpeed;
	BOOLEAN               DeviceIsHub;
	USHORT                DeviceAddress;
	ULONG                 NumberOfOpenPipes;
	USB_CONNECTION_STATUS ConnectionStatus;
	USB_PIPE_INFO         PipeList[0];
} USB_NODE_CONNECTION_INFORMATION, * PUSB_NODE_CONNECTION_INFORMATION;

typedef struct _USB_DESCRIPTOR_REQUEST {
	ULONG  ConnectionIndex;
	struct {
		UCHAR  bmRequest;
		UCHAR  bRequest;
		USHORT wValue;
		USHORT wIndex;
		USHORT wLength;
	} SetupPacket;
	UCHAR  Data[0];
} USB_DESCRIPTOR_REQUEST, * PUSB_DESCRIPTOR_REQUEST;


typedef struct _USB_STRING_DESCRIPTOR {
	UCHAR bLength;
	UCHAR bDescriptorType;
	WCHAR bString[1];
} USB_STRING_DESCRIPTOR, * PUSB_STRING_DESCRIPTOR;

#define USB_DEVICE_DESCRIPTOR_TYPE                0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE         0x02
#define USB_STRING_DESCRIPTOR_TYPE                0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE             0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE              0x05

// descriptor types defined by DWG documents
#define USB_RESERVED_DESCRIPTOR_TYPE              0x06
#define USB_CONFIG_POWER_DESCRIPTOR_TYPE          0x07
#define USB_INTERFACE_POWER_DESCRIPTOR_TYPE       0x08


const GUID GUID_DEVINTERFACE_USB_HUB = { 0xf18a0e88L, 0xc30c, 0x11d0, {0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8} };
const GUID GUID_DEVINTERFACE_USB_DEVICE = { 0xA5DCBF10L, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };
const GUID GUID_DEVINTERFACE_USB_HOST_CONTROLLER = { 0x3abf6f2dL, 0x71c4, 0x462a, { 0x8a, 0x92, 0x1e, 0x68, 0x61, 0xe6, 0xaf, 0x27 } };

typedef enum _enum_DEVICE_TYPE
{
	DEV_TYPE_CDROM = 0,
	DEV_TYPE_WLAN = 1,
	DEV_TYPE_BLUETOOTH = 2,
	DEV_TYPE_COM = 3,
	DEV_TYPE_LPT = 4,
	DEV_TYPE_FLOPPYDISK = 5,
	DEV_TYPE_WPD = 6,
	DEV_TYPE_USB = 7,
	DEV_OTHER_MAX = 8,
}ENUM_DEV_TYPE;

typedef struct DeviceInfoFull_st
{
	DeviceInfoFull_st() {
		isParentUSB = FALSE;
		ParentDevInst = 0;
		ParentDevHub = 0;
		ParentDevPort = 0;
		ParentDevInstanceId = _T("");
		UsbVid = 0;
		UsbPid = 0;
		SelfDevInst = 0;
		SelfDeviceInstanceId = _T("");
		SelfClassName = _T("");
		SelfClassType = DEV_OTHER_MAX;
		dwDevStatus = 0;
		dwProblem = 0;
		isInsert = 0;
		iAction = 0;//DEVICE_KEEP
		ActionMethod = 0;
		bIgnore = FALSE;
		wsDevicePath = _T("");
		memset(&devGuid, 0, sizeof(devGuid));
		wsVendorInfo = _T("");
		wsProductInfo = _T("");
	};

	//设备父节点信息
	DWORD		ParentDevInst;       //父节点设备实例标识符，句柄
	DWORD		ParentDevHub;		 //父节点为USB的设备的集线器和端口，
	DWORD		ParentDevPort;
	wstring     ParentDevInstanceId; //设备实例标ID，

	//设备信息
	DWORD				UsbVid;              //产品信息ID
	DWORD				UsbPid;				 //厂商ID 
	DWORD				SelfDevInst;         //设备实例标识符，句柄
	wstring				SelfDeviceInstanceId;//设备实例标ID，
	wstring				SelfClassName;		 //ClassName指的是usb、mouse、port等
	ENUM_DEV_TYPE		SelfClassType;       //ClassType指的是软盘、CDROME、并口等
	SP_DEVINFO_DATA		DeviceInfoData;		 //DeviceInfo里面有ClassGUID，DevInst，将作为WinAPI传参保存
	GUID				devGuid;			 //DeviceInterFaceGUID
	wstring				wsDevicePath;		 //其中有GUID。
	wstring				wsVendorInfo;
	wstring				wsProductInfo;

	//设备状态信息
	DWORD		dwDevStatus;				//CM_Get_DevNode_Status获取设备状态dwDevStatus 、dwProblem
	DWORD       dwProblem;
	BOOL		isInsert;					//设备是否插入		
	int			iAction;					//Action 设备将要进行的操作		
	int			ActionMethod;				//动作方法
	bool        bIgnore;					//Device是否忽略，（鼠标、键盘）
	BOOL		isParentUSB;				//父节点是不是USB


	//管理数据
	vector<boost::shared_ptr<struct DeviceInfoFull_st>> vectChild;
	boost::shared_ptr<struct DeviceInfoFull_st>		  parent;
}DeviceInfoFull, * PDeviceInfoFull;

enum WL_CLIENT_MSG_OTHER_RESULT
{
	EVT_SUC = 1,
	EVT_NO,
	EVT_FAIL
};

typedef struct DevPathAndDevInst_st
{
	wstring     wstrDevicePath;
	DWORD       DevInst;
	BOOL		isExist;  //表示当前设备是否插入到系统
} DevPathAndDevInst;


typedef struct _VENDOR_ID {
	USHORT  usVendorID;
	string  szVendor;
} VENDOR_ID, * PVENDOR_ID;

enum DeviceAction_st
{
	DEVICE_KEEP = 0,
	DEVICE_DISABLE = 1,
	DEVICE_ENABLE = 2,
};

static const GUID GUID_DEVINTERFACE_LIST[] =
{
	// GUID_DEVINTERFACE_USB_DEVICE
	{ 0xA5DCBF10, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } },
	// GUID_DEVINTERFACE_DISK
	{ 0x53f56307, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b } },
	// GUID_DEVINTERFACE_HID, 
	//{ 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } },
	// GUID_NDIS_LAN_CLASS
	//{ 0xad498944, 0x762f, 0x11d0, { 0x8d, 0xcb, 0x00, 0xc0, 0x4f, 0xc3, 0x35, 0x8c } }

	//GUID_DEVINTERFACE_COMPORT
	//{ 0x86e0d1e0, 0x8089, 0x11d0, { 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73 } },
	//GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR
	//{ 0x4D36E978, 0xE325, 0x11CE, { 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 } },
	//GUID_DEVINTERFACE_PARALLEL
	//{ 0x97F76EF0, 0xF883, 0x11D0, { 0xAF, 0x1F, 0x00, 0x00, 0xF8, 0x00, 0x84, 0x5C } },
	//GUID_DEVINTERFACE_PARCLASS
	//{ 0x811FC6A5, 0xF728, 0x11D0, { 0xA5, 0x37, 0x00, 0x00, 0xF8, 0x75, 0x3E, 0xD1 } }
};


vector<boost::shared_ptr<DeviceInfoFull>> g_vecDevice;

// 此类是从 dll 导出的
class DYNAMICLINKLIBRARY_API CWLUDisk {
public:

	~CWLUDisk();
	static CWLUDisk* GetInstance();
	static vector<wstring> m_stcDevInfo;

public:

	void DealDeviceChangeMsg();
	bool extractPortAndHub(const std::wstring& input, int& port, int& hub);
	BOOL GetDevicePathAndDevInst(LPGUID lpGuid, vector<DevPathAndDevInst>& vectDevInfo, wstring& strError, DWORD flags);
	BOOL EnumAllDeviceFullInfo(vector<boost::shared_ptr<DeviceInfoFull>>& vecDeviceLastSnap);
	boost::shared_ptr<unsigned char>	ptr;
	void GetSymbolicName(DEVINST devInst, wstring& strSymbolicName);
	wstring getVendorNameByVid(unsigned short VendorID);
	void getUsbDevProductInfo(vector<DevPathAndDevInst>& vectHubDevBaseInfo, DeviceInfoFull& USBDeviceInfo, wstring& wstrProductInfo);
	BOOL GetUsbDeviceDeviceName(const WCHAR* pszUSBHubDevicePath, int ConnectIndex, std::wstring& wstrUSBDeviceName, std::wstring& strError);
	BOOL HubGetNodeStringDescriptorProduct(HANDLE hHubDevice, ULONG ConnectIndex, USB_DEVICE_DESCRIPTOR& DevDescInfo, wstring& WstrDesc, std::wstring& strError);
	BOOL GetStringDescriptor(HANDLE hHubDevice, ULONG ConnectionIndex, UCHAR DescriptorIndex, USHORT LanguageID, wstring& WstrOutDesc, std::wstring& strError);
	BOOL HubGetNodeConnectionInformation(HANDLE hHubDevice, int ConnectIndex, USB_NODE_CONNECTION_INFORMATION& connectinfo, std::wstring& strError);
	void GetUsbDeviceProductInfo(vector<DevPathAndDevInst>& vectHubDevBaseInfo, DeviceInfoFull& USBDeviceInfo);
	void GetCMPropertybyDevinst(const DEVINST& devInst, DWORD cm_drp_flag, wstring& WstRet);
	BOOL GetDeviceFullInfo(
		HDEVINFO hDevInfo,
		PSP_DEVINFO_DATA pDeviceInfoData,
		vector<boost::shared_ptr<DeviceInfoFull>>& VectAllDevice,
		vector<DevPathAndDevInst>& vectHubPathAndDevinst
	);
	void ParseUsbVendorProductIdFromUsbParentId(wstring UsbDeviceId, ULONG& ulVendorId, ULONG& ulProductId);
	int DevicePathToGUID(wstring& SymbolicName, GUID& outGuid);
	void BuildDeviceRelation();
	void RsynUSBParentAndChlidignoreState(DeviceInfoFull& childDevInfo);
	void RsynUSBChildignoreState(DeviceInfoFull& USBDeviceInfo);

	static unsigned int	WINAPI MonitorThread(LPVOID lpParameter);

	void CreatMonitorThread();

private:

	CWLUDisk();
	int m_nWinVersion;
	HANDLE m_hMonitorThread;
	static CWLUDisk* m_instance;
	void Destroy();
};

extern "C" __declspec(dllexport) int EnableDeviceControl();

extern DYNAMICLINKLIBRARY_API int nDynamicLinkLibrary;

DYNAMICLINKLIBRARY_API int fnDynamicLinkLibrary(void);
