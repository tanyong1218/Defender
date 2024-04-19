#pragma once
#include "CommonHeader.h"
class IComponent
{
public:
	virtual DWORD		 UnRegister()	   = 0;
	virtual IComponent*  Register()		   = 0;
	virtual BOOL		 EnableFunction()  = 0;
	virtual BOOL		 DisableFunction() = 0;
};
