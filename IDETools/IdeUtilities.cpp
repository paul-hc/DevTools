
#include "pch.h"
#include "IdeUtilities.h"
#include "TrackMenuWnd.h"
#include "utl/EnumTags.h"
#include "utl/Logger.h"
#include "utl/Path.h"
#include "utl/UI/ProcessUtils.h"
#include "utl/Registry.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ide
{
	// CScopedWindow implementation

	CScopedWindow::CScopedWindow( void )
		: m_pFocusWnd( ide::GetFocusWindow() )
		, m_pMainWnd( ide::GetMainWindow( m_pFocusWnd ) )
		, m_hasDifferentThread( m_pFocusWnd != nullptr && proc::InDifferentThread( m_pFocusWnd->GetSafeHwnd() ) )	// VC 7.1+: this COM object runs in a different thread than the text window
		, m_ideType( ide::FindIdeType( m_pMainWnd ) )
	{
		LOG_TRACE( _T("IDE: %s"), FormatInfo().c_str() );
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

		::SetForegroundWindow( hWnd );

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
		if ( nullptr == hWnd )
			return _T("<null-wnd>");

		DWORD style = ::GetWindowLong( hWnd, GWL_STYLE );
		CRect windowRect;
		::GetWindowRect( hWnd, &windowRect );

		static const TCHAR s_sep[] = _T(", ");
		std::tstring text = str::Format( _T("0x%08X [%s] \"%s\" style=0x%08X"),
			hWnd, ui::GetClassName( hWnd ).c_str(), ui::GetWindowText( hWnd ).c_str(), style );

		if ( HasFlag( style, WS_CHILD ) )
			stream::Tag( text, str::Format( _T("child_id=%d"), ui::ToIntCmdId( ::GetDlgCtrlID( hWnd ) ) ), s_sep );

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
		if ( pMainWnd->GetSafeHwnd() != nullptr )
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

		if ( ::GetGUIThreadInfo( ::GetWindowThreadProcessId( ::GetForegroundWindow(), nullptr ), &threadInfo ) )
		{
			HWND s_hFocusWnds[] = { threadInfo.hwndCaret, threadInfo.hwndFocus, threadInfo.hwndCapture, threadInfo.hwndActive };

			for ( unsigned int i = 0; i != COUNT_OF( s_hFocusWnds ); ++i )
				if ( s_hFocusWnds[ i ] != nullptr )
					return CWnd::FromHandle( s_hFocusWnds[ i ] );
		}

		HWND hWnds[] = { ::GetFocus(), ::GetActiveWindow() };

		for ( unsigned int i = 0; i != COUNT_OF( hWnds ); ++i )
			if ( hWnds[ i ] != nullptr )
				return CWnd::FromHandle( hWnds[ i ] );

		return CWnd::GetForegroundWindow();
	}

	CWnd* GetMainWindow( CWnd* pStartingWnd /*= GetFocusWindow()*/ )
	{
		if ( nullptr == pStartingWnd )
			pStartingWnd = CWnd::GetForegroundWindow();

		if ( pStartingWnd != nullptr )
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
				LOG_TRACE( _T("NO_CARET!") );
			}
			else if ( pFocusWindow->GetStyle() & WS_CHILD )
				pFocusWindow->ClientToScreen( &mouseScreenPos );

			LOG_TRACE( _T("Mouse pos (%d,%d)"), mouseScreenPos.x, mouseScreenPos.y );
		}
		else
			::GetCursorPos( &mouseScreenPos );

		return mouseScreenPos;
	}

	std::pair<HMENU, int> FindPopupMenuWithCommand( HWND hWnd, UINT commandID )
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

		return std::make_pair( (HMENU)nullptr, -1 );
	}
}


namespace ide
{
	// VC registry access

	std::tstring GetRegistryPath_VC6( const TCHAR entry[] )
	{
		// Visual C++ 6 'Directories' key path
		static const fs::CPath s_regKeyPathVC6 = _T("Software\\Microsoft\\DevStudio\\6.0\\Build System\\Components\\Platforms\\Win32 (x86)\\Directories");
		std::tstring path;
		reg::CKey regKey;
		if ( regKey.Open( HKEY_CURRENT_USER, s_regKeyPathVC6, KEY_READ ) )
		{
			path = regKey.ReadStringValue( entry );
			LOG_TRACE( _T("VC6 %s: %s"), entry, path.c_str() );
		}
		else
			TRACE( _T("# Error accessing the registry: %s\n"), s_regKeyPathVC6.GetPtr() );

		return path;
	}

	std::tstring GetRegistryPath_VC71( const TCHAR entry[] )
	{
		// Visual C++ 7.1 'Directories' key path
		static const fs::CPath s_regKeyPathVC71 = _T("SOFTWARE\\Microsoft\\VisualStudio\\7.1\\VC\\VC_OBJECTS_PLATFORM_INFO\\Win32\\Directories");

		std::tstring path;
		reg::CKey regKey;
		if ( regKey.Open( HKEY_LOCAL_MACHINE, s_regKeyPathVC71, KEY_READ ) )
		{
			path = regKey.ReadStringValue( entry );
			const std::tstring& vc71InstallDir = GetVC71InstallDir();

			if ( !vc71InstallDir.empty() )
				str::Replace( path, _T("$(VCInstallDir)"), vc71InstallDir.c_str() );

			LOG_TRACE( _T("VC71 %s: %s"), entry, path.c_str() );
		}
		else
			TRACE( _T("# Error accessing the registry: %s\n"), s_regKeyPathVC71.GetPtr() );

		return path;
	}

	const std::tstring& GetVC71InstallDir( void )
	{
		static std::tstring s_vc71InstallDir;
		if ( s_vc71InstallDir.empty() )
		{
			reg::CKey regKey;
			if ( regKey.Open( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\VisualStudio\\7.1"), KEY_READ ) )
			{
				s_vc71InstallDir = regKey.ReadStringValue( _T("InstallDir") );
				str::Replace( s_vc71InstallDir, _T("Common7\\IDE\\"), _T("Vc7\\") );
			}
		}

		return s_vc71InstallDir;
	}


	namespace vs6
	{
		fs::CPath GetCommonDirPath( bool trailSlash /*= true*/ )
		{
			std::tstring vsCommonDirPath;

			reg::CKey key;
			if ( key.Open( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\VisualStudio\\6.0\\Setup"), KEY_READ ) )
			{
				vsCommonDirPath = key.ReadStringValue( _T("VsCommonDir") );
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
			std::tstring vsVC98DirPath;

			reg::CKey key;
			if ( key.Open( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\VisualStudio\\6.0\\Setup\\Microsoft Visual C++"), KEY_READ ) )
			{
				vsVC98DirPath = key.ReadStringValue( _T("ProductDir") );
				path::SetBackslash( vsVC98DirPath, trailSlash );
			}
			return vsVC98DirPath;
		}
	}
}
