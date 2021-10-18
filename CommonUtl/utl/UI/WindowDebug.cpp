
#include "stdafx.h"
#include "WindowDebug.h"

#ifdef _DEBUG
#include "Utilities.h"
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
	#ifdef _DEBUG
		static const TCHAR s_indent[] = _T("  ");

		std::tostringstream os;

		os << tag << std::endl;
		os << s_indent << _T("hWnd=") << str::Format( _T("0x%08X"), hWnd ) << std::endl;

		if ( ::IsWindow( hWnd ) )
		{
			os << s_indent << _T("WndClassName=[") << ui::GetClassName( hWnd ) << _T(']') << std::endl;
			os << s_indent << _T("Text=\"") << ui::GetWindowText( hWnd ) << _T('"') << std::endl;

			UINT ctrlId = ::GetDlgCtrlID( hWnd );
			os << s_indent << str::Format( _T("ID=%d (0x%X)"), ctrlId, ctrlId ) << std::endl;

			DWORD style = ui::GetStyle( hWnd );
			os << s_indent << str::Format( _T("Style=0x08%X  "), style ) << GetStyleTags().FormatUi( style ) << std::endl;

			DWORD wndThreadId = ::GetWindowThreadProcessId( hWnd, NULL ), currThreadId = ::GetCurrentThreadId();
			os << s_indent << str::Format( _T("WndThreadId=0x%X  CurrentThreadId=0x%X  "), wndThreadId, currThreadId ) << ( wndThreadId == currThreadId ? _T("(in current thread)") : _T("(in different thread)") ) << std::endl;

			CRect wndRect;
			::GetWindowRect( hWnd, &wndRect );
			os << s_indent << str::Format( _T("WndRect=(L=%d, T=%d) (R=%d, B=%d)  W=%d, H=%d"), wndRect.left, wndRect.top, wndRect.right, wndRect.bottom, wndRect.Width(), wndRect.Height() ) << std::endl;
		}

		TRACE( os.str().c_str() );
	#else
		hWnd, tag;
	#endif //_DEBUG
	}
}
