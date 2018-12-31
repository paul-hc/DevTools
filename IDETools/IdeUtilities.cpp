
#include "stdafx.h"
#include "IdeUtilities.h"
#include "TrackMenuWnd.h"
#include "Application.h"
#include "utl/EnumTags.h"
#include "utl/Path.h"
#include "utl/ProcessUtils.h"
#include "utl/Registry.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ide
{
	// CScopedWindow implementation

	CScopedWindow::CScopedWindow( void )
		: m_pFocusWnd( ide::GetFocusWindow() )
		, m_pMainWnd( ide::GetMainWindow( m_pFocusWnd ) )
		, m_hasDifferentThread( false )
		, m_ideType( ide::FindIdeType( m_pMainWnd ) )
	{
		if ( IsValid() )
			m_hasDifferentThread = ::GetWindowThreadProcessId( m_pFocusWnd->m_hWnd, NULL ) != ::GetCurrentThreadId();	// VC 7.1+: this COM object runs in a different thread than the text window

		DEBUG_LOG( _T("IDE: %s"), FormatInfo().c_str() );
	}

	CScopedWindow::~CScopedWindow()
	{
		RestoreFocusWnd();
	}

	bool CScopedWindow::RestoreFocusWnd( void )
	{
		HWND hWnd = m_pFocusWnd->GetSafeHwnd();
		if ( !::IsWindow( hWnd ) )
			return false;

		CScopedAttachThreadInput scopedThreadInput( hWnd );
		bool isFocused = ::GetFocus() == hWnd;

		if ( !isFocused )
		{
			::SetFocus( hWnd );

			isFocused = ::GetFocus() == hWnd;
		}

		return isFocused;
	}

	UINT CScopedWindow::TrackPopupMenu( CMenu& rMenu, CPoint screenPos, UINT flags /*= TPM_RIGHTBUTTON*/ )
	{
		if ( -1 == screenPos.x && -1 == screenPos.y )
			screenPos = ui::GetCursorPos();

		if ( !IsValid() )
			return 0;

		// VC 7.1 and up: this COM object runs in a different thread than the text window, therefore we need to create a tracking window in THIS thread.
		CTrackMenuWnd trackingWnd;
		VERIFY( trackingWnd.Create( m_pFocusWnd ) );

		UINT command = trackingWnd.TrackContextMenu( &rMenu, screenPos, flags );
		trackingWnd.DestroyWindow();

		return command;
	}

	std::tstring CScopedWindow::FormatInfo( void ) const
	{
		if ( !IsValid() )
			return _T("<Invalid IDE window>");

		return str::Format( _T("%s%s - FOCUS window: %s - MAIN window: %s"),
			GetTags_IdeType().FormatUi( m_ideType ).c_str(),
			m_hasDifferentThread ? _T(" (Different thread)") : _T(""),
			FormatWndInfo( m_pFocusWnd->GetSafeHwnd() ).c_str(),
			FormatWndInfo( m_pMainWnd->GetSafeHwnd() ).c_str() );
	}
	
	std::tstring CScopedWindow::FormatWndInfo( HWND hWnd )
	{
		if ( NULL == hWnd )
			return _T("<null-wnd>");

		DWORD style = ::GetWindowLong( hWnd, GWL_STYLE );
		CRect windowRect;
		::GetWindowRect( hWnd, &windowRect );

		static const TCHAR s_sep[] = _T(", ");
		std::tstring text = str::Format( _T("0x%08X [%s] \"%s\" style=0x%08X"),
			hWnd, ui::GetClassName( hWnd ).c_str(), ui::GetWindowText( hWnd ).c_str(), style );

		if ( HasFlag( style, WS_CHILD ) )
			stream::Tag( text, str::Format( _T("child_id=%d"), ui::ToCmdId( ::GetDlgCtrlID( hWnd ) ) ), s_sep );

		stream::Tag( text, str::Format( _T("pos(%d, %d), size(%d, %d)"), windowRect.left, windowRect.top, windowRect.Width(), windowRect.Height() ), s_sep );
		return text;
	}

	const CEnumTags& CScopedWindow::GetTags_IdeType( void )
	{
		static const CEnumTags s_tags( _T("VC 6.0|VC 7.1 to 9.0|VC 11+") );
		return s_tags;
	}
}


namespace ide
{
	IdeType FindIdeType( CWnd* pMainWnd /*= GetMainWindow()*/ )
	{
		if ( pMainWnd->GetSafeHwnd() != NULL )
		{
			std::tstring className = ui::GetClassName( pMainWnd->GetSafeHwnd() );

			if ( str::HasPrefix( className.c_str(), _T("Afx:") ) )
				return VC_60;
			else if ( className == _T("wndclass_desked_gsk") )
				return VC_71to90;
		}

		return VC_110plus;
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
			HWND s_hFocusWnds[] = { threadInfo.hwndCaret, threadInfo.hwndFocus, threadInfo.hwndCapture, threadInfo.hwndActive };

			for ( unsigned int i = 0; i != COUNT_OF( s_hFocusWnds ); ++i )
				if ( s_hFocusWnds[ i ] != NULL )
					return CWnd::FromHandle( s_hFocusWnds[ i ] );
		}

		HWND hWnds[] = { ::GetFocus(), ::GetActiveWindow() };

		for ( unsigned int i = 0; i != COUNT_OF( hWnds ); ++i )
			if ( hWnds[ i ] != NULL )
				return CWnd::FromHandle( hWnds[ i ] );

		return CWnd::GetForegroundWindow();
	}

	CWnd* GetMainWindow( CWnd* pStartingWnd /*= GetFocusWindow()*/ )
	{
		if ( NULL == pStartingWnd )
			pStartingWnd = CWnd::GetForegroundWindow();

		if ( pStartingWnd != NULL )
			pStartingWnd = ui::GetTopLevelParent( pStartingWnd );

		return pStartingWnd;
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

	std::pair< HMENU, int > FindPopupMenuWithCommand( HWND hWnd, UINT commandID )
	{
		ASSERT_PTR( hWnd );

		if ( HMENU hMenuIDE = ::GetMenu( hWnd ) )
			for ( UINT i = 0, count = ::GetMenuItemCount( hMenuIDE ); i != count; ++i )
			{
				if ( HMENU hSubMenu = ::GetSubMenu( hMenuIDE, i ) )
				{
					UINT itemState = ::GetMenuState( hSubMenu, commandID, MF_BYCOMMAND );
					if ( itemState != UINT_MAX )
						return std::make_pair( hSubMenu, i );
				}
			}

		return std::make_pair( (HMENU)NULL, -1 );
	}
}


namespace ide
{
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
