
#include "pch.h"
#include "PopupMenus.h"
#include "MenuUtilities.h"
#include "ContextMenuMgr.h"
#include "ColorRepository.h"
#include "WndUtils.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace mfc
{
	// CTrackingPopupMenu implementation

	CTrackingPopupMenu::CTrackingPopupMenu( ui::ICustomPopupMenu* pCustomPopupMenu /*= nullptr*/ )
		: CMFCPopupMenu()
		, m_pCustomPopupMenu( pCustomPopupMenu )
	{
	}

	CTrackingPopupMenu::~CTrackingPopupMenu()
	{
	}

	void CTrackingPopupMenu::CloseActiveMenu( void )
	{
		if ( CMFCPopupMenu* pMenuActive = CMFCPopupMenu::GetActiveMenu() )
			pMenuActive->SendMessage( WM_CLOSE );
	}

	CMFCToolBarButton* CTrackingPopupMenu::FindTrackingBarButton( UINT btnId )
	{
		const CMFCPopupMenu* pPopupMenu = mfc::CContextMenuMgr::Instance()->GetTrackingPopupMenu();
		CMFCToolBarButton* pButton = nullptr;

		if ( pPopupMenu != nullptr )
			pButton = mfc::FindBarButton( pPopupMenu, btnId );

		if ( nullptr == pButton )
			if ( ( pPopupMenu = CMFCPopupMenu::GetActiveMenu() ) != nullptr )
				pButton = mfc::FindBarButton( pPopupMenu, btnId );		// the button could be on a sub-popup, i.e. the active one

		return pButton;
	}

	BOOL CTrackingPopupMenu::InitMenuBar( void )
	{
		if ( nullptr == m_pCustomPopupMenu )
			return __super::InitMenuBar();			// default MFC implementation

		// Inspired from CMFCPopupMenu::InitMenuBar(), stripped to the minimum required for menu tracking.
		CMFCPopupMenuBar* pMenuBar = GetMenuBar();

		ASSERT_VALID( pMenuBar );
		ENSURE( ::IsMenu( m_hMenu ) );

		if ( !pMenuBar->ImportFromMenu( m_hMenu, TRUE ) )
		{
			TRACE( "Can't import menu\n" );
			return FALSE;
		}

		m_pCustomPopupMenu->OnCustomizeMenuBar( this );

		// Note: for menu button controls:
		//	m_pMessageWnd is the button itself
		//	GetParent() is the CDialogEx
		pMenuBar->OnUpdateCmdUI( (CFrameWnd*)m_pMessageWnd, FALSE );		// hack the cast to CFrameWnd (it's harmless)

		// maybe, main application frame should update the popup menu context before it displayed (example: windows list)
		if ( !ActivatePopupMenu( AFXGetTopLevelFrame( this ), this ) )
			return FALSE;

		RecalcLayout();
		return TRUE;
	}
}


namespace mfc
{
	// CColorMenuButton implementation

	IMPLEMENT_SERIAL( CColorMenuButton, CMFCColorMenuButton, VERSIONABLE_SCHEMA | 1 )

	CColorMenuButton::CColorMenuButton( void )
		: CMFCColorMenuButton()
		, m_pColorTable( nullptr )
		, m_pDocColorTable( nullptr )
	{
	}

	CColorMenuButton::CColorMenuButton( UINT uiCmdID, const CColorTable* pColorTable )
		: CMFCColorMenuButton( uiCmdID, safe_ptr( pColorTable )->GetTableName().c_str() )
		, m_pColorTable( pColorTable )
		, m_pDocColorTable( nullptr )
	{
		ASSERT_PTR( m_pColorTable );

		SetImage( -1 );			// mark as unselected by default

		m_Colors.RemoveAll();
		m_pColorTable->QueryMfcColors( m_Colors );
		m_pColorTable->RegisterColorButtonNames();
		m_dwdItemData = reinterpret_cast<DWORD_PTR>( m_pColorTable );

		SetColumnsNumber( m_pColorTable->GetColumnCount() );
	}

