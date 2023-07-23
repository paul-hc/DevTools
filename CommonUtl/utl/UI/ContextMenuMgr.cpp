
#include "pch.h"
#include "ContextMenuMgr.h"
#include "PopupMenus.h"
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

	void CContextMenuMgr::ResetNewTrackingPopupMenu( mfc::CTrackingPopupMenu* pNewTrackingPopupMenu )
	{
		// If allocated by the caller right before a TrackPopupMenu(), it will be used instead of creating a 'new CMFCPopupMenu;' in ShowPopupMenu().

		m_pNewTrackingPopupMenu.reset( pNewTrackingPopupMenu );
	}

	CMFCPopupMenu* CContextMenuMgr::ShowPopupMenu( HMENU hMenuPopup, int x, int y, CWnd* pWndOwner, BOOL bOwnMessage /*= FALSE*/, BOOL bAutoDestroy /*= TRUE*/, BOOL bRightAlign /*= FALSE*/ )
	{
		REQUIRE( ::IsMenu( hMenuPopup ) );

		if ( nullptr == m_pNewTrackingPopupMenu.get() )
		{
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

		if ( g_pTearOffMenuManager != NULL )
			g_pTearOffMenuManager->SetupTearOffMenus( hMenuPopup );

		if ( m_bTrackMode )
			bOwnMessage = TRUE;

		if ( !bOwnMessage )
			while ( pWndOwner != NULL && HasFlag( pWndOwner->GetStyle(), WS_CHILD ) )
				pWndOwner = pWndOwner->GetParent();

		mfc::CTrackingPopupMenu* pPopupMenu = m_pNewTrackingPopupMenu.get();

		pPopupMenu->SetAutoDestroy( FALSE );
		pPopupMenu->SetTrackMode( m_bTrackMode );
		pPopupMenu->SetRightAlign( bRightAlign );

		if ( !m_bDontCloseActiveMenu )
			CTrackingPopupMenu::CloseActiveMenu();

		if ( !pPopupMenu->Create( pWndOwner, x, y, hMenuPopup, FALSE, bOwnMessage ) )
		{
			m_pNewTrackingPopupMenu.reset();			// delete the tracking menu
			return nullptr;
		}

		m_pTrackingPopupMenu = m_pNewTrackingPopupMenu.release();	// pass ownership to the caller
		return m_pTrackingPopupMenu;
	}

	UINT CContextMenuMgr::TrackPopupMenu( HMENU hMenuPopup, int x, int y, CWnd* pWndOwner, BOOL bRightAlign /*= FALSE*/ )
	{
		UINT selCmdId = __super::TrackPopupMenu( hMenuPopup, x, y, pWndOwner, bRightAlign );

		m_pTrackingPopupMenu = nullptr;		// reset the temporary tracking menu
		return selCmdId;
	}
}
