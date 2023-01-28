// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_UUIDOF

#include "utl/utl_base.h"
#include "utl/UI/utl_ui.h"

#include <afxdisp.h>			// OLE Automation

#include <comutil.h>			// _bstr_t, _variant_t
#include <comdef.h>				// _com_error
#include <atlbase.h>
#include <atlcom.h>


#ifdef _DEBUG
	#define USE_UT		// no UT code in release builds
#endif // _DEBUG
