// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#include "utl/targetver.h"

#include <afxwin.h>							// MFC core and standard components
#include "utl/StdStl.h"						// Standard C++ Library
#include "utl/CommonWinDefs.h"				// min/max

#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC OLE automation classes
#include <afxcmn.h>			// MFC support for Windows Common Controls


// include manifest for common controls
#pragma comment( linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"" )


// Warning Level 4 STL disables:
#pragma warning( disable: 4018 )			// signed/unsigned mismatch


#include "utl/CommonDefs.h"


#define PROF_SEP _T("|")
#define EDIT_SEP _T(";")

#define ENTRY_OF( value ) _T( #value )
#define ENTRY_MEMBER( value ) ( _T( #value ) + 2 )		// skip past m_


enum ThreeState { False, True, Toggle = -1, };

typedef int Ternary;


namespace str
{
	inline int Length( const TCHAR* pString ) { return static_cast< int >( _tcslen( pString ) ); }	// return int for convenience
}
