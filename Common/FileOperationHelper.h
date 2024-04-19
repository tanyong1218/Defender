#pragma once
#include <boost/filesystem.hpp>
#include <Windows.h>
#include <string.h>

namespace fs = boost::filesystem;
using namespace std;
class FileOperationHelper
{
public:
	FileOperationHelper() {};
	~FileOperationHelper() {};
public:
	static BOOL SeFileCopy(std::string& strDestFilePath, std::string& strSrcFilePath, BOOL bOverwrite, std::string& strError);
	static std::uintmax_t SeGetFileSize(const std::string& strFilePath, std::string& strError);

};
