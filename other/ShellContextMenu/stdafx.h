#pragma once

#include "utl/targetver.h"

#include <afxwin.h>				// move it upper to prevent "uafxcw.lib(afxmem.obj) : error LNK2005: "void * __cdecl operator new[](unsigned int)" (??_U@YAPAXI@Z) already defined in libcpmt.lib(newaop.obj)"
#include "utl/StdStl.h"			// standard C++ Library
#include "utl/CommonWinDefs.h"	// min/max

#include <afxext.h>
#include <afxdisp.h>
#include <afxcmn.h>

#include "utl/StdManifest.h"	// include manifest for common controls
#include "utl/CommonDefs.h"