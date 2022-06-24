
#include "stdafx.h"
#include "TrackMenuWnd.h"
#include "IdeUtilities.h"
#include "utl/Algorithms.h"
#include "utl/UI/CmdUpdate.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/PostCall.h"
#include "utl/UI/ProcessUtils.h"
#include "utl/UI/WndUtils.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseTrackMenuWnd.hxx"


CTrackMenuWnd::CTrackMenuWnd( CCmdTarget* pCmdTarget /*= NULL*/ )
	: CLayoutDialog()
	, m_pCmdTarget( pCmdTarget )
	, m_rightClickRepeat( false )
	, m_hilightId( 0 )
	, m_selCmdId( 0 )
{
}

CTrackMenuWnd::CTrackMenuWnd( CWnd* pParentWnd, CCmdTarget* pCmdTarget )
	: CLayoutDialog( IDD_INPUT_TYPE_QUALIFIER_DIALOG, pParentWnd )
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
	bool result = CreateModeless( IDD_INPUT_TYPE_QUALIFIER_DIALOG, pParentWnd, SW_SHOWNORMAL );
	m_autoDelete = false;
	return result;

//	return CreateEx( WS_EX_TOOLWINDOW, AfxRegisterWndClass( 0 ), _T("<TrackMenuHiddenPopup>"), WS_POPUP | WS_CAPTION | WS_BORDER, 100, 100, 50, 50, pParentWnd->GetSafeHwnd(), NULL ) != FALSE;
}

#include "utl/Logger.h"
#include "utl/UI/WindowDebug.h"

UINT CTrackMenuWnd::TrackContextMenu( CMenu* pPopupMenu, CPoint screenPos /*= ui::GetCursorPos()*/, UINT flags /*= TPM_RIGHTBUTTON*/ )
{
//CScopedAttachThreadInput scopedThreadInput( ide::GetFocusWindow()->GetSafeHwnd() );

CWnd* pFocus = GetFocus();
LOG_TRACE( dbg::FormatWndInfo( pFocus->GetSafeHwnd(), _T("CTrackMenuWnd::TrackContextMenu() - Pre - focus:") ).c_str() );

	ShowWindow( SW_SHOWNORMAL );
	SetForegroundWindow();						// MSDN documentation of CMenu::TrackPopupMenu requires SetForegroundWindow(), but we use a child window (static control)
//	SetFocus();									// MSDN documentation of CMenu::TrackPopupMenu requires SetForegroundWindow(), but we use a child window (static control)

pFocus = GetFocus();
LOG_TRACE( dbg::FormatWndInfo( pFocus->GetSafeHwnd(), _T("CTrackMenuWnd::TrackContextMenu() - SetForeground - focus:") ).c_str() );

	if ( true /*proc::InDifferentThread( GetParent()->GetSafeHwnd() )*/ )
	{	// simulate a window click to "unlock" the keyboard for menu selection - workaround CMenu::TrackPopupMenu multi-threading issues when cancelling the menu (clicking outside of it).
		SendMessage( WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM( 1, 1 ) );		// client coordinates
//		SendMessage( WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM( 2, 2 ) );
		PostMessage( WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM( 2, 2 ) );
	}
ui::PumpPendingMessages( m_hWnd );			// wait for the WM_COMMAND message to be dispatched
SetForegroundWindow();						// MSDN documentation of CMenu::TrackPopupMenu requires SetForegroundWindow(), but we use a child window (static control)

	DoTrackPopupMenu( pPopupMenu, screenPos, flags );

	PostMessage( WM_NULL );						// TrackPopupMenu on MSDN: force a task switch to the application that called TrackPopupMenu, by posting a benign message to the window or thread
	ui::PumpPendingMessages( m_hWnd );			// wait for the WM_COMMAND message to be dispatched

	return m_selCmdId;
}

UINT CTrackMenuWnd::ExecuteContextMenu( CMenu* pPopupMenu, const CPoint& screenPos /*= ui::GetCursorPos()*/, UINT flags /*= TPM_RIGHTBUTTON*/ )
{
	m_pMenuInfo.reset( new CMenuInfo( pPopupMenu, screenPos, flags ) );

	if ( IDCANCEL == DoModal() )
		return 0;

	return m_selCmdId;
}

