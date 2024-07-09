#pragma once
#ifndef __SCHEME_H__
#define __SCHEME_H__

// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>

#ifndef ArraySize
#define ArraySize(X)    ((int)(sizeof(X)/sizeof(X[0])))
#endif
#endif