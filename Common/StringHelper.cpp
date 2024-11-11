#include "StringHelper.h"

CStrUtil::CStrUtil(void)
{
}

CStrUtil::~CStrUtil(void)
{
}

string CStrUtil::ConvertW2A(const wstring& wstr)
{
	int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string result(bufferSize, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], bufferSize, nullptr, nullptr);
	return result;
}

wstring CStrUtil::ConvertA2W(const string& str)
{
	int bufferSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	std::wstring result(bufferSize - 1, L'\0');  // 减去终止符的空间
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], bufferSize - 1);
	return result;
}

std::wstring CStrUtil::UTF8ToUnicode(std::string szAnsi)
{
	int nLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szAnsi.c_str(), -1, NULL, 0);
	WCHAR* wszAnsi = new WCHAR[nLen + 1];
	memset(wszAnsi, 0, sizeof(WCHAR) * (static_cast<unsigned long long>(nLen) + 1));
	nLen = MultiByteToWideChar(CP_UTF8, 0, szAnsi.c_str(), -1, wszAnsi, nLen + 1);
	std::wstring strRet;
	strRet = wszAnsi;
	delete[]wszAnsi;
	return strRet;
}

std::string CStrUtil::UnicodeToUTF8(std::wstring str)
{
	int nLen = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
	CHAR* szUtf8 = new CHAR[nLen + 1];
	memset(szUtf8, 0, static_cast<size_t>(nLen) + 1);
	nLen = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, szUtf8, nLen + 1, NULL, NULL);
	std::string strUtf8 = szUtf8;
	delete[]szUtf8;

	return strUtf8;
}

wstring CStrUtil::MacAddrToString(const unsigned char* pMac)
{
	char buf[64] = { 0 };
	_snprintf_s(buf, _countof(buf), _TRUNCATE, "%02X:%02X:%02X:%02X:%02X:%02X",
		pMac[0], pMac[1], pMac[2], pMac[3], pMac[4], pMac[5]);

	wstring strRet = _T("");
#ifdef _UNICODE
	string s1 = buf;
	wstring ws1 = ConvertA2W(s1);
	strRet = ws1.c_str();
#else
	strRet = buf;
#endif

	return strRet;
}

wstring CStrUtil::convertTimeTToStr(const time_t& time)
{
	std::wstring  wsTime;
	struct tm tm_time = {0};
	localtime_s(&tm_time, &time);//把UTC时间转成本地时间。所以在数据库中中保存的时间都是utc时间。
	TCHAR szTime[1024] = {0};
	_stprintf_s (szTime, _T("%4d-%02d-%02d %02d-%02d-%02d"), tm_time.tm_year + 1900,
		tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour,
		tm_time.tm_min, tm_time.tm_sec);
	wsTime = szTime;

	return wsTime;
}


std::string CStrUtil::StringToUTF8(const std::string &str)
{ 
    std::wstring ws;
    std::string retStr;

    ws = ConvertA2W(str);
    retStr = UnicodeToUTF8(ws);
    return retStr;
}