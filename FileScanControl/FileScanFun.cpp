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
    long long handle; //文件句柄
    size_t len = 0;
    size_t pos = 0;
    size_t find_ret = wstring::npos;
    wstring strTail;
    struct _wfinddata_t finder;           //文件信息的结构体
	/* win32 链接文件有4种
      类型
    1.快捷方式 （文件或目录）文件属性与普通文件相同，通过后缀.link识别 就是一种普通文件由explorer.exe进程解析/维护 非内核维护
    2.硬链接 （只能是文件） 就是普通文件，无法与真身区别 或者说每个都是真身 类型由内核维护
    3.软链接  （只能是目录）通过文件属性FILE_ATTRIBUTE_REPARSE_POINT识别，暂不知如何与符号链接区别 类型由内核维护
    4.符号链接 （文件或目录） 通过文件属性FILE_ATTRIBUTE_REPARSE_POINT识别，暂不知如何与软链接区别 类型由内核维护
    对于除硬链接之外 的全部不扫描！

    */
	//判断是否是.link文件
    len = wstrFolder.length();
    if (len > 6) /* 字符串".link/" 的长度 */
    {
        pos = len - 6;
    }

    /* 如果找到了 */
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

	handle = _wfindfirst(strFullMask.c_str(), &finder); //第一次查找
    if (-1 == handle)
    {
        return -1;
    }

    do
    {
        if (finder.attrib & _A_SUBDIR)  //如果是目录则递归;
        {
            if (0 == _tcscmp(finder.name, _T(".")) || 0 == _tcscmp(finder.name, _T("..")) )//如果是.或..则过滤;
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

            // skip system temp folder    //不能过滤掉临时目录，有些工业软件会释放文件到临时目录运行
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

	// 获取文件的属性
    DWORD fileAttributes = GetFileAttributes(wstrFilePath.c_str());
	if (fileAttributes == INVALID_FILE_ATTRIBUTES || (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
    {
        WriteError(("File not found or invalid"));
        goto END;
    }
	// 打开文件
    hFileHandle = CreateFile(wstrFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFileHandle == INVALID_HANDLE_VALUE)
    {
		WriteError(("Failed to open file"));
        goto END;
    }

    // 读取文件的前两个字节
    if (!ReadFile(hFileHandle, buffer, sizeof(buffer), &bytesRead, NULL) || bytesRead != sizeof(buffer)) 
    {
		WriteError(("Failed to ReadFile GetLasrError = {}"),GetLastError());
        goto END;
    }

    // 判断是否为 PE 文件
    if (buffer[0] == 'M' && buffer[1] == 'Z') 
    {
        bIsPEFile = TRUE;
    }

END:
    // 关闭文件句柄
    if (hFileHandle)
    {
		CloseHandle(hFileHandle);
    }
	return bIsPEFile;
}