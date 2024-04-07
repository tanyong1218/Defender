#include "StringHelper.h"
CStrUtil::CStrUtil(void)
{
}

CStrUtil::~CStrUtil(void)
{
}

string CStrUtil::ConvertW2A(const wstring& wstr)
{
	setlocale(LC_ALL, ".936");
	size_t nSize = wstr.length() * 2 + 1;
	char* psz = new char[nSize];

	memset(psz, 0, nSize);
	size_t convertedChars = 0;
	wcstombs_s(&convertedChars, psz, nSize, wstr.c_str(), nSize);
	std::string str = psz;
	delete[]psz;
	return str;
}

wstring CStrUtil::ConvertA2W(const string& str)
{
	setlocale(LC_ALL, ".936");
	size_t nSize = str.length() + 1;
	wchar_t* wpsz = new wchar_t[nSize];

	memset(wpsz, 0, sizeof(wchar_t) * nSize);
	mbstowcs_s(&nSize, nullptr, 0, str.c_str(), 0);
	std::wstring wstr = wpsz;
	delete[]wpsz;
	return wstr;
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