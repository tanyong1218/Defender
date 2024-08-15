#pragma once
#include <boost/filesystem.hpp>
#include <Windows.h>
#include <string.h>
#include <unordered_map>
#include <tchar.h>
#include <fstream>
#include <filesystem>   //C++17

namespace fs = boost::filesystem;  //Boost¿â
using namespace std;
class FileOperationHelper
{
public:
	FileOperationHelper() {};
	~FileOperationHelper() {};
public:
	static BOOL SeFileCopy(std::string& strDestFilePath, std::string& strSrcFilePath, BOOL bOverwrite, std::string& strError);
	static std::uintmax_t SeGetFileSize(const std::string& strFilePath, std::string& strError);
	static BOOL SeEnumFile(const std::string strFileName, std::unordered_map<std::string, int>& FileMapViews);
	static BOOL SeWriteFile(const std::string strFilePath, const std::string strBuffer, unsigned int iLength);
};
