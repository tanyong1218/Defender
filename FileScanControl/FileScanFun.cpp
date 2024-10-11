#include "FileScanFun.h"
#include <FileOperationHelper.h>


CFileScanFun::CFileScanFun()
{
	m_bStopSearch = FALSE;
}

CFileScanFun::~CFileScanFun()
{
	if (m_PeCacheHelper)
	{
		m_PeCacheHelper->PE_CACHE_Clear_AllCache();
		delete m_PeCacheHelper;
		m_PeCacheHelper = nullptr;
	}

}

BOOL CFileScanFun::EnableScanFileFunction()
{
	ThreadPool FileScanThreadPool(5);
	//获取所有逻辑驱动器
	DWORD drives = GetLogicalDrives();
	DWORD count = 0;
	wstring wstrDrive;
	if (!m_PeCacheHelper)
	{
		m_PeCacheHelper = new PECacheHelper();
	}
	
	for (TCHAR letter = 'A'; letter <= 'Z'; ++letter)
	{
		if ((drives & 1) == 1)
		{
			wchar_t FileName[MAX_PATH] = { 0 };
			swprintf_s(FileName, MAX_PATH, _T("%c:\\"), letter);
			FileScanThreadPool.enqueue([FileName]() {
				CFileScanFun File;
				File.GetFileListByFolder(wstring(FileName));
				});
		}
		drives >>= 1;
	}

	WriteInfo(("END"));
	return 0;
}

BOOL  CFileScanFun::GetFileListByFolder(const std::wstring wstrFolder)
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

		WriteInfo((" skip quick link path= {}"), CStrUtil::ConvertW2A(wstrFolder).c_str());
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
		WriteInfo((" skip soft/symbol link path= {}"), CStrUtil::ConvertW2A(wstrFolder).c_str());
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
			if (0 == _tcscmp(finder.name, _T(".")) || 0 == _tcscmp(finder.name, _T("..")))//如果是.或..则过滤;
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
			if (-1 != strCurFolder.find(_T("system volume information")))
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
				//获取PE文件的详细信息
				wstring wstrHashCode;
				ULONGLONG FileSize;
				ULONGLONG LastWriteTime;
				GetFileInfoEx(wstrFullFileName, wstrHashCode, FileSize, LastWriteTime);

				//保存到文件中
				string strFileName = CStrUtil::ConvertW2A(wstrFullFileName) + "Hash:" + CStrUtil::ConvertW2A(wstrHashCode);
				FileOperationHelper::SeWriteFile("FileScan.txt", strFileName, strFileName.size());

				if (!m_PeCacheHelper->PE_CACHE_Query_Cache(wstrFullFileName, FileSize, LastWriteTime, NULL, NULL, NULL))
				{
					m_PeCacheHelper->PE_CACHE_insert(wstrFullFileName, FileSize, LastWriteTime, wstrHashCode);
				}
				
			}
		}
		dwFileCount++;
	} while (!_wfindnext(handle, &finder) && !m_bStopSearch);

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
	hFileHandle = CreateFile(wstrFilePath.c_str(), GENERIC_ALL, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFileHandle == INVALID_HANDLE_VALUE)
	{
		WriteError(("Failed to open file  GetLasrError = {} filepath = {}"), GetLastError(), CStrUtil::ConvertW2A(wstrFilePath).c_str());
		goto END;
	}

	// 读取文件的前两个字节
	if (!ReadFile(hFileHandle, buffer, sizeof(buffer), &bytesRead, NULL) || bytesRead != sizeof(buffer))
	{
		WriteError(("Failed to ReadFile GetLasrError = {} filepath = {} "), GetLastError(),CStrUtil::ConvertW2A(wstrFilePath).c_str());
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


TCHAR* CFileScanFun::GetHashString(const unsigned char* pcSrc, DWORD dwSrcLen, TCHAR* pszDst, DWORD dwDstLen)
{
	if (dwSrcLen * 2 > dwDstLen)
	{
		return NULL;
	}

	for (int i = 0; i < dwSrcLen; i++)
	{
		swprintf_s(pszDst + 2 * i, dwDstLen - 2 * i, _T("%02X"), pcSrc[i]);
	}

	return pszDst;
}

BOOL CFileScanFun::GetFileInfoEx(const std::wstring wstrFullFileName, wstring& wstrHashCode, ULONGLONG& FileSize, ULONGLONG& LastWriteTime)
{
	unsigned char bHashCode[INTEGRITY_LENGTH] = { 0 };
	TCHAR szFileHash[INTEGRITY_LENGTH * 2 + 1] = { 0 };
	WIN32_FILE_ATTRIBUTE_DATA FileAttrData;

	if (!m_pefilevalidate.GetPEFileDegistByLib(wstrFullFileName.c_str(), bHashCode))
	{
		return FALSE;
	}

	GetHashString(bHashCode, sizeof(bHashCode), szFileHash, sizeof(szFileHash) / sizeof(TCHAR));
	if (!GetFileAttributesEx(wstrFullFileName.c_str(), GetFileExInfoStandard, &FileAttrData))
	{
		return FALSE;
	}

	ULARGE_INTEGER size;
	ULARGE_INTEGER WriteTime;
    size.HighPart = FileAttrData.nFileSizeHigh;
    size.LowPart  = FileAttrData.nFileSizeLow;
	
	WriteTime.HighPart = FileAttrData.ftLastWriteTime.dwHighDateTime;
	WriteTime.LowPart = FileAttrData.ftLastWriteTime.dwLowDateTime;

	FileSize = size.QuadPart;
	LastWriteTime = WriteTime.QuadPart;
	wstrHashCode = szFileHash;

	return TRUE;
}