// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "utl/utl_base.h"

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <afxdisp.h>        // MFC OLE automation classes

#define BUILD_UTL_UI

#include "CommonWinDefs.h"	// min/max


#ifdef _DEBUG
	#define USE_UT		// no UT code in release builds
#endif // _DEBUG
