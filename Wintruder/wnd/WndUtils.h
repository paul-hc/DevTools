#ifndef WndUtils_h
#define WndUtils_h
#pragma once

#include "utl/Utilities.h"


namespace wnd
{
	bool HasUIPI( void );						// Windows 8+; expect slow window SendMessage access due to process isolation
	bool HasSlowWindows( void );
	bool IsSlowWindow( HWND hWnd );

	std::tstring GetWindowText( HWND hWnd );
	std::tstring FormatWindowTextLine( HWND hWnd, size_t maxLen = 64 );
	HICON GetWindowIcon( HWND hWnd );

	std::tstring FormatBriefWndInfo( HWND hWnd );
	inline std::tstring FormatWindowHandle( HWND hWnd ) { return str::Format( _T("%08X"), hWnd ); }
	CRect GetCaptionRect( HWND hWnd );

	HWND GetTopLevelParent( HWND hWnd );
}


namespace wnd
{
	// attached thread input
	bool ShowWindow( HWND hWnd, int cmdShow );
	bool Activate( HWND hWnd );
	bool MoveWindowUp( HWND hWnd );
	bool MoveWindowDown( HWND hWnd );
	bool MoveWindowToTop( HWND hWnd );
	bool MoveWindowToBottom( HWND hWnd );
}


#endif // WndUtils_h
