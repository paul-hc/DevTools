// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
#pragma once

#include "utl/targetver.h"

#include <afx.h>			// MFC core and standard components
#include "utl/StdStl.h"		// include after afxwin.h to prevent: uafxcw.lib(afxmem.obj) : error LNK2005: "void * __cdecl operator new[](unsigned int)" (??_U@YAPAXI@Z) already defined in libcpmt.lib(newaop.obj)

#include "utl/CommonDefs.h"

#ifdef _DEBUG
	/*
		Workaround for startup DLL loading error "The procedure entry point HIMAGELIST_QueryInterface could not be located in the dynamic link library COMCTL32.dll":
		Since the debug build does not discard unused code, all the UI dependencies remain linked in the executable, causing the DLL loading error on program startup.
			- including the manifest will pick the right version of comctl32.dll
		Release build does not have this problem, since it discards unused code - Linker > Optimization: References=Eliminate Unreferenced Data (/OPT:REF)
	*/
	#include "utl/StdManifest.h"		// include manifest for common controls (workaround comctl32.dll error on program startup)
#endif
