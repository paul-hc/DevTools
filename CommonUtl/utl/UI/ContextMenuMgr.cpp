
#include "pch.h"
#include "ContextMenuMgr.h"
#include "PopupMenus.h"
#include "MenuUtilities.h"
#include "WndUtils.h"
#include <afxpopupmenu.h>
#include <afxmenutearoffmanager.h>

#include <afxdialogex.h>			// for RUNTIME_CLASS()
#include <afxpropertypage.h>		// for RUNTIME_CLASS()

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace mfc
{
	CContextMenuMgr* CContextMenuMgr::s_pInstance = nullptr;

	CContextMenuMgr::CContextMenuMgr( void )
		: CContextMenuManager()
		, m_pTrackingPopupMenu( nullptr )
	{
		ASSERT_NULL( s_pInstance );
		s_pInstance = this;
	}

	CContextMenuMgr::~CContextMenuMgr()
	{
		ASSERT( this == s_pInstance );
		s_pInstance = nullptr;
	}

	CMFCPopupMenu* CContextMenuMgr::GetTrackingPopupMenu( void ) const
	{
		ASSERT_PTR( this );		// requires that CContextMenuMgr has been initialized in app InitInstance()
		return mfc::GetSafePopupMenu( m_pTrackingPopupMenu );
	}

	void CContextMenuMgr::ResetTrackingPopup( CMFCPopupMenu* pNewTrackingPopup )
	{
		// If allocated by the caller right before a TrackPopupMenu(), it will be used instead of creating a 'new CMFCPopupMenu;' in ShowPopupMenu().

		ASSERT_NULL( m_pNewTrackingPopup.get() );		// never double allocate!
		m_pNewTrackingPopup.reset( pNewTrackingPopup );
	}

	UINT CContextMenuMgr::TrackModalPopup( OPTIONAL HMENU hMenuPopup, CWnd* pTargetWnd, bool sendCommand, CPoint screenPos /*= CPoint( -1, -1 )*/, bool rightAlign /*= false*/ )
	{	// tracks either the hMenuPopup (if specified), or externally stored m_pNewTrackingPopup
		ASSERT( hMenuPopup != nullptr || m_pNewTrackingPopup.get() != nullptr );	// tracking popup must be set by client if hMenuPopup is null

		UINT cmdId = 0;
		HWND hFocusWnd = ::GetFocus();

		ui::AdjustMenuTrackPos( screenPos, pTargetWnd );

		cmdId = TrackPopupMenu( hMenuPopup, screenPos.x, screenPos.y, pTargetWnd, rightAlign );		// HMENU is optional

		ui::TakeFocus( hFocusWnd );

		if ( sendCommand && cmdId != 0 )
			ui::SendCommand( pTargetWnd->GetSafeHwnd(), cmdId );

		return cmdId;
	}

	UINT CContextMenuMgr::TrackModalPopup( CMFCPopupMenu* pPopupMenu, CWnd* pTargetWnd, bool sendCommand, CPoint screenPos, bool rightAlign )
	{
		ASSERT_PTR( pPopupMenu );

		if ( nullptr == m_pNewTrackingPopup.get() )
			ResetTrackingPopup( pPopupMenu );

		return TrackModalPopup( (HMENU)nullptr, pTargetWnd, sendCommand, screenPos, rightAlign );	// track popup window with its own custom menu set up
	}


	CMFCPopupMenu* CContextMenuMgr::ShowPopupMenu( HMENU hMenuPopup, int x, int y, CWnd* pWndOwner, BOOL bOwnMessage /*= FALSE*/, BOOL bAutoDestroy /*= TRUE*/, BOOL bRightAlign /*= FALSE*/ ) override
	{
		// hMenuPopup is optional if we have m_pNewTrackingPopup set up

		if ( nullptr == m_pNewTrackingPopup.get() )
		{
			REQUIRE( ::IsMenu( hMenuPopup ) );

			m_pTrackingPopupMenu = __super::ShowPopupMenu( hMenuPopup, x, y, pWndOwner, bOwnMessage, bAutoDestroy, bRightAlign );		// use standard MFC implementation
			return m_pTrackingPopupMenu;
		}

		if ( pWndOwner != nullptr && !bOwnMessage )
			if ( pWndOwner->IsKindOf( RUNTIME_CLASS( CDialogEx ) ) || pWndOwner->IsKindOf( RUNTIME_CLASS( CMFCPropertyPage ) ) )
			{
				// CDialogEx, CMFCPropertyPage: should own menu messages
				ASSERT( false );
				return nullptr;
			}

		if ( hMenuPopup != nullptr && g_pTearOffMenuManager != nullptr )
			g_pTearOffMenuManager->SetupTearOffMenus( hMenuPopup );

		if ( m_bTrackMode )
			bOwnMessage = TRUE;

		if ( !bOwnMessage )
			while ( pWndOwner != nullptr && HasFlag( pWndOwner->GetStyle(), WS_CHILD ) )
				pWndOwner = pWndOwner->GetParent();

		CMFCPopupMenu* pPopupMenu = m_pNewTrackingPopup.get();

		pPopupMenu->SetAutoDestroy( FALSE );
		pPopupMenu->SetRightAlign( bRightAlign );
		mfc::PopupMenu_SetTrackMode( pPopupMenu, m_bTrackMode );

		if ( !m_bDontCloseActiveMenu )
			CTrackingPopupMenu::CloseActiveMenu();

		if ( !pPopupMenu->Create( pWndOwner, x, y, hMenuPopup, FALSE, bOwnMessage ) )
		{
			m_pNewTrackingPopup.reset();			// delete the tracking menu
			return nullptr;
		}

		m_pTrackingPopupMenu = m_pNewTrackingPopup.release();	// pass ownership to the caller
		return m_pTrackingPopupMenu;
	}

	UINT CContextMenuMgr::TrackPopupMenu( HMENU hMenuPopup, int x, int y, CWnd* pWndOwner, BOOL bRightAlign /*= FALSE*/ ) override
	{
		UINT selCmdId = __super::TrackPopupMenu( hMenuPopup, x, y, pWndOwner, bRightAlign );

		m_pTrackingPopupMenu = nullptr;		// reset the temporary tracking menu
		return selCmdId;
	}
}
