#ifndef GdiPlus_fwd_h
#define GdiPlus_fwd_h
#pragma once

#pragma warning( push, 3 )			// switch to warning level 3
#pragma warning( disable: 4458 )	// warning C4458: declaration of 'nativeCap' hides class member

#include <gdiplus.h>

#pragma warning( pop )				// restore to the initial warning level


using namespace Gdiplus;


/*
	You have to initialize/uninitialize GDI+ outside of DLL and do it only once.
    GDI+ is not thread safe. So you should use it in main app thread only.

	This must be used in an EXE or regular DLL (never when called from DllMain).
	If using MFC with a CWinApp object, it should be created in InitInstance() and destroyed in ExitInstance().
*/
class CScopedGdiPlusInit
{
public:
	CScopedGdiPlusInit( void )
	{
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup( &m_gpToken, &gdiplusStartupInput, nullptr );		// initialize GDI+
	}

	~CScopedGdiPlusInit( void )
	{
		Gdiplus::GdiplusShutdown( m_gpToken );									// release GDI+
	}
private:
	ULONG_PTR m_gpToken;
};


#endif // GdiPlus_fwd_h
