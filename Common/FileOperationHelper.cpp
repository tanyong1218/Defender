#include "FileOperationHelper.h"

//����ȫ�������ļ�
BOOL FileOperationHelper::SeFileCopy(std::string& strDestFilePath, std::string& strSrcFilePath,BOOL bOverwrite,std::string &strError)
{
    try {
        fs::copy_file(strSrcFilePath, strDestFilePath, bOverwrite ? fs::copy_option::overwrite_if_exists:fs::copy_option::fail_if_exists);
        return TRUE;
    }
    catch (const fs::filesystem_error& ex) {
        strError = ex.what();
        return FALSE;
    }
}

// ��ȡ�ļ���С
std::uintmax_t FileOperationHelper::SeGetFileSize(const std::string& strFilePath,std::string& strError)
{
    try {
        return fs::file_size(strFilePath);
    }
    catch (const fs::filesystem_error& ex) {
        strError = ex.what();
        return 0;
    }
}