UINT CTrackMenuWnd::DoTrackPopupMenu( CMenu* pPopupMenu, const CPoint& screenPos, UINT flags )
{
	if ( m_rightClickRepeat )
		ClearFlag( flags, TPM_RIGHTBUTTON );	// avoid eating right clicks as valid commands

	if ( HasFlag( flags, TPM_RETURNCMD ) )
	{
		ClearFlag( flags, TPM_RETURNCMD );		// this returns the command anyway; avoid problems of CMenu::TrackPopupMenu returning 0xFFFF in the high word of the selected command
		TRACE( "! CTrackMenuWnd::TrackContextMenu(): forced removing the TPM_RETURNCMD (sould not be passed)." );
	}
	ClearFlag( flags, TPM_NONOTIFY );			// we do need to receive and handle all menu messages

	m_selCmdId = 0;
	m_subMenus.clear();

	BOOL selected = ui::TrackPopupMenu( *pPopupMenu, this, screenPos, flags ); selected;

LOG_TRACE( _T("CTrackMenuWnd::TrackContextMenu() - m_selCmdId=%d"), m_selCmdId );

	if ( !m_modeless )
		EndDialog( IDOK );

	return m_selCmdId;
}

bool CTrackMenuWnd::_HighlightMenuItem( HMENU hHoverPopup )
{
	return
		ui::HoverOnMenuItem( m_hWnd, hHoverPopup, m_hilightId ) ||
		ui::ScrollVisibleMenuItem( m_hWnd, hHoverPopup, m_hilightId );
}

std::pair<HMENU, UINT> CTrackMenuWnd::FindMenuItemFromPoint( const CPoint& screenPos ) const
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
						return std::pair<HMENU, UINT>( hSubMenu, cmdId );
					else
						break;
				}
		}

	return std::pair<HMENU, UINT>( NULL, 0 );
}

BOOL CTrackMenuWnd::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) override
{
	BOOL handled = m_pCmdTarget != NULL && m_pCmdTarget->OnCmdMsg( id, code, pExtra, pHandlerInfo );

	if ( CN_COMMAND == code )
		if ( NULL == pHandlerInfo )
			m_selCmdId = id;

	if ( !handled )
		handled = __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );

	return handled;
}


// message handlers

BEGIN_MESSAGE_MAP( CTrackMenuWnd, CLayoutDialog )
	ON_WM_INITMENUPOPUP()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

BOOL CTrackMenuWnd::OnInitDialog( void ) override
{
	BOOL result = __super::OnInitDialog();

	if ( m_pMenuInfo.get() != NULL )
	{
		ShowWindow( SW_SHOWNORMAL );
		ui::PostCall( this, &CTrackMenuWnd::DoTrackPopupMenu, m_pMenuInfo->m_pPopupMenu, m_pMenuInfo->m_screenPos, m_pMenuInfo->m_flags );
//		DoTrackPopupMenu( m_pMenuInfo->m_pPopupMenu, m_pMenuInfo->m_screenPos, m_pMenuInfo->m_flags );
	}

	return result;
}

void CTrackMenuWnd::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
//	AfxCancelModes( m_hWnd );		// cancel any combobox popups that could be in toolbars or dialog bars
//	if ( !isSysMenu )
//		ui::UpdateMenuUI( this, pPopupMenu );

	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );

	if ( !isSysMenu )
	{
		if ( m_hilightId != 0 )
			ui::PostCall( this, &CTrackMenuWnd::_HighlightMenuItem, pPopupMenu->GetSafeHmenu() );		// wait for the menu to become visible to allow hover item highlighting

		utl::AddUnique( m_subMenus, pPopupMenu->GetSafeHmenu() );
	}
}

void CTrackMenuWnd::OnRButtonUp( UINT vkFlags, CPoint point )
{
	if ( m_rightClickRepeat )
	{
		// point is in screen coordinates when modal tracking a menu
		std::pair<HMENU, UINT> clickedItem = FindMenuItemFromPoint( point );
		if ( clickedItem.second != 0 )
		{
			LOG_TRACE( _T("Right-clicked command %d"), clickedItem.second );

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
