// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef pch_h
#define pch_h
//#pragma once


// add headers that you want to pre-compile here
#include "targetver.h"


#define _ATL_APARTMENT_THREADED

#include <afxwin.h>         // MFC core and standard components
#include "StdStl.h"			// Standard C++ Library

#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <afxdisp.h>        // MFC Automation classes

#include <atlbase.h>
#include <atlcom.h>


// generated with VC 2022:
#ifdef _UNICODE
	#if defined _M_IX86
		#pragma comment( linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"" )
	#elif defined _M_X64
		#pragma comment( linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"" )
	#else
		#pragma comment( linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"" )
	#endif
#endif


#include "Utilities.h"


#endif // pch_h
