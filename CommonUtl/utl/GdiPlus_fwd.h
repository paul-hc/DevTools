#ifndef GdiPlus_fwd_h
#define GdiPlus_fwd_h
#pragma once

#include <gdiplus.h>

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
		Gdiplus::GdiplusStartup( &m_gpToken, &gdiplusStartupInput, NULL );		// initialize GDI+
	}

	~CScopedGdiPlusInit( void )
	{
		Gdiplus::GdiplusShutdown( m_gpToken );									// release GDI+
	}
private:
	ULONG_PTR m_gpToken;
};


#endif // GdiPlus_fwd_h