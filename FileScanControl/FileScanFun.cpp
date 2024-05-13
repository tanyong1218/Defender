#include "FileScanFun.h"
#include <LogHelper.h>
				
CFileScanFun::CFileScanFun()
{
	m_bStopSearch = FALSE;
}

CFileScanFun::~CFileScanFun()
{
}

unsigned int __stdcall CFileScanFun::ScanFileThread(LPVOID lpParameter)
{
    CFileScanFun* pFileScanFun = (CFileScanFun*)lpParameter;
    pFileScanFun->GetFileListByFolder(L"C:\\");
    _endthreadex(0);
    return 0;
}

BOOL CFileScanFun::EnableScanFileFunction()
{
    std::wstring wstrFolder = L"C:\\";
    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, ScanFileThread, this, 0, NULL);
    return 0;
}

BOOL CFileScanFun::GetFileListByFolder(const std::wstring wstrFolder)
{
	std::wstring		strFullMask;
	std::wstring		wstrSubFolder;
	std::wstring		strCurFolder;
    std::string			strFileName;

	static DWORD 		dwFileCount = 0;

    static int cnt = 0;
    long long handle; //�ļ����
    size_t len = 0;
    size_t pos = 0;
    size_t find_ret = wstring::npos;
    wstring strTail;
    struct _wfinddata_t finder;           //�ļ���Ϣ�Ľṹ��
	/* win32 �����ļ���4��
      ����
    1.��ݷ�ʽ ���ļ���Ŀ¼���ļ���������ͨ�ļ���ͬ��ͨ����׺.linkʶ�� ����һ����ͨ�ļ���explorer.exe���̽���/ά�� ���ں�ά��
    2.Ӳ���� ��ֻ�����ļ��� ������ͨ�ļ����޷����������� ����˵ÿ���������� �������ں�ά��
    3.������  ��ֻ����Ŀ¼��ͨ���ļ�����FILE_ATTRIBUTE_REPARSE_POINTʶ���ݲ�֪���������������� �������ں�ά��
    4.�������� ���ļ���Ŀ¼�� ͨ���ļ�����FILE_ATTRIBUTE_REPARSE_POINTʶ���ݲ�֪��������������� �������ں�ά��
    ���ڳ�Ӳ����֮�� ��ȫ����ɨ�裡

    */
	//�ж��Ƿ���.link�ļ�
    len = wstrFolder.length();
    if (len > 6) /* �ַ���".link/" �ĳ��� */
    {
        pos = len - 6;
    }

    /* ����ҵ��� */
    strTail = wstrFolder.substr(pos, 6);
    if (!_tcsicmp(strTail.c_str(), _T(".link/")))
    {

        WriteInfo((" skip quick link path= {}"),CStrUtil::ConvertW2A(wstrFolder).c_str());
        return 0;
    }

    DWORD attr = GetFileAttributes(wstrFolder.c_str());
	if (attr == INVALID_FILE_ATTRIBUTES)
    {
        WriteInfo(("GetFileAttributes return attr is INVALID_FILE_ATTRIBUTES, Folder = {}"), CStrUtil::ConvertW2A(wstrFolder).c_str());
        return 0;
    }
	if (attr & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        WriteInfo((" skip soft/symbol link path= {}"),CStrUtil::ConvertW2A(wstrFolder).c_str());
        return 0;
    }
    else if (attr & FILE_ATTRIBUTE_DIRECTORY)
    {
        strFullMask = wstrFolder + std::wstring(_T("*.*"));
    }
    else
    {
        strFullMask = wstrFolder;
    }

	handle = _wfindfirst(strFullMask.c_str(), &finder); //��һ�β���
    if (-1 == handle)
    {
        return -1;
    }

    do
    {
        if (finder.attrib & _A_SUBDIR)  //�����Ŀ¼��ݹ�;
        {
            if (0 == _tcscmp(finder.name, _T(".")) || 0 == _tcscmp(finder.name, _T("..")) )//�����.��..�����;
                continue;

            wstrSubFolder = wstrFolder + finder.name + _T("\\");

            strCurFolder = wstrSubFolder;

            //if (m_pFileFinderProc != NULL && !m_bStopSearch)
            //	m_pFileFinderProc(this, FF_FOLDER, m_pCustomParam);

            // skip RECYCLER
            /*strCurFolder.MakeLower();*/
            transform(strCurFolder.begin(), strCurFolder.end(), strCurFolder.begin(), ::tolower);
            if (-1 != strCurFolder.find(_T("recycler")) || -1 != strCurFolder.find(_T("$recycle.bin")))
            {
                continue;
            }

            // skip system temp folder    //���ܹ��˵���ʱĿ¼����Щ��ҵ������ͷ��ļ�����ʱĿ¼����
            if (-1 != strCurFolder.find(_T("winnt\\temp")) || -1 != strCurFolder.find(_T("windows\\temp")))
            {
                continue;
            }
            // skip user temp folder
            if (-1 != strCurFolder.find(_T("local settings\\temp")) || -1 != strCurFolder.find(_T("local\\temp")))
            {
                continue;
            }

            // skip volume folder
            if (-1 != strCurFolder.find(_T("system volume information")) )
            {
                continue;
            }

            // Recursive call
            GetFileListByFolder(wstrSubFolder);
        }
        else
        {
            wstring wstrFullFileName = wstrFolder + finder.name;
            if (CheckIsPEFile(wstrFullFileName))
            {
				m_FileList.push_back(CStrUtil::ConvertW2A(wstrFullFileName));
                //WriteInfo(("PEFilePath = {}"), CStrUtil::ConvertW2A(wstrFullFileName));
            }
        }
		dwFileCount++;
    }while(!_wfindnext(handle, &finder) && !m_bStopSearch);


    _findclose(handle);

	return 0;
}

BOOL CFileScanFun::CheckIsPEFile(const std::wstring wstrFilePath)
{
    BOOL bIsPEFile = FALSE;
    HANDLE hFileHandle = nullptr;
    BYTE buffer[2];
    DWORD bytesRead;

	// ��ȡ�ļ�������
    DWORD fileAttributes = GetFileAttributes(wstrFilePath.c_str());
	if (fileAttributes == INVALID_FILE_ATTRIBUTES || (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
    {
        WriteError(("File not found or invalid"));
        goto END;
    }
	// ���ļ�
    hFileHandle = CreateFile(wstrFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFileHandle == INVALID_HANDLE_VALUE)
    {
		WriteError(("Failed to open file"));
        goto END;
    }

    // ��ȡ�ļ���ǰ�����ֽ�
    if (!ReadFile(hFileHandle, buffer, sizeof(buffer), &bytesRead, NULL) || bytesRead != sizeof(buffer)) 
    {
		WriteError(("Failed to ReadFile GetLasrError = {}"),GetLastError());
        goto END;
    }

    // �ж��Ƿ�Ϊ PE �ļ�
    if (buffer[0] == 'M' && buffer[1] == 'Z') 
    {
        bIsPEFile = TRUE;
    }

END:
    // �ر��ļ����
    if (hFileHandle)
    {
		CloseHandle(hFileHandle);
    }
	return bIsPEFile;
}