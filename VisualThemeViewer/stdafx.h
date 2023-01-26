
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#include "utl/utl_base.h"			// UTL_BASE library classes
#include "utl/UI/utl_ui.h"			// UTL_UI library classes

// MFC stuff not actually used in this project:
//#include <afxdisp.h>				// MFC Automation classes
//#include <afxdtctl.h>           	// MFC support for Internet Explorer 4 Common Controls
//#include <afxcontrolbars.h>     	// MFC support for ribbons and control bars


#ifdef _DEBUG
	#define USE_UT		// no UT code in release builds
#endif // _DEBUG
