// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#pragma once

#include "utl/targetver.h"
#include "utl/StdStl.h"		// standard C++ Library

#include <afx.h>

#define BUILD_UTL_BASE

#include "utl/CommonDefs.h"
#include "utl/StringBase.h"


#ifdef _DEBUG
	#define USE_UT		// no UT code in release builds
#endif // _DEBUG
