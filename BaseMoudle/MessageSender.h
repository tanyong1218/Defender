#pragma once
#include "CommonHeader.h"
#include "IPCMmf.h"

//////////////////////////////////////////////////////////////////////////
/*客户端显示界面进程的IPC通信的消息码*/
#define CLIENT_MSG_CODE_DEVICE_CONTROL							0x01	/**< 外设防护功能IPC事件ID*/
#define CLIENT_MSG_CODE_SYSTEMLOG_CONTROL						0x02	/**< 操作系统日志IPC事件ID*/
#define CLIENT_MSG_CODE_FILESCAN_CONTROL						0x03	/**< 扫描文件功能IPC事件ID*/
/*<外设控制命令消息>的事件类型 （CLIENT_MSG_CODE_DEVICE_CONTROL_MESSAGECODE）*/
enum CLIENT_MSG_CODE_DEVICE_CONTROL_MESSAGECODE
{
	DEVICE_CONTROL_OPEN_ALL_FUNCTION = 1,												/**<  开启所有功能 */
	DEVICE_CONTROL_CLOSE_ALL_FUNCTION,												/**<  关闭所有功能 */
};

/*<操作系统日志命令消息>的事件类型 （CLIENT_MSG_CODE_DEVICE_CONTROL_MESSAGECODE）*/
enum CLIENT_MSG_CODE_SYSTEMLOG_CONTROL_MESSAGECODE
{
	SYSTEMLOG_CONTROL_OPEN_ALL_FUNCTION = 1,												/**<  开启所有功能 */
	SYSTEMLOG_CONTROL_CLOSE_ALL_FUNCTION,												/**<  关闭所有功能 */
};

enum CLIENT_MSG_CODE_FILESCAN_CONTROL_MESSAGECODE
{
	FILESCAN_CONTROL_OPEN_ALL_FUNCTION = 1,												/**<  开启所有功能 */
	FILESCAN_CONTROL_CLOSE_ALL_FUNCTION,												/**<  关闭所有功能 */
};

class CWLMessageSender
{
public:
	CWLMessageSender(void);
	~CWLMessageSender(void);

	/*!
	*  向主服务程序发送消息
	* \param[in] dwMsgCode	  消息码
	* \param[in] dwEventType  事件类型
	* \param[in] dwDataSize   事件数据长度(字节数, in bytes)
	* \param[in] lpEventData  事件数据
	* \return  返回ERROR_SUCCESS(即0)表示成功，返回其他值表示失败(返回值为错误码)。
	*/
	static DWORD SendMsg(DWORD dwEventType, DWORD dwMsgCode, DWORD dwDataSize, BYTE* lpEventData);
};
