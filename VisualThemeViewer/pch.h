// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H
#pragma once

#include "utl/utl_base.h"			// UTL_BASE library classes
#include "utl/UI/utl_ui.h"			// UTL_UI library classes


// Extra MFC headers - not actually used in this project:
//#include <afxdisp.h>				// MFC Automation classes
//#include <afxdtctl.h>           	// MFC support for Internet Explorer 4 Common Controls
//#include <afxcontrolbars.h>     	// MFC support for ribbons and control bars


#ifdef _DEBUG
	#define USE_UT		// no UT code in release builds
#endif // _DEBUG


#endif //PCH_H
