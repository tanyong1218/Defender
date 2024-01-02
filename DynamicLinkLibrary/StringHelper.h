#pragma once
#include <string>
#include <vector>
#include <tchar.h>
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
};

#ifndef ArraySize
#define ArraySize(X)    ((int)(sizeof(X)/sizeof(X[0])))
#endif
