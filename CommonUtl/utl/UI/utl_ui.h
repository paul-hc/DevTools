#ifndef utl_ui_h
#define utl_ui_h
#pragma once

#ifndef _UNICODE
	#error "(!) UTL libary can be used only in projects using UNICODE (wide encoding)."
#endif

#include "utl/utl_base.h"			// UTL_BASE library classes

#define STRICT_TYPED_ITEMIDS		// better type safety for IDLists

#include <afxwin.h>					// MFC core and standard components
#include <afxext.h>					// MFC extensions
#include <afxcmn.h>					// MFC support for Windows Common Controls

#include "utl/UI/CommonWinDefs.h"	// min/max
#include "utl/UI/StdManifest.h"		// include manifest for common controls


#endif // utl_ui_h
