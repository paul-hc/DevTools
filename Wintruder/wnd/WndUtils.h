#ifndef WndUtils_h
#define WndUtils_h
#pragma once

#include "utl/Utilities.h"


namespace wnd
{
	inline std::tstring FormatWindowHandle( HWND hWnd ) { return str::Format( _T("%08X"), hWnd ); }

	HICON GetIcon( HWND hWnd );
	HWND GetTopLevelParent( HWND hWnd );

	bool Activate( HWND hWnd );

	bool MoveWindowUp( HWND hWnd );
	bool MoveWindowDown( HWND hWnd );
	bool MoveWindowToTop( HWND hWnd );
	bool MoveWindowToBottom( HWND hWnd );
}


// attach this thread to hWnd's thread in order to acces the focused window, active window, etc

struct CScopedAttachThreadInput
{
	CScopedAttachThreadInput( HWND hWnd )
		: m_currThreadId( GetCurrentThreadId() )
		, m_wndThreadId( ::GetWindowThreadProcessId( hWnd, NULL ) )
		, m_attached( m_currThreadId != m_wndThreadId && ::AttachThreadInput( m_currThreadId, m_wndThreadId, TRUE ) != FALSE )
	{
		ASSERT_PTR( ::IsWindow( hWnd ) );
	}

	~CScopedAttachThreadInput()
	{
		if ( m_attached )
			::AttachThreadInput( m_currThreadId, m_wndThreadId, FALSE );		// un-attach this thread from hWnd's thread
	}
private:
	DWORD m_currThreadId;
	DWORD m_wndThreadId;
	bool m_attached;
};


#endif // WndUtils_h