	CColorMenuButton::~CColorMenuButton()
	{
	}

	void CColorMenuButton::SetDocColorTable( const CColorTable* pDocColorTable )
	{
		m_pDocColorTable = pDocColorTable;

		if ( m_pDocColorTable != nullptr && !m_pDocColorTable->IsEmpty() )
			EnableDocumentColors( m_pDocColorTable->GetTableName().c_str() );
	}

	void CColorMenuButton::SetSelected( bool isTableSelected /*= true*/ )
	{
		SetImage( isTableSelected ? afxCommandManager->GetCmdImage( ID_SELECTED_COLOR_BUTTON, FALSE ) : -1 );
	}


	void CColorMenuButton::SetImage( int iImage )
	{
		//__super::SetImage( iImage );

		// Need to override default processing, which alters this button image in afxCommandManager, and switches to m_bUserButton = TRUE.
		// This is to prevent the stickiness of the selected table button, after selecting a color from another table
		m_iImage = iImage;
		m_bImage = m_iImage != -1;
		m_bText = TRUE;
		m_bUserButton = FALSE;
		//afxCommandManager->SetCmdImage( m_nID, m_iImage, m_bUserButton );

		if ( m_pWndParent != NULL )
		{
			CRect rectImage;
			GetImageRect( rectImage );

			m_pWndParent->InvalidateRect( &rectImage );
			m_pWndParent->UpdateWindow();
		}
	}

	void CColorMenuButton::SetColor( COLORREF color, BOOL notify )
	{
		__super::SetColor( color, notify );

		if ( m_pPopupMenu != nullptr )			// not a proxy source button?
			if ( CWnd* pMessageWnd = m_pPopupMenu->GetMessageWnd() )
				ui::SendCommand( pMessageWnd->GetSafeHwnd(), m_nID, CMBN_COLORSELECTED, m_pPopupMenu->GetMenuBar()->GetSafeHwnd() );
	}

	BOOL CColorMenuButton::OpenColorDialog( const COLORREF colorDefault, OUT COLORREF& colorRes )
	{
		COLORREF color = colorDefault;

		if ( !ui::EditColor( &color, GetMessageWnd(), true) )
			return false;

		colorRes = color;
		return true;
	}

	CWnd* CColorMenuButton::GetMessageWnd( void ) const
	{
		if ( m_pWndMessage != nullptr )
			return m_pWndMessage;

		if ( CMFCPopupMenu* pPopupMenu = GetPopupMenu() )
			return pPopupMenu->GetMessageWnd();

		return nullptr;
	}

	void CColorMenuButton::CopyFrom( const CMFCToolBarButton& src )
	{
		__super::CopyFrom( src );

		const CColorMenuButton& srcButton = (const CColorMenuButton&)src;

		m_pColorTable = srcButton.m_pColorTable;
		m_pDocColorTable = srcButton.m_pDocColorTable;
	}

	CMFCPopupMenu* CColorMenuButton::CreatePopupMenu( void )
	{
		//return __super::CreatePopupMenu();

		// customize document colors table (if any)
		ui::TMFCColorList docColors;

		if ( m_bIsDocumentColors != FALSE )
		{
			REQUIRE( m_pDocColorTable != nullptr && !m_pDocColorTable->IsEmpty() );
			m_pDocColorTable->QueryMfcColors( docColors );
		}

		return new CMFCColorPopupMenu( m_Colors, m_Color,
									   m_bIsAutomaticButton ? m_strAutomaticButtonLabel.GetString() : nullptr,
									   m_bIsOtherButton ? m_strOtherButtonLabel.GetString() : nullptr,
									   m_bIsDocumentColors ? m_strDocumentColorsLabel.GetString() : nullptr,
									   docColors, m_nColumns, m_nHorzDockRows, m_nVertDockColumns, m_colorAutomatic, m_nID, m_bStdColorDlg );
	}
}
