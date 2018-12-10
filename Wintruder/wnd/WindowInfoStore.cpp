
#include "stdafx.h"
#include "WindowInfoStore.h"
#include "WndUtils.h"
#include "AppService.h"
#include "utl/Guards.h"
#include "utl/ProcessUtils.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


double CWindowInfoStore::s_timeoutSecs = 0.5;

CWindowInfoStore& CWindowInfoStore::Instance( void )
{
	static CWindowInfoStore store;
	return store;
}

const CWindowInfoStore::CWindowInfo* CWindowInfoStore::FindInfo( HWND hWnd ) const
{
	ASSERT_PTR( hWnd );
	stdext::hash_map< HWND, CWindowInfo >::const_iterator itCached = m_slowCache.find( hWnd );
	return itCached != m_slowCache.end() ? &itCached->second : NULL;
}

std::tstring CWindowInfoStore::LookupCaption( HWND hWnd )
{
	if ( !ui::IsValidWindow( hWnd ) )
		return NULL;

	if ( const CWindowInfo* pCached = FindInfo( hWnd ) )
		return pCached->m_caption;

	utl::CSlowSectionGuard slowGuard( FormatContext( hWnd, _T("LookupCaption()") ), s_timeoutSecs );
	std::tstring caption = ui::GetWindowText( hWnd );

	if ( slowGuard.IsTimeout() )		// slow window access?
		m_slowCache[ hWnd ] = CWindowInfo( caption, QueryWndIcon( hWnd ) );				// cache the caption/icon (including NULL icon)

	return caption;
}

HICON CWindowInfoStore::LookupIcon( HWND hWnd )
{
	if ( !ui::IsValidWindow( hWnd ) )
		return NULL;

	if ( const CWindowInfo* pCached = FindInfo( hWnd ) )
		return pCached->m_hIcon;

	// cache icon only for slow windows
	utl::CSlowSectionGuard slowGuard( FormatContext( hWnd, _T("LookupIcon()") ), s_timeoutSecs );
	HICON hIcon = QueryWndIcon( hWnd );

	if ( slowGuard.IsTimeout() )			// slow window access?
		m_slowCache[ hWnd ] = CWindowInfo( ui::GetWindowText( hWnd ), hIcon );			// cache the caption/icon (including NULL icon)

	return hIcon;
}

HICON CWindowInfoStore::CacheIcon( HWND hWnd )
{
	utl::CSlowSectionGuard slowGuard( FormatContext( hWnd, _T("LookupIcon()") ), s_timeoutSecs );
	HICON hIcon = GetIcon( hWnd );
	m_slowCache[ hWnd ] = CWindowInfo( ui::GetWindowText( hWnd ), hIcon );	// cache the caption/icon (including NULL icon)
	return hIcon;
}

std::tstring CWindowInfoStore::FormatContext( HWND hWnd, const TCHAR sectionName[] )
{
	return str::Format( _T("%s for hWnd=%s sameElevation=%d"), sectionName, wnd::FormatWindowHandle( hWnd ).c_str(), proc::HasCurrentElevation( hWnd ) );
}

bool CWindowInfoStore::MustCacheIcon( HWND hWnd )
{
	switch ( app::GetOptions()->m_queryWndIcons )
	{
		case opt::NoWndIcons:
			return false;			// don't even bother with private window icons
		case opt::TopWndIcons:
			return ui::IsTopLevel( hWnd );
		default:
			ASSERT( false );
		case opt::AllWndIcons:
			return true;
	}
}

HICON CWindowInfoStore::QueryWndIcon( HWND hWnd )
{
	if ( !MustCacheIcon( hWnd ) )
		return NULL;

	return GetIcon( hWnd );
}

HICON CWindowInfoStore::GetIcon( HWND hWnd )
{
	ASSERT( ::IsWindow( hWnd ) );

	// icon access can deadlock on Windows 10 due to UIPI, for windows of processes having different elevation
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
