#pragma once
#include <windows.h>  
#include <tchar.h>  
#include <stdio.h>  
#include <strsafe.h>
#include <string>

using namespace std;

//������class CopilotDemo
class CopilotDemo
{
public:
	static CopilotDemo* getInstance();
	~CopilotDemo();
}