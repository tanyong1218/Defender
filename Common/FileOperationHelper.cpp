#include "FileOperationHelper.h"

//（安全）拷贝文件
BOOL FileOperationHelper::SeFileCopy(std::string& strDestFilePath, std::string& strSrcFilePath, BOOL bOverwrite, std::string& strError)
{
	try {
		fs::copy_file(strSrcFilePath, strDestFilePath, bOverwrite ? fs::copy_option::overwrite_if_exists : fs::copy_option::fail_if_exists);
		return TRUE;
	}
	catch (const fs::filesystem_error& ex) {
		strError = ex.what();
		return FALSE;
	}
}

// 获取文件大小
std::uintmax_t FileOperationHelper::SeGetFileSize(const std::string& strFilePath, std::string& strError)
{
	try {
		return fs::file_size(strFilePath);
	}
	catch (const fs::filesystem_error& ex) {
		strError = ex.what();
		return 0;
	}
}

BOOL FileOperationHelper::SeEnumFile(const std::string strFileName, std::unordered_map<std::string, int>& FileMapViews)
{
	for (const auto& entry : std::filesystem::recursive_directory_iterator(strFileName))   //滚动遍历目录
	{
		if (entry.is_directory())
		{
			continue;
		}
		else if (entry.is_regular_file())
		{
			FileMapViews[entry.path().string()] = (int)entry.file_size();
		}
	}
	return TRUE;
}

BOOL FileOperationHelper::SeWriteFile(const std::string strFilePath, const std::string strBuffer, unsigned int iLength)
{
    fs::path filePath(strFilePath);

    if (!fs::exists(filePath)) 
	{
		std::fstream createFile(filePath.c_str());
    }

    std::fstream outputFile(filePath.string(), std::ios::out | std::ios::binary | std::ios::app);
    if (!outputFile.is_open()) {
       
        return FALSE;
    }

    outputFile.write(strBuffer.c_str(), iLength);
	outputFile << std::endl;
    outputFile.close();
    return TRUE;
}

BOOL FileOperationHelper::FetchXFromDir(std::list<std::wstring>& lstFiles, const std::wstring& strDir, const std::wstring& strSuffix)
{
	 std::wstring strFullPath;
    BOOL         bRet                = FALSE;
    std::wstring xstrSuffix          = strSuffix;
    std::wstring strPathWithWildcard = strDir;
    std::wstring strBasePath         = _T("");
    WIN32_FIND_DATA  stFindData      = { 0 };
    HANDLE           hFind           = INVALID_HANDLE_VALUE;

    if (strPathWithWildcard[strPathWithWildcard.length() - 1] != '\\')
    {
        strPathWithWildcard += _T("\\");
    }
    strBasePath = strPathWithWildcard;

    strPathWithWildcard += _T("*");
    strPathWithWildcard += xstrSuffix;

    hFind = FindFirstFile(strPathWithWildcard.c_str(), &stFindData);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        return bRet;
    }

    // 开始遍历（只遍历1层）
    lstFiles.clear();
    do 
    {
        if (0 == _tcsicmp(_T("."), stFindData.cFileName)
            ||0 ==  _tcsicmp(_T(".."), stFindData.cFileName))
        {
            continue;
        }

        strFullPath = strBasePath + stFindData.cFileName;
        lstFiles.push_back(strFullPath);

    } while (FindNextFile(hFind, &stFindData));

    bRet = TRUE;
}

