#pragma once
#include <windows.h>  
#include <tchar.h>  
#include <stdio.h>  
#include <strsafe.h>
#include <string>

using namespace std;

//µ¥ÀýÀàclass CopilotDemo
class CopilotDemo
{
public:
	static CopilotDemo* getInstance();
	~CopilotDemo();
}