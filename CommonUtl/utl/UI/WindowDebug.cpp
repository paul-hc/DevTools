
#include "stdafx.h"
#include "WindowDebug.h"
#include "SystemTray.h"
#include "TrayIcon.h"

#ifdef _DEBUG
#include "WndUtils.h"
#include "utl/FlagTags.h"
#include <sstream>
#define new DEBUG_NEW
#endif


namespace dbg
{
#ifdef _DEBUG
	const CFlagTags& GetStyleTags( void )
	{
		static const CFlagTags::FlagDef s_styleFlags[] =
		{
			FLAG_TAG( WS_CHILD ),
			FLAG_TAG( WS_VISIBLE ),
			FLAG_TAG( WS_DISABLED ),
			FLAG_TAG( WS_BORDER ),
			FLAG_TAG( WS_DLGFRAME ),
			FLAG_TAG( WS_THICKFRAME ),
			FLAG_TAG( WS_VSCROLL ),
			FLAG_TAG( WS_HSCROLL ),
			FLAG_TAG( WS_SYSMENU ),
			FLAG_TAG( WS_MINIMIZE ),
			FLAG_TAG( WS_MAXIMIZE )
		};
		static const CFlagTags s_styleTags( s_styleFlags, COUNT_OF( s_styleFlags ) );
		return s_styleTags;
	}
#endif

	void TraceWindow( HWND hWnd, const TCHAR tag[] )
	{
		hWnd, tag;
		TRACE( FormatWndInfo( hWnd, tag ).c_str() );
	}

	std::tstring FormatWndInfo( HWND hWnd, const TCHAR tag[] )
	{
	#ifdef _DEBUG
		static const TCHAR s_indent[] = _T("  ");

		std::tostringstream os;

		os << tag << std::endl;
		os << s_indent << _T("hWnd=") << str::Format( _T("0x%08X"), hWnd ) << std::endl;

		if ( ::IsWindow( hWnd ) )
		{
			if ( CWnd* pWnd = CWnd::FromHandlePermanent( hWnd ) )
				os << s_indent << _T("Type: ") << str::GetTypeName( typeid( *pWnd ) ) << std::endl;

			os << s_indent << _T("WndClassName=[") << ui::GetClassName( hWnd ) << _T(']') << std::endl;
			os << s_indent << _T("Text=\"") << ui::GetWindowText( hWnd ) << _T('"') << std::endl;

			UINT ctrlId = ::GetDlgCtrlID( hWnd );
			os << s_indent << str::Format( _T("ID=%d (0x%X)"), ctrlId, ctrlId ) << std::endl;

			DWORD style = ui::GetStyle( hWnd );
			os << s_indent << str::Format( _T("Style=0x%08X  "), style ) << GetStyleTags().FormatUi( style ) << std::endl;

			DWORD wndThreadId = ::GetWindowThreadProcessId( hWnd, nullptr ), currThreadId = ::GetCurrentThreadId();
			os << s_indent << str::Format( _T("WndThreadId=0x%X  CurrentThreadId=0x%X  "), wndThreadId, currThreadId ) << ( wndThreadId == currThreadId ? _T("(in current thread)") : _T("(in different thread)") ) << std::endl;

			CRect wndRect;
			::GetWindowRect( hWnd, &wndRect );
			os << s_indent << str::Format( _T("WndRect=(L=%d, T=%d) (R=%d, B=%d)  W=%d, H=%d"), wndRect.left, wndRect.top, wndRect.right, wndRect.bottom, wndRect.Width(), wndRect.Height() ) << std::endl;
		}
		else
			os << s_indent << _T("<not a valid window>") << std::endl;

		return os.str();
	#else
		hWnd, tag;
		return str::GetEmpty();
	#endif //_DEBUG
	}


	void TraceTrayNotifyCode( UINT msgNotifyCode )
	{
	#ifdef _DEBUG
		const TCHAR* pNotifyCode = nullptr;

		switch ( msgNotifyCode )
		{
			case WM_CONTEXTMENU: pNotifyCode = _T("WM_CONTEXTMENU"); break;
			case WM_LBUTTONDOWN: pNotifyCode = _T("WM_LBUTTONDOWN"); break;
			case WM_LBUTTONUP: pNotifyCode = _T("WM_LBUTTONUP"); break;
			case WM_LBUTTONDBLCLK: pNotifyCode = _T("WM_LBUTTONDBLCLK"); break;
			case WM_RBUTTONDOWN: pNotifyCode = _T("WM_RBUTTONDOWN"); break;
			case WM_RBUTTONUP: pNotifyCode = _T("WM_RBUTTONUP"); break;
			case WM_RBUTTONDBLCLK: pNotifyCode = _T("WM_RBUTTONDBLCLK"); break;
			case NIN_SELECT: pNotifyCode = _T("NIN_SELECT"); break;
			case NINF_KEY: pNotifyCode = _T("NINF_KEY"); break;
			case NIN_KEYSELECT: pNotifyCode = _T("NIN_KEYSELECT"); break;
			case NIN_BALLOONSHOW: pNotifyCode = _T("NIN_BALLOONSHOW"); break;
			case NIN_BALLOONHIDE: pNotifyCode = _T("NIN_BALLOONHIDE"); break;
			case NIN_BALLOONTIMEOUT: pNotifyCode = _T("NIN_BALLOONTIMEOUT"); break;
			case NIN_BALLOONUSERCLICK: pNotifyCode = _T("NIN_BALLOONUSERCLICK"); break;
			case NIN_POPUPOPEN: pNotifyCode = _T("NIN_POPUPOPEN"); break;
			case NIN_POPUPCLOSE: pNotifyCode = _T("NIN_POPUPCLOSE"); break;
			case WM_MOUSEMOVE:
				return;		// avoid noisy output
			default:
			{
				static TCHAR s_buffer[ 64 ];
				_itot( msgNotifyCode, s_buffer, 10 );
				pNotifyCode = s_buffer;
			}
		}
		static int count = 0;
		TRACE( _T(" OnTrayIconNotify(%d): NotifCode=%s\n"), count++, pNotifyCode );
	#else
		msgNotifyCode;
	#endif //_DEBUG
	}


	// CScopedTrayIconDiagnostics implementation

	CScopedTrayIconDiagnostics::CScopedTrayIconDiagnostics( const CTrayIcon* pTrayIcon, UINT msgNotifyCode )
		: m_pTrayIcon( pTrayIcon )
	{
	#ifdef _DEBUG
		if ( msgNotifyCode != WM_MOUSEMOVE )
		{
			m_preMsg = m_pTrayIcon->FormatState();
			TRACE( _T("  Pre {%s}\n"), m_preMsg.c_str() );
		}
	#else
		msgNotifyCode;
	#endif
	}

	CScopedTrayIconDiagnostics::~CScopedTrayIconDiagnostics()
	{
	#ifdef _DEBUG
		if ( !m_preMsg.empty() )
		{
			std::tstring postMsg = m_pTrayIcon->FormatState();
			if ( postMsg != m_preMsg )
				TRACE( _T("  Post {%s}\n"), postMsg.c_str() );
		}
	#endif
	}
}
