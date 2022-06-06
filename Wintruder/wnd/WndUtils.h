#ifndef WndUtils_h
#define WndUtils_h
#pragma once

#include "utl/UI/WndUtils.h"


namespace wnd
{
	bool HasUIPI( void );						// Windows 8+; expect slow window SendMessage access due to process isolation
	bool HasSlowWindows( void );
	bool IsSlowWindow( HWND hWnd );

	std::tstring GetWindowText( HWND hWnd );
	std::tstring GetWindowTextLine( HWND hWnd, size_t maxLen = 64 );
	HICON GetWindowIcon( HWND hWnd );

	std::tstring FormatBriefWndInfo( HWND hWnd, const std::tstring& wndText );		// slow windows: pass wndText already evaluated
	inline std::tstring FormatBriefWndInfo( HWND hWnd ) { return FormatBriefWndInfo( hWnd, ui::GetWindowText( hWnd ) ); }

	std::tstring FormatWindowTextLine( const std::tstring& text, size_t maxLen = 64 );
	inline std::tstring FormatWindowHandle( HWND hWnd ) { return str::Format( _T("%08X"), hWnd ); }

	CRect GetCaptionRect( HWND hWnd );

	HWND GetTopLevelParent( HWND hWnd );
}


namespace wnd
{
	inline HWND GetRealParent( HWND hWnd ) { ASSERT( ::IsWindow( hWnd ) ); return ::GetAncestor( hWnd, GA_PARENT ); }	// the real parent, NULL if desktop - ::GetParent( hWnd ) may return the owner

	// attached thread input
	bool ShowWindow( HWND hWnd, int cmdShow );
	bool Activate( HWND hWnd );
}


#endif // WndUtils_h
