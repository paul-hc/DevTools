// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#include "utl/targetver.h"
#include "utl/StdStl.h"				// Standard C++ Library

#include <afxwin.h>					// MFC core and standard components
#include <afxext.h>         		// MFC extensions
#include <afxdisp.h>        		// MFC OLE automation classes
#include <afxcmn.h>					// MFC support for Windows Common Controls


#include "utl/CommonDefs.h"
#include "utl/UI/CommonWinDefs.h"		// min/max
#include "utl/UI/StdManifest.h"		// include manifest for common controls


#define PROF_SEP _T("|")
#define EDIT_SEP _T(";")

#define ENTRY_OF( value ) _T( #value )
#define ENTRY_MEMBER( value ) ( _T( #value ) + 2 )		// skip past m_


enum ThreeState { False, True, Toggle = -1, };

typedef int Ternary;
