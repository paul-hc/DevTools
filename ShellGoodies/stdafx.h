// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_UUIDOF

#include "utl/utl_base.h"

#include <afxwin.h>					// MFC core and standard components
#include <afxdisp.h>
#include <afxmt.h>

#include <comdef.h>
#include <comutil.h>

#include <atlbase.h>
#include <atlcom.h>
#include <afxcmn.h>

#include "utl/UI/utl_ui.h"


#ifdef _DEBUG
	#define USE_UT		// no UT code in release builds
#endif // _DEBUG
