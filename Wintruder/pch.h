// pch.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#include "utl/utl_base.h"

#include <afxwin.h>					// MFC core and standard components
#include <afxext.h>         		// MFC extensions
#include <afxdisp.h>        		// MFC Automation classes
#include <afxdtctl.h>				// MFC support for Internet Explorer 4 Common Controls
#include <afxcmn.h>					// MFC support for Windows Common Controls

#include "utl/UI/utl_ui.h"


#ifdef _DEBUG
	#define USE_UT		// no UT code in release builds
#endif // _DEBUG
