
#include "stdafx.h"
#include "TrackMenuWnd.h"
#include "IdeUtilities.h"
#include "Application.h"
#include "utl/CmdUpdate.h"
#include "utl/ContainerUtilities.h"
#include "utl/MenuUtilities.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTrackMenuWnd::CTrackMenuWnd( CCmdTarget* pCmdTarget /*= NULL*/ )
	: CStatic()
	, m_pCmdTarget( pCmdTarget )
	, m_rightClickRepeat( false )
	, m_hilightId( 0 )
	, m_selCmdId( 0 )
{
}

CTrackMenuWnd::~CTrackMenuWnd()
{
}

bool CTrackMenuWnd::Create( CWnd* pParentWnd )
{
	return CStatic::Create( _T("TRACKING-STATIC"), WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY, CRect( 0, 0, 0, 0 ), pParentWnd ) != FALSE;
}

UINT CTrackMenuWnd::TrackContextMenu( CMenu* pPopupMenu, CPoint screenPos /*= ui::GetCursorPos()*/, UINT flags /*= TPM_RIGHTBUTTON*/ )
{
	if ( m_rightClickRepeat )
		ClearFlag( flags, TPM_RIGHTBUTTON );		// avoid eating right clicks as valid commands

	if ( HasFlag( flags, TPM_RETURNCMD ) )
	{
		ClearFlag( flags, TPM_RETURNCMD );			// this returns the command anyway; avoid problems of CMenu::TrackPopupMenu returning 0xFFFF in the high word of the selected command
		TRACE( "! CTrackMenuWnd::TrackContextMenu(): forced removing the TPM_RETURNCMD (sould not be passed)." );
	}
	ClearFlag( flags, TPM_NONOTIFY );				// we do need to receive and handle all menu messages

	SetFocus();										// MSDN documentation of CMenu::TrackPopupMenu requires SetForegroundWindow(), but we use a child window (static control)
	m_selCmdId = 0;

	BOOL selected = ui::TrackPopupMenu( *pPopupMenu, this, screenPos, flags );

	PostMessage( WM_NULL );							// TrackPopupMenu on MSDN: Force a task switch to the application that called TrackPopupMenu, by posting a benign message to the window or thread

	if ( selected != FALSE )
		ui::PumpPendingMessages( m_hWnd );			// wait for the WM_COMMAND message to be dispatched

	return m_selCmdId;
}

bool CTrackMenuWnd::HighlightMenuItem( HMENU hHoverPopup )
{
	ASSERT( m_hilightId != 0 );
	ASSERT_PTR( ::IsMenu( hHoverPopup ) );

	// simulate selection hilight on current item: by posting arrow down for non-separator items until reaching the m_hilightId item
	if ( ::GetMenuState( hHoverPopup, m_hilightId, MF_BYCOMMAND ) != UINT_MAX )			// item exists in popup?
		for ( UINT i = 0, count = ::GetMenuItemCount( hHoverPopup ); i != count; ++i )
			if ( UINT itemId = ::GetMenuItemID( hHoverPopup, i ) )		// not a separator
			{	// non-separator item, simulate arrow-down key
				PostMessage( WM_KEYDOWN, VK_DOWN );

				if ( itemId == m_hilightId )
					return true;
			}

	return false;
}

std::pair< HMENU, UINT > CTrackMenuWnd::FindMenuItemFromPoint( const CPoint& screenPos ) const
{
	if ( HWND hMenuWnd = ui::FindMenuWindowFromPoint( screenPos ) )
		for ( std::vector< HMENU >::const_iterator itSubMenu = m_subMenus.begin(); itSubMenu != m_subMenus.end(); ++itSubMenu )
		{
			HMENU hSubMenu = *itSubMenu;
			int itemPos = ::MenuItemFromPoint( hMenuWnd, hSubMenu, screenPos );

			if ( itemPos != -1 )
				if ( ui::IsHiliteMenuItem( hSubMenu, itemPos ) )
				{
					int cmdId = ::GetMenuItemID( hSubMenu, itemPos );
					if ( cmdId != 0 && cmdId != UINT_MAX )
						return std::pair< HMENU, UINT >( hSubMenu, cmdId );
					else
						break;
				}
		}

	return std::pair< HMENU, UINT >( NULL, 0 );
}

BOOL CTrackMenuWnd::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	BOOL handled = m_pCmdTarget != NULL && m_pCmdTarget->OnCmdMsg( id, code, pExtra, pHandlerInfo );

	if ( !handled )
		handled = __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );

	if ( CN_COMMAND == code && NULL == pHandlerInfo )
		m_selCmdId = id;

	return handled;
}


// message handlers

BEGIN_MESSAGE_MAP( CTrackMenuWnd, CStatic )
	ON_WM_INITMENUPOPUP()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

void CTrackMenuWnd::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	AfxCancelModes( m_hWnd );		// cancel any combobox popups that could be in toolbars or dialog bars
	if ( !isSysMenu )
		ui::UpdateMenuUI( this, pPopupMenu );

	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );

	if ( !isSysMenu )
	{
		if ( m_hilightId != 0 )
			HighlightMenuItem( pPopupMenu->GetSafeHmenu() );

		utl::AddUnique( m_subMenus, pPopupMenu->GetSafeHmenu() );
	}
}

void CTrackMenuWnd::OnRButtonUp( UINT vkFlags, CPoint point )
{
	if ( m_rightClickRepeat )
	{
		// point is in screen coordinates when modal tracking a menu
		std::pair< HMENU, UINT > clickedItem = FindMenuItemFromPoint( point );
		if ( clickedItem.second != 0 )
		{
			DEBUG_LOG( _T("Right-clicked command %d"), clickedItem.second );

			if ( m_pCmdTarget != NULL && ui::HandleCommand( m_pCmdTarget, clickedItem.second ) )
			{
				ui::UpdateMenuUI( this, CMenu::FromHandle( clickedItem.first ) );
				ui::InvalidateMenuWindow();
			}
		}

		return;		// avoid default processing for right button messages since this may break and re-enter the modal loop, which is pretty bad for us
	}

	__super::OnRButtonUp( vkFlags, point );
}
