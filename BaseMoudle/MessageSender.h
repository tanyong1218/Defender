#pragma once
#include "CommonHeader.h"
#include "IPCMmf.h"

//////////////////////////////////////////////////////////////////////////
/*�ͻ�����ʾ������̵�IPCͨ�ŵ���Ϣ��*/
#define CLIENT_MSG_CODE_DEVICE_CONTROL							0x01	/**< �����������IPC�¼�ID*/
#define CLIENT_MSG_CODE_SYSTEMLOG_CONTROL						0x02	/**< ����ϵͳ��־IPC�¼�ID*/
#define CLIENT_MSG_CODE_FILESCAN_CONTROL						0x03	/**< ɨ���ļ�����IPC�¼�ID*/
/*<�������������Ϣ>���¼����� ��CLIENT_MSG_CODE_DEVICE_CONTROL_MESSAGECODE��*/
enum CLIENT_MSG_CODE_DEVICE_CONTROL_MESSAGECODE
{
	DEVICE_CONTROL_OPEN_ALL_FUNCTION = 1,												/**<  �������й��� */
	DEVICE_CONTROL_CLOSE_ALL_FUNCTION,												/**<  �ر����й��� */
};

/*<����ϵͳ��־������Ϣ>���¼����� ��CLIENT_MSG_CODE_DEVICE_CONTROL_MESSAGECODE��*/
enum CLIENT_MSG_CODE_SYSTEMLOG_CONTROL_MESSAGECODE
{
	SYSTEMLOG_CONTROL_OPEN_ALL_FUNCTION = 1,												/**<  �������й��� */
	SYSTEMLOG_CONTROL_CLOSE_ALL_FUNCTION,												/**<  �ر����й��� */
};

enum CLIENT_MSG_CODE_FILESCAN_CONTROL_MESSAGECODE
{
	FILESCAN_CONTROL_OPEN_ALL_FUNCTION = 1,												/**<  �������й��� */
	FILESCAN_CONTROL_CLOSE_ALL_FUNCTION,												/**<  �ر����й��� */
};

class CWLMessageSender
{
public:
	CWLMessageSender(void);
	~CWLMessageSender(void);

	/*!
	*  ���������������Ϣ
	* \param[in] dwMsgCode	  ��Ϣ��
	* \param[in] dwEventType  �¼�����
	* \param[in] dwDataSize   �¼����ݳ���(�ֽ���, in bytes)
	* \param[in] lpEventData  �¼�����
	* \return  ����ERROR_SUCCESS(��0)��ʾ�ɹ�����������ֵ��ʾʧ��(����ֵΪ������)��
	*/
	static DWORD SendMsg(DWORD dwEventType, DWORD dwMsgCode, DWORD dwDataSize, BYTE* lpEventData);
};
