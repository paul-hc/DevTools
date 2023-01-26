
#include "stdafx.h"
#include "WindowInfoStore.h"
#include "WndUtils.h"
#include "AppService.h"
#include "utl/Guards.h"
#include "utl/UI/ProcessUtils.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWindowInfoStore::CWindowInfo implementation

CWindowInfoStore::CWindowInfo::CWindowInfo( const std::tstring& caption, HWND hWnd, HICON hIcon )
	: m_caption( caption )
	, m_briefInfo( wnd::FormatBriefWndInfo( hWnd, m_caption ) )
	, m_hIcon( hIcon )
{
}


// CWindowInfoStore implementation

double CWindowInfoStore::s_timeoutSecs = 0.5;

CWindowInfoStore& CWindowInfoStore::Instance( void )
{
	static CWindowInfoStore s_store;
	return s_store;
}

const CWindowInfoStore::CWindowInfo* CWindowInfoStore::FindInfo( HWND hWnd ) const
{
	ASSERT_PTR( hWnd );
	std::unordered_map<HWND, CWindowInfo>::const_iterator itCached = m_slowCache.find( hWnd );
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
	{
		CWindowInfo info( caption, hWnd, QueryWndIcon( hWnd ) );

		info.m_briefInfo += str::Format( _T("  GetWindowText<%.3f seconds>"), slowGuard.GetTimer().ElapsedSeconds() );
		m_slowCache[ hWnd ] = info;		// cache the caption/icon (including NULL icon)
	}

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
	{
		CWindowInfo info( ui::GetWindowText( hWnd ), hWnd, hIcon );

		info.m_briefInfo += str::Format( _T("  GetIcon<%.3f seconds>=0x%08X"), slowGuard.GetTimer().ElapsedSeconds(), info.m_hIcon );
		m_slowCache[ hWnd ] = info;			// cache the caption/icon (including NULL icon)
	}

	return hIcon;
}

HICON CWindowInfoStore::CacheIcon( HWND hWnd )
{
	utl::CSlowSectionGuard slowGuard( FormatContext( hWnd, _T("LookupIcon()") ), s_timeoutSecs );
	HICON hIcon = GetIcon( hWnd );
	m_slowCache[ hWnd ] = CWindowInfo( ui::GetWindowText( hWnd ), hWnd, hIcon );	// cache the caption/icon (including NULL icon)
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
