// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#pragma once

#include "utl/utl_base.h"

#include <afxwin.h>					// MFC core and standard components
#include <afxext.h>         		// MFC extensions
#include <afxdisp.h>        		// MFC OLE automation classes
#include <afxcmn.h>					// MFC support for Windows Common Controls

#include "utl/UI/utl_ui.h"


#ifdef _DEBUG
	#define USE_UT		// no UT code in release builds
#endif // _DEBUG


#define PROF_SEP _T("|")
#define EDIT_SEP _T(";")

#define ENTRY_OF( value ) _T( #value )
#define ENTRY_MEMBER( value ) ( _T( #value ) + 2 )		// skip past m_


enum ThreeState { False, True, Toggle = -1, };

typedef int TTernary;
