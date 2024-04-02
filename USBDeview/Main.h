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
using namespace std;

const TCHAR DEVICECONTROL[] = _T("DeviceControl.dll");
typedef int(_cdecl* TESTDLL)();

