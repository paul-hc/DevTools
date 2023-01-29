// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

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
