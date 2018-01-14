
#include "stdafx.h"
#include "WndUtils.h"
#include "utl/BaseApp.h"
#include "utl/Guards.h"
#include "utl/StringUtilities.h"
#include <hash_map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace wnd
{
	std::tstring GetWindowText( HWND hWnd )
	{
		return wnd::CWindowInfoStore::Instance().LookupCaption( hWnd );
	}

	std::tstring FormatWindowTextLine( HWND hWnd, size_t maxLen /*= 64*/ )
	{
		std::tstring text = wnd::GetWindowText( hWnd );
		return str::SingleLine( text, maxLen );
	}

	HICON GetWindowIcon( HWND hWnd )
	{
		return wnd::CWindowInfoStore::Instance().LookupIcon( hWnd );
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


namespace wnd
{
	bool HasUIPI( void )
	{
		static const bool s_hasUIPI = win::IsVersionOrGreater( win::Win8 );
		return s_hasUIPI;
	}

	// CWindowInfoStore implementation

	double CWindowInfoStore::s_timeoutSecs = 0.5;

	CWindowInfoStore& CWindowInfoStore::Instance( void )
	{
		static CWindowInfoStore store;
		return store;
	}

	std::tstring CWindowInfoStore::LookupCaption( HWND hWnd )
	{
		if ( !ui::IsValidWindow( hWnd ) )
			return NULL;

		if ( const CWindowInfo* pCached = FindInfo( hWnd ) )
			return pCached->m_caption;

		// extract caption
		utl::CSlowSectionGuard slowGuard( FormatContext( hWnd, _T("LookupCaption()") ), s_timeoutSecs );
		std::tstring caption = ui::GetWindowText( hWnd );

		if ( slowGuard.IsTimeout() )		// slow window?
			m_slowCache[ hWnd ] = CWindowInfo( caption, ExtractIcon( hWnd ) );				// cache the caption/icon (including NULL icon)

		return caption;
	}

	HICON CWindowInfoStore::LookupIcon( HWND hWnd )
	{
		if ( !ui::IsValidWindow( hWnd ) )
			return NULL;

		if ( const CWindowInfo* pCached = FindInfo( hWnd ) )
			return pCached->m_hIcon;

		// extract icon
		utl::CSlowSectionGuard slowGuard( FormatContext( hWnd, _T("LookupIcon()") ), s_timeoutSecs );
		HICON hIcon = ExtractIcon( hWnd );

		if ( slowGuard.IsTimeout() )		// slow window?
			m_slowCache[ hWnd ] = CWindowInfo( ui::GetWindowText( hWnd ), hIcon );			// cache the caption/icon (including NULL icon)

		return hIcon;
	}

	const CWindowInfoStore::CWindowInfo* CWindowInfoStore::FindInfo( HWND hWnd ) const
	{
		ASSERT_PTR( hWnd );
		stdext::hash_map< HWND, CWindowInfo >::const_iterator itCached = m_slowCache.find( hWnd );
		return itCached != m_slowCache.end() ? &itCached->second : NULL;
	}

	std::tstring CWindowInfoStore::FormatContext( HWND hWnd, const TCHAR sectionName[] )
	{
		return str::Format( _T("%s for hWnd=%s sameElevation=%d"), sectionName, wnd::FormatWindowHandle( hWnd ).c_str(), utl::HasCurrentElevation( hWnd ) );
	}

	HICON CWindowInfoStore::ExtractIcon( HWND hWnd )
	{
		ASSERT( ::IsWindow( hWnd ) );
		if ( HasUIPI() )
			if ( ui::IsChild( hWnd ) )
				return NULL;				// don't even bother with icons for child windows

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
}
