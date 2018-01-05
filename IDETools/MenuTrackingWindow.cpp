
#include "stdafx.h"
#include "MenuTrackingWindow.h"
#include "IdeUtilities.h"
#include "MenuUtilitiesOLD.h"
#include "Application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CMenuTrackingWindow::CMenuTrackingWindow( IMenuCommandTarget* pCommandTarget, CMenu& rTrackingMenu,
										  MenuTrackMode menuTrackMode /*= MTM_None*/ )
	: CFrameWnd()
	, m_pCommandTarget( pCommandTarget )
	, m_rTrackingMenu( rTrackingMenu )
	, m_menuTrackMode( menuTrackMode )
	, m_hMenuHover( m_rTrackingMenu )
	, m_hilightId( 0 )
	, m_menuIdleCount( 0 )
{
	menu::addSubPopups( m_popups, m_rTrackingMenu );
}

CMenuTrackingWindow::~CMenuTrackingWindow()
{
}

void CMenuTrackingWindow::Create( CWnd* pOwner )
{
	VERIFY( CFrameWnd::Create( NULL, NULL, WS_POPUP | WS_VISIBLE, CRect( 0, 0, 0, 0 ), pOwner ) );
}

void CMenuTrackingWindow::InvalidateMenuWindow( void )
{
	HWND hWndMenu = menu::getMenuWindowFromPoint();

	if ( hWndMenu != NULL )
		::InvalidateRect( hWndMenu, NULL, TRUE );
}

void CMenuTrackingWindow::OnMenuEnterIdle( int idleCount )
{
	if ( idleCount == 1 && m_hilightId != 0 && m_hMenuHover != NULL )
	{	// first idle time on popup menu, select 'm_hilightId' current item
		// SIMPLER WAY:
		if ( ::GetMenuState( m_hMenuHover, m_hilightId, MF_BYCOMMAND ) != UINT_MAX )
		{	// item exists in popup -> simulate arrow down for non-separator items until item to select
			for ( int i = 0, count = ::GetMenuItemCount( m_hMenuHover ); i < count; ++i )
			{
				UINT itemID = ::GetMenuItemID( m_hMenuHover, i );

				if ( itemID != 0 )
				{	// non-separator item, simulate arrow down key
					PostMessage( WM_KEYDOWN, VK_DOWN );
					if ( itemID == m_hilightId )
						break;
				}
			}
		}
	}
}


// message handlers

BEGIN_MESSAGE_MAP( CMenuTrackingWindow, CFrameWnd )
END_MESSAGE_MAP()

LRESULT CMenuTrackingWindow::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	if ( WM_COMMAND == message )
	{
		if ( m_pCommandTarget != NULL && m_pCommandTarget->OnMenuCommand( LOWORD( wParam ) ) )
			return 0L;
	}
	else if ( WM_INITMENUPOPUP == message )
	{	// restart auto-selection
		if ( !HIWORD( lParam ) )
			m_hMenuHover = (HMENU)wParam;
		else
			m_hMenuHover = NULL;

		m_menuIdleCount = 0;
	}
	else if ( message == WM_ENTERIDLE && wParam == MSGF_MENU )
		OnMenuEnterIdle( ++m_menuIdleCount );
	else if ( m_menuTrackMode == MTM_CommandRepeat )
		if ( message == WM_RBUTTONUP )
		{	// note that 'screenPos' is screen coordinates when modal tracking a menu
			CPoint clickPos( LOWORD( lParam ), HIWORD( lParam ) );
			MenuItem foundItem;

			if ( UINT cmdId = menu::getItemFromPosID( foundItem, clickPos, m_popups ) )		// doesn't seem to function properly
			{
				DEBUG_LOG( _T("Right-clicked command %d\n"), cmdId );
				ASSERT( foundItem.isValidCommand() );
				if ( m_pCommandTarget != NULL && m_pCommandTarget->OnMenuCommand( cmdId ) )
					InvalidateMenuWindow();
			}

			// avoid default processing for right button messages since this may
			// break and reenter the modal loop, which is pretty bad for us
			return 0L;
		}

	return CFrameWnd::WindowProc( message, wParam, lParam );
}
