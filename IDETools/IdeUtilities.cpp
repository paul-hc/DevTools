
#include "stdafx.h"
#include "IdeUtilities.h"
#include "Application.h"
#include "utl/Path.h"
#include "utl/Registry.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ide
{
	bool IsVC6( void )
	{
		CWnd* pRootWindow = GetRootWindow();
		return pRootWindow != NULL && getWindowClassName( pRootWindow->m_hWnd ).Left( 4 ) == _T("Afx:");
	}

	bool IsVC_71to90( void )
	{
		CWnd* pRootWindow = GetRootWindow();
		return pRootWindow != NULL && getWindowClassName( pRootWindow->m_hWnd ) == _T("wndclass_desked_gsk");
	}

	IdeType FindIdeType( void )
	{
		if ( IsVC6() )
			return VC_60;
		if ( IsVC_71to90() )
			return VC_71to90;

		return VC_110plus;
	}

	CWnd* GetRootWindow( void )
	{
		CWnd* pRootWnd = GetFocusWindow();

		if ( pRootWnd == NULL )
			pRootWnd = CWnd::GetForegroundWindow();

		if ( pRootWnd != NULL )
			pRootWnd = ui::GetTopLevelParent( pRootWnd );

		if ( pRootWnd != NULL )
			DEBUG_LOG( _T("RootWindow: %s"), getWindowInfo( pRootWnd->m_hWnd ).c_str() );

		return pRootWnd;
	}

	CWnd* GetFocusWindow( void )
	{
		GUITHREADINFO threadInfo =
		{
			sizeof( GUITHREADINFO ),
			GUI_CARETBLINKING | GUI_INMENUMODE | GUI_INMOVESIZE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE
		};

		if ( ::GetGUIThreadInfo( ::GetWindowThreadProcessId( ::GetForegroundWindow(), NULL ), &threadInfo ) )
		{
			HWND hFocusWnds[] = { threadInfo.hwndCaret, threadInfo.hwndFocus, threadInfo.hwndCapture, threadInfo.hwndActive };

			for ( unsigned int i = 0; i != COUNT_OF( hFocusWnds ); ++i )
				if ( hFocusWnds[ i ] != NULL )
					return CWnd::FromHandle( hFocusWnds[ i ] );
		}

		HWND hWnds[] = { ::GetFocus(), ::GetActiveWindow() };

		for ( unsigned int i = 0; i != COUNT_OF( hWnds ); ++i )
			if ( hWnds[ i ] != NULL )
				return CWnd::FromHandle( hWnds[ i ] );

		return CWnd::GetForegroundWindow();
	}

	CPoint GetMouseScreenPos( void )
	{
		CPoint mouseScreenPos;

		if ( CWnd* pFocusWindow = GetFocusWindow() )
		{
			if ( !::GetCaretPos( &mouseScreenPos ) )
			{
				CRect windowRect;

				pFocusWindow->GetWindowRect( &windowRect );
				mouseScreenPos = windowRect.TopLeft();
				DEBUG_LOG( _T("NO_CARET!") );
			}
			else if ( pFocusWindow->GetStyle() & WS_CHILD )
				pFocusWindow->ClientToScreen( &mouseScreenPos );

			DEBUG_LOG( _T("Mouse pos (%d,%d)"), mouseScreenPos.x, mouseScreenPos.y );
		}
		else
			::GetCursorPos( &mouseScreenPos );

		return mouseScreenPos;
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


	// VC registry access

	std::tstring GetRegistryPath_VC6( const TCHAR entry[] )
	{
		// Visual C++ 6 'Directories' key path
		static const TCHAR regKeyPathVC6[] = _T("HKEY_CURRENT_USER\\Software\\Microsoft\\DevStudio\\6.0\\Build System\\Components\\Platforms\\Win32 (x86)\\Directories");
		std::tstring path;
		reg::CKey regKey( regKeyPathVC6, false );
		if ( regKey.IsValid() )
		{
			path = regKey.ReadString( entry );
			DEBUG_LOG( _T("VC6 %s: %s"), entry, path.c_str() );
		}
		else
			TRACE( _T("# Error accessing the registry: %s\n"), regKeyPathVC6 );

		return path;
	}

	std::tstring GetRegistryPath_VC71( const TCHAR entry[] )
	{
		// Visual C++ 7.1 'Directories' key path
		static const TCHAR regKeyPathVC71[] = _T("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\7.1\\VC\\VC_OBJECTS_PLATFORM_INFO\\Win32\\Directories");

		std::tstring path;
		reg::CKey regKey( regKeyPathVC71, false );

		if ( regKey.IsValid() )
		{
			path = regKey.ReadString( entry );
			const std::tstring& vc71InstallDir = GetVC71InstallDir();

			if ( !vc71InstallDir.empty() )
				str::Replace( path, _T("$(VCInstallDir)"), vc71InstallDir.c_str() );

			DEBUG_LOG( _T("VC71 %s: %s"), entry, path.c_str() );
		}
		else
			TRACE( _T("# Error accessing the registry: %s\n"), regKeyPathVC71 );

		return path;
	}

	const std::tstring& GetVC71InstallDir( void )
	{
		static std::tstring vc71InstallDir;
		if ( vc71InstallDir.empty() )
		{
			reg::CKey regKey( _T("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\7.1"), false );
			if ( regKey.IsValid() )
			{
				vc71InstallDir = regKey.ReadString( _T("InstallDir") );
				str::Replace( vc71InstallDir, _T("Common7\\IDE\\"), _T("Vc7\\") );
			}
		}

		return vc71InstallDir;
	}


	namespace vs6
	{
		fs::CPath GetCommonDirPath( bool trailSlash /*= true*/ )
		{
			reg::CKey key( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\VisualStudio\\6.0\\Setup") );
			std::tstring vsCommonDirPath;

			if ( key.IsValid() )
			{
				vsCommonDirPath = key.ReadString( _T("VsCommonDir") );
				path::SetBackslash( vsCommonDirPath, trailSlash );
			}
			return vsCommonDirPath;
		}

		fs::CPath GetMacrosDirPath( bool trailSlash /*= true*/ )
		{
			fs::CPath vsMacrosDirPath = GetCommonDirPath();
			if ( !vsMacrosDirPath.IsEmpty() )
			{
				vsMacrosDirPath /= fs::CPath( _T("MSDev98\\Macros") );
				vsMacrosDirPath.SetBackslash( trailSlash );
			}
			return vsMacrosDirPath;
		}

		fs::CPath GetVC98DirPath( bool trailSlash /*= true*/ )
		{
			reg::CKey key( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\VisualStudio\\6.0\\Setup\\Microsoft Visual C++") );
			//reg::CKey key( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\DevStudio\\6.0\\Products\\Microsoft Visual C++") );
			std::tstring vsVC98DirPath;

			if ( key.IsValid() )
			{
				vsVC98DirPath = key.ReadString( _T("ProductDir") );
				path::SetBackslash( vsVC98DirPath, trailSlash );
			}
			return vsVC98DirPath;
		}
	}
}
