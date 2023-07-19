
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
	{
		ASSERT_NULL( s_pInstance );
		s_pInstance = this;
	}

	CContextMenuMgr::~CContextMenuMgr()
	{
		ASSERT( this == s_pInstance );
		s_pInstance = nullptr;
	}

	void CContextMenuMgr::ResetNewTrackingPopupMenu( mfc::CTrackingPopupMenu* pNewTrackingPopupMenu )
	{
		m_pNewTrackingPopupMenu.reset( pNewTrackingPopupMenu );
	}

	CMFCPopupMenu* CContextMenuMgr::ShowPopupMenu( HMENU hMenuPopup, int x, int y, CWnd* pWndOwner, BOOL bOwnMessage, BOOL bAutoDestroy, BOOL bRightAlign )
	{
		REQUIRE( ::IsMenu( hMenuPopup ) );

		if ( nullptr == m_pNewTrackingPopupMenu.get() )
			return __super::ShowPopupMenu( hMenuPopup, x, y, pWndOwner, bOwnMessage, bAutoDestroy, bRightAlign );		// use standard MFC implementation

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
			return nullptr;

		return m_pNewTrackingPopupMenu.release();		// pass ownership to the caller
	}
}
