#pragma once


#define VC_EXTRALEAN						// exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN					// exclude rarely-used stuff from Windows headers
#define NOMINMAX

#define _CRT_SECURE_NO_WARNINGS				// no use printf_s warnings
#define _SECURE_ATL 1
#define STRICT

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#define _AFX_ALL_WARNINGS					// turns off MFC's hiding of some common and often safely ignored warning messages


// Including SDKDDKVer.h defines the highest available Windows platform.
//   If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
//   set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <SDKDDKVer.h>
