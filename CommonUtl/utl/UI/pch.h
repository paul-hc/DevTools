// pch.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "utl/utl_base.h"			// UTL_BASE library classes

#define STRICT_TYPED_ITEMIDS		// better type safety for IDLists

#include <afxwin.h>					// MFC core and standard components
#include <afxext.h>					// MFC extensions
#include <afxcmn.h>					// MFC support for Windows Common Controls

#define BUILD_UTL_UI

#include "CommonWinDefs.h"			// min/max


#ifdef _DEBUG
	#define USE_UT		// no UT code in release builds
#endif // _DEBUG
