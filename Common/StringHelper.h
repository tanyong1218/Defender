#pragma once
#include <string>
#include <vector>
#include <tchar.h>
#include <sstream>
#include <time.h>
#include <Windows.h>
using std::string;
using std::wstring;

class CStrUtil
{
public:
	CStrUtil(void);
	~CStrUtil(void);

	static string ConvertW2A(const wstring& wstr);
	static wstring ConvertA2W(const string& str);
	static std::wstring UTF8ToUnicode(std::string szAnsi);
	static std::string UnicodeToUTF8(std::wstring str);
	static wstring MacAddrToString(const unsigned char* pMac);
	static wstring convertTimeTToStr(const time_t& time);
	static std::string StringToUTF8(const std::string& str);
};

#ifndef ArraySize
#define ArraySize(X)    ((int)(sizeof(X)/sizeof(X[0])))
#endif
