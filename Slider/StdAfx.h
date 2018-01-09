// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#include "utl/targetver.h"

#include <afxwin.h>					// MFC core and standard components
#include <xutility>					// std::min, std::max

#include "utl/StdStl.h"				// standard C++ Library
#include "utl/CommonWinDefs.h"		// min/max

#include <afxext.h>         		// MFC extensions
#include <afxdisp.h>        		// MFC Automation classes
#include <afxdtctl.h>				// MFC support for Internet Explorer 4 Common Controls
#include <afxcmn.h>					// MFC support for Windows Common Controls

#include <afxmt.h>
#include <comdef.h>
#include <atlbase.h>				// For CComPtr<> and stuff


#include "utl/StdManifest.h"		// include manifest for common controls
#include "utl/CommonDefs.h"


#define PROF_SEP _T("|")
