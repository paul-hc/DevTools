
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#include "utl/targetver.h"

#include <afxwin.h>							// MFC core and standard components
#include "utl/StdStl.h"						// Standard C++ Library
#include "utl/CommonWinDefs.h"				// min/max

#include <afxext.h>				// MFC extensions
#include <afxdisp.h>			// MFC Automation classes
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#include <afxcmn.h>             // MFC support for Windows Common Controls
#include <afxcontrolbars.h>     // MFC support for ribbons and control bars


#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif


#include "utl/CommonDefs.h"
