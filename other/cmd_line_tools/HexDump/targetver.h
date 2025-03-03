#pragma once

#include <sdkddkver.h>

#define VC_EXTRALEAN						// exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN					// exclude rarely-used stuff from Windows headers some more
#define NOMINMAX

#define _CRT_SECURE_NO_WARNINGS				// no use printf_s warnings
#define _SECURE_ATL 1
#define STRICT
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
#define _AFX_ALL_WARNINGS					// turns off MFC's hiding of some common and often safely ignored warning messages


/* { original code:

// The following macros define the minimum required platform.  The minimum required platform
// is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run 
// your application.  The macros work by enabling all features available on platform versions up to and 
// including the version specified.

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
#endif

   original code } */
