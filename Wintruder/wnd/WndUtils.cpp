
#include "pch.h"
#include "WndUtils.h"
#include "WindowClass.h"
#include "WindowInfoStore.h"
#include "utl/StringUtilities.h"
#include "utl/UI/BaseApp.h"
#include "utl/UI/ProcessUtils.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace wnd
{
	bool HasUIPI( void )
	{
		static const bool s_hasUIPI = win::IsVersionOrGreater( win::Win8 );
		return s_hasUIPI;
	}

	bool HasSlowWindows( void )
	{
		return !CWindowInfoStore::Instance().IsEmpty();
	}

	bool IsSlowWindow( HWND hWnd )
	{
		return CWindowInfoStore::Instance().IsSlowWindow( hWnd );
	}


	std::tstring GetWindowText( HWND hWnd )
	{
		return CWindowInfoStore::Instance().LookupCaption( hWnd );
	}

	std::tstring GetWindowTextLine( HWND hWnd, size_t maxLen /*= 64*/ )
	{
		return FormatWindowTextLine( wnd::GetWindowText( hWnd ), maxLen );
	}

	HICON GetWindowIcon( HWND hWnd )
	{
		return CWindowInfoStore::Instance().LookupIcon( hWnd );
	}

	std::tstring FormatBriefWndInfo( HWND hWnd, const std::tstring& wndText )
	{
		std::tstring info; info.reserve( 128 );
		static const TCHAR s_sep[] = _T(" "), s_commaSep[] = _T(", ");

		stream::Tag( info, wnd::FormatWindowHandle( hWnd ), s_sep );

		if ( ui::IsValidWindow( hWnd ) )
		{
			stream::Tag( info, str::Format( _T("\"%s\" %s"), wnd::FormatWindowTextLine( wndText ).c_str(), wc::FormatClassName( hWnd ).c_str() ), s_sep );

			DWORD style = ui::GetStyle( hWnd );
			std::tstring status;

			if ( ui::IsTopMost( hWnd ) )
				stream::Tag( status, _T("Top-most"), s_commaSep );
			if ( !HasFlag( style, WS_VISIBLE ) )
				stream::Tag( status, _T("Hidden"), s_commaSep );
			if ( HasFlag( style, WS_DISABLED ) )
				stream::Tag( status, _T("Disabled"), s_commaSep );
			if ( IsSlowWindow( hWnd ) )
				stream::Tag( status, _T("Slow Access"), s_commaSep );

			if ( !status.empty() )
				stream::Tag( info, str::Format( _T("(%s)"), status.c_str() ), s_sep );
		}
		else
			stream::Tag( info, _T("<EXPIRED>"), s_sep );

		return info;
	}

	std::tstring FormatWindowTextLine( const std::tstring& text, size_t maxLen /*= 64*/ )
	{
		std::tstring outText = text;
		return str::SingleLine( outText, maxLen );
	}


	CRect GetCaptionRect( HWND hWnd )
	{	// returns the caption rect in client coordinates
		CRect captionRect;
		::GetWindowRect( hWnd, &captionRect );
		ui::ScreenToClient( hWnd, captionRect );

		DWORD style = ui::GetStyle( hWnd );
		int edgeExtent = ::GetSystemMetrics( HasFlag( style, WS_THICKFRAME ) ? SM_CXSIZEFRAME : SM_CYDLGFRAME );
		int captionExtent = ::GetSystemMetrics( HasFlag( ui::GetStyleEx( hWnd ), WS_EX_TOOLWINDOW ) ? SM_CYSMCAPTION : SM_CYCAPTION );

		//enum { MinMaxButtonWidth = 27, CloseButtonWidth = 47 };		// Win7 magic numbers
		// reliable on themed UI?
		NONCLIENTMETRICS ncm = { sizeof( NONCLIENTMETRICS ) };
		VERIFY( SystemParametersInfo( SPI_GETNONCLIENTMETRICS, sizeof( NONCLIENTMETRICS ), &ncm, 0 ) );
		ncm.iCaptionWidth; // 35

		int buttonsWidth = 0;
		if ( HasFlag( style, WS_SYSMENU ) )
			buttonsWidth += ncm.iCaptionWidth;
		if ( HasFlag( style, WS_MINIMIZEBOX ) )
			buttonsWidth += ncm.iCaptionWidth;
		if ( HasFlag( style, WS_MAXIMIZEBOX ) )
			buttonsWidth += ncm.iCaptionWidth;

		captionRect.DeflateRect( edgeExtent, edgeExtent, edgeExtent + captionExtent + buttonsWidth, 0 );
		captionRect.bottom = captionRect.top + captionExtent;
		return captionRect;
	}


	HWND GetTopLevelParent( HWND hWnd )
	{
		ASSERT( ::IsWindow( hWnd ) );
		HWND hTopLevelWnd = hWnd, hDesktopWnd = ::GetDesktopWindow();
		while ( hTopLevelWnd != nullptr && ui::IsChild( hTopLevelWnd ) )
		{
			HWND hParent = ::GetParent( hTopLevelWnd );
			if ( nullptr == hParent || hDesktopWnd == hParent )
				break;
			else
				hTopLevelWnd = hParent;
		}

		return hTopLevelWnd;
	}
}


namespace wnd
{
    bool ShowWindow( HWND hWnd, int cmdShow )
    {
		CScopedAttachThreadInput scopedThreadAccess( hWnd );
		bool result = ::ShowWindow( hWnd, cmdShow ) != FALSE;

		ui::RedrawWnd( hWnd );
		return result;
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
}
