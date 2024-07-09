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
	std::wstring result(bufferSize - 1, L'\0');  // ¼õÈ¥ÖÕÖ¹·ûµÄ¿Õ¼ä
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