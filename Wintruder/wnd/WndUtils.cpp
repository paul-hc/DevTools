
#include "stdafx.h"
#include "WndUtils.h"
#include <hash_map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace wnd
{
	HICON GetIcon( HWND hWnd )
	{
		ASSERT( IsWindow( hWnd ) );

		HICON hIcon = (HICON)::SendMessage( hWnd, WM_GETICON, ICON_SMALL, 0 );
		if ( NULL == hIcon )
			hIcon = (HICON)::SendMessage( hWnd, WM_GETICON, ICON_SMALL2, 0 );
		if ( NULL == hIcon )
			hIcon = (HICON)::SendMessage( hWnd, WM_GETICON, ICON_BIG, 0 );

		if ( NULL == hIcon )
			hIcon = (HICON)::GetClassLongPtr( hWnd, GCLP_HICONSM );
		if ( NULL == hIcon )
			hIcon = (HICON)::GetClassLongPtr( hWnd, GCLP_HICON );

		return hIcon;
	}

	HWND GetTopLevelParent( HWND hWnd )
	{
		ASSERT( ::IsWindow( hWnd ) );
		HWND hTopLevelWnd = hWnd, hDesktopWnd = ::GetDesktopWindow();
		while ( hTopLevelWnd != NULL && ui::IsChild( hTopLevelWnd ) )
		{
			HWND hParent = ::GetParent( hTopLevelWnd );
			if ( NULL == hParent || hDesktopWnd == hParent )
				break;
			else
				hTopLevelWnd = hParent;
		}

		return hTopLevelWnd;
	}

	bool Activate( HWND hWnd )
	{
		if ( HWND hTopLevelWnd = GetTopLevelParent( hWnd ) )
		{
			if ( ::IsIconic( hTopLevelWnd ) )
				::SendMessage( hTopLevelWnd, WM_SYSCOMMAND, SC_RESTORE, 0 );

			CScopedAttachThreadInput scopedThreadAccess( hTopLevelWnd );
			::SetActiveWindow( hTopLevelWnd );
			::SetForegroundWindow( hTopLevelWnd );
			return true;
		}
		return false;
	}

	bool MoveWindowUp( HWND hWnd )
	{
		if ( HWND hPrevious = ::GetWindow( hWnd, GW_HWNDPREV ) )
			if ( ( hPrevious = ::GetWindow( hPrevious, GW_HWNDPREV ) ) != NULL )
				return ::SetWindowPos( hWnd, hPrevious, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE ) != FALSE;

		return MoveWindowToTop( hWnd );
	}

	bool MoveWindowDown( HWND hWnd )
	{
		if ( HWND hNext = ::GetWindow( hWnd, GW_HWNDNEXT ) )
			return ::SetWindowPos( hWnd, hNext, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE ) != FALSE;

		return false;
	}

	bool MoveWindowToTop( HWND hWnd )
	{
		return ::SetWindowPos( hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE ) != FALSE;
	}

	bool MoveWindowToBottom( HWND hWnd )
	{
		return ::SetWindowPos( hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE ) != FALSE;
	}
}
