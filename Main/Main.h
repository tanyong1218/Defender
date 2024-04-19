#pragma once
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <tchar.h>
#include <vector>
#include <userenv.h>
#include <LogHelper.h>
#include <WindowsHelper.h>
#include <future>
#include "IComponent.h"
using namespace std;

#define DEVICECONTROL _T("DeviceControl.dll")
#define SYSTEMLOGCONTROL _T("SystemLogControl.dll")

vector<wstring> g_LoadMoudleVector
{
	DEVICECONTROL,
	SYSTEMLOGCONTROL
};


typedef IComponent* (_cdecl* TESTDLL)();

std::vector<std::shared_ptr<IComponent*>> g_IComponentVector;


