
#include "stdafx.h"
#include "IdeUtilities.h"
#include "Application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ide
{
	bool isVC6( void )
	{
		CWnd* pRootWindow = getRootWindow();

		return pRootWindow != NULL && getWindowClassName( pRootWindow->m_hWnd ).Left( 4 ) == _T("Afx:");
	}

	bool isVC71( void )
	{
		CWnd* pRootWindow = getRootWindow();

		return pRootWindow != NULL && getWindowClassName( pRootWindow->m_hWnd ) == _T("wndclass_desked_gsk");
	}

	CWnd* getRootWindow( void )
	{
		CWnd* pRootWindow = getFocusWindow();

		if ( pRootWindow == NULL )
			pRootWindow = CWnd::GetForegroundWindow();

		if ( pRootWindow != NULL )
			pRootWindow = getRootParentWindow( pRootWindow );

		if ( pRootWindow != NULL )
			DEBUG_LOG( _T("RootWindow: %s\n"), getWindowInfo( pRootWindow->m_hWnd ).c_str() );

		return pRootWindow;
	}

	CWnd* getFocusWindow( void )
	{
		GUITHREADINFO threadInfo =
		{
			sizeof( GUITHREADINFO ),
			GUI_CARETBLINKING | GUI_INMENUMODE | GUI_INMOVESIZE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE
		};

		if ( GetGUIThreadInfo( GetWindowThreadProcessId( GetForegroundWindow(), NULL ), &threadInfo ) )
		{
			HWND hFocusWnds[] = { threadInfo.hwndCaret, threadInfo.hwndFocus, threadInfo.hwndCapture, threadInfo.hwndActive };

			for ( unsigned int i = 0; i != COUNT_OF( hFocusWnds ); ++i )
				if ( hFocusWnds[ i ] != NULL )
					return CWnd::FromHandle( hFocusWnds[ i ] );
		}

		HWND hWnds[] = { GetFocus(), GetActiveWindow() };

		for ( unsigned int i = 0; i != COUNT_OF( hWnds ); ++i )
			if ( hWnds[ i ] != NULL )
				return CWnd::FromHandle( hWnds[ i ] );

		return CWnd::GetForegroundWindow();
	}

	CPoint getCaretScreenPos( void )
	{
		CPoint mousePointerPos;
		CWnd* pFocusWindow = getFocusWindow();

		if ( pFocusWindow != NULL )
		{
			if ( !GetCaretPos( &mousePointerPos ) )
			{
				CRect windowRect;

				pFocusWindow->GetWindowRect( &windowRect );
				mousePointerPos = windowRect.TopLeft();
				DEBUG_LOG( _T("NO_CARET!\n") );
			}
			else if ( pFocusWindow->GetStyle() & WS_CHILD )
				pFocusWindow->ClientToScreen( &mousePointerPos );

			DEBUG_LOG( _T("CaretPos [%d,%d]\n"), mousePointerPos.x, mousePointerPos.y );
		}
		else
			::GetCursorPos( &mousePointerPos );

		return mousePointerPos;
	}

	CPoint getMouseScreenPos( void )
	{
		CPoint mousePointerPos;

		GetCursorPos( &mousePointerPos );
		return mousePointerPos;
	}

	std::pair< HMENU, int > findPopupMenuWithCommand( HWND hWnd, UINT commandID )
	{
		ASSERT( hWnd != NULL );

		HMENU hMenuIDE = GetMenu( hWnd );

		if ( hMenuIDE != NULL )
			for ( UINT i = 0, count = GetMenuItemCount( hMenuIDE ); i < count; ++i )
			{
				HMENU hPopup = GetSubMenu( hMenuIDE, i );

				if ( hPopup != NULL )
				{
					UINT itemState = GetMenuState( hPopup, commandID, MF_BYCOMMAND );

					if ( itemState != UINT( -1 ) )
						return std::make_pair( hPopup, i );
				}
			}

		return std::make_pair( (HMENU)NULL, -1 );
	}

	CWnd* getRootParentWindow( CWnd* pWindow )
	{
		while ( pWindow != NULL )
			if ( pWindow->GetStyle() & WS_CHILD )
				pWindow = pWindow->GetParent();
			else
				break;

		return pWindow;
	}

	std::tstring getWindowInfo( HWND hWnd )
	{
		if ( hWnd == NULL )
			return _T("{NULL-WND}");

		TCHAR className[ 256 ];
		GetClassName( hWnd, className, COUNT_OF( className ) );

		TCHAR caption[ 256 ];
		GetWindowText( hWnd, caption, COUNT_OF( caption ) );

		UINT id = GetDlgCtrlID( hWnd );
		DWORD style = GetWindowLong( hWnd, GWL_STYLE );

		CRect windowRect;
		GetWindowRect( hWnd, &windowRect );

		return str::Format( _T("0x%08X {%s} '%s' id=%d, style=0x%08X, [%d,%d] - [%d,%d]"),
							hWnd, className, caption, id, style,
							windowRect.left, windowRect.top, windowRect.Width(), windowRect.Height() );
	}

	CString getWindowClassName( HWND hWnd )
	{
		ASSERT( IsWindow( hWnd ) );

		CString className;

		::GetClassName( hWnd, className.GetBuffer( 256 ), 256 );
		className.ReleaseBuffer();
		return className;
	}

	CString getWindowTitle( HWND hWnd )
	{
		CString title;

		ASSERT( IsWindow( hWnd ) );
		::GetWindowText( hWnd, title.GetBuffer( 256 ), 256 );
		title.ReleaseBuffer();
		return title;
	}

	int setFocusWindow( HWND hWnd )
	{
		ASSERT( IsWindow( hWnd ) );

		bool isFocused = ::GetFocus() == hWnd;

		if ( !isFocused )
		{
			DWORD currThreadID =::GetCurrentThreadId();
			DWORD wndThreadID =::GetWindowThreadProcessId( hWnd, NULL );
			bool differentThread = wndThreadID != currThreadID;

			if ( differentThread )
				VERIFY( ::AttachThreadInput( currThreadID, wndThreadID, TRUE ) );

			::SetFocus( hWnd );

			isFocused = ::GetFocus() == hWnd;

			if ( differentThread )
				VERIFY( ::AttachThreadInput( currThreadID, wndThreadID, FALSE ) );
		}

		return isFocused;
	}

	UINT trackPopupMenu( CMenu& rMenu, const CPoint& screenPos, CWnd* pWindow,
						 UINT flags /*= TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON*/ )
	{
		ASSERT( IsWindow( pWindow->GetSafeHwnd() ) );

		CWnd* pTrackingWindow = pWindow;

		// VC 7.1 and up: this COM object runs in a different thread than the text window, therefore
		// we need to create a tracking frame in this thread.
		if ( GetWindowThreadProcessId( pTrackingWindow->m_hWnd, NULL ) != GetCurrentThreadId() )
			pTrackingWindow = createThreadTrackingWindow( pTrackingWindow );

		UINT command = rMenu.TrackPopupMenu( flags, screenPos.x, screenPos.y, pTrackingWindow );

		if ( pTrackingWindow != pWindow )
			pTrackingWindow->DestroyWindow();	// this will also delete pTrackingWindow

		return command;
	}

	CWnd* createThreadTrackingWindow( CWnd* pParent )
	{
		CFrameWnd* pTrackingFrame = new CFrameWnd;

		VERIFY( pTrackingFrame->Create( NULL, NULL, WS_POPUP | WS_VISIBLE, CRect( 0, 0, 0, 0 ), pParent ) );
		return pTrackingFrame;
	}

} //namespace ide
