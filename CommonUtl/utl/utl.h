#ifndef utl_h
#define utl_h
#pragma once


// In Visual Studio 2013++ there is no UTL solution.
// Build the library in Visual Studio 2008 using utl/utl_vc9.sln.
// Include this header in Visual Studio 2013 project in stdafx.h to link to utl_ui_XX.lib


#ifndef _UNICODE
#error "(!) UTL libary can be used only in projects using UNICODE (wide encoding)."
#endif


#if defined( _WIN64 )		// x64 platform
	#ifdef _DEBUG
		#pragma comment( lib, "utl_ui_ud64.lib" )
	#else
		#pragma comment( lib, "utl_ui_u64.lib" )
	#endif
#else						// Win32 platform
	#ifdef _DEBUG
		#pragma comment( lib, "utl_ui_ud.lib" )
	#else
		#pragma comment( lib, "utl_ui_u.lib" )
	#endif
#endif


#endif // utl_h
