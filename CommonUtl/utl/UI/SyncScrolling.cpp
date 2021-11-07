
#include "stdafx.h"
#include "SyncScrolling.h"
#include "WindowHook.h"
#include "ContainerUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


class CThumbTrackScrollHook : public CWindowHook
{
public:
	CThumbTrackScrollHook( CSyncScrolling* pOwner, CWnd* pCtrl ) : CWindowHook( true ), m_pOwner( pOwner ) { HookWindow( pCtrl->GetSafeHwnd() ); }
private:
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
private:
	CSyncScrolling* m_pOwner;
};


LRESULT CThumbTrackScrollHook::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	LRESULT result = CWindowHook::WindowProc( message, wParam, lParam );
	if ( WM_VSCROLL == message || WM_HSCROLL == message )
		if ( SB_THUMBTRACK == LOWORD( wParam ) )
			m_pOwner->Synchronize( CWnd::FromHandle( GetHwnd() ) );

	return result;
}


// CSyncScrolling implementation

CSyncScrolling& CSyncScrolling::AddCtrl( CWnd* pCtrl )
{
	utl::PushUnique( m_ctrls, pCtrl );
	return *this;
}

void CSyncScrolling::SetScrollType( int scrollType )
{
	m_scrollTypes.clear();

	if ( SB_BOTH == scrollType )
	{
		m_scrollTypes.push_back( SB_VERT );
		m_scrollTypes.push_back( SB_HORZ );
	}
	else
	{
		ASSERT( SB_VERT == scrollType || SB_HORZ == scrollType );
		m_scrollTypes.push_back( scrollType );
	}
}

bool CSyncScrolling::SyncHorizontal( void ) const
{
	return utl::Contains( m_scrollTypes, SB_HORZ );
}

bool CSyncScrolling::SyncVertical( void ) const
{
	return utl::Contains( m_scrollTypes, SB_VERT );
}

void CSyncScrolling::SetCtrls( CWnd* pDlg, const UINT ctrlIds[], size_t count )
{
	ASSERT_PTR( pDlg->GetSafeHwnd() );
	m_ctrls.reserve( count );
	for ( size_t i = 0; i != count; ++i )
	{
		CWnd* pCtrl = pDlg->GetDlgItem( ctrlIds[ i ] );
		ASSERT_PTR( pCtrl->GetSafeHwnd() );
		m_ctrls.push_back( pCtrl );
	}
}

void CSyncScrolling::HookThumbTrack( void )
{
	ASSERT( !m_ctrls.empty() );						// must be init
	ASSERT( m_scrollHooks.empty() );				// hook only once

	for ( std::vector< CWnd* >::const_iterator itCtrl = m_ctrls.begin(); itCtrl != m_ctrls.end(); ++itCtrl )
		m_scrollHooks.push_back( new CThumbTrackScrollHook( this, *itCtrl ) );			// WM_VSCROLL/WM_HSCROLL: hook SB_THUMBTRACK events
}

bool CSyncScrolling::Synchronize( CWnd* pRefCtrl )
{
	ASSERT_PTR( pRefCtrl->GetSafeHwnd() );

	if ( !utl::Contains( m_ctrls, pRefCtrl ) )
		return false;

	std::vector< std::pair<UINT, int> > scrollPoses;
	scrollPoses.reserve( m_scrollTypes.size() );
	for ( std::vector< int >::const_iterator itScrollType = m_scrollTypes.begin(); itScrollType != m_scrollTypes.end(); ++itScrollType )
		scrollPoses.push_back( std::make_pair( SB_VERT == *itScrollType ? WM_VSCROLL : WM_HSCROLL, pRefCtrl->GetScrollPos( *itScrollType ) ) );

	for ( std::vector< CWnd* >::const_iterator itCtrl = m_ctrls.begin(); itCtrl != m_ctrls.end(); ++itCtrl )
		if ( *itCtrl != pRefCtrl )
			for ( std::vector< std::pair<UINT, int> >::const_iterator itScrollPos = scrollPoses.begin(); itScrollPos != scrollPoses.end(); ++itScrollPos )
				( *itCtrl )->SendMessage( itScrollPos->first, MAKEWPARAM( SB_THUMBPOSITION, itScrollPos->second ) );

	return true;
}
