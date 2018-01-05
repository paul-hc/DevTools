// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once

#include "utl/targetver.h"

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_UUIDOF

#include <afxwin.h>							// MFC core and standard components
#include "utl/StdStl.h"						// Standard C++ Library
#include "utl/CommonWinDefs.h"				// min/max

#include <afxdisp.h>
#include <afxmt.h>

#include <comdef.h>
#include <comutil.h>

#include <atlbase.h>
#include <atlcom.h>
#include <afxcmn.h>

// include manifest for common controls
#pragma comment( linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"" )


#include "utl/CommonDefs.h"


// You may derive a class from CComModule and use it if you want to override
// something, but do not change the name of _Module
extern CComModule _Module;
