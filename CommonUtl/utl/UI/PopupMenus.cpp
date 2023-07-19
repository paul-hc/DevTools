
#include "pch.h"
#include "PopupMenus.h"
#include "MenuUtilities.h"
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

	void CTrackingPopupMenu::CloseActiveMenu( void )
	{
		if ( CMFCPopupMenu* pMenuActive = CMFCPopupMenu::GetActiveMenu() )
			pMenuActive->SendMessage( WM_CLOSE );
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

		// maybe, main application frame should update the popup menu context before it displayed (example - windows list):
		CFrameWnd* pTopLevelFrame = AFXGetTopLevelFrame( this );

		if ( !ActivatePopupMenu( pTopLevelFrame, this ) )
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
	{
	}

	CColorMenuButton::CColorMenuButton( UINT uiCmdID, const CColorTable* pColorTable )
		: CMFCColorMenuButton( uiCmdID, safe_ptr( pColorTable )->GetTableName().c_str() )
		, m_pColorTable( pColorTable )
	{
		ASSERT_PTR( m_pColorTable );

		m_Colors.RemoveAll();
		m_pColorTable->QueryMfcColors( m_Colors );
		m_pColorTable->RegisterColorButtonNames();

		SetColumnsNumber( m_pColorTable->GetColumnCount() );
	}

	CColorMenuButton::~CColorMenuButton()
	{
	}

	void CColorMenuButton::SetColor( COLORREF color, BOOL notify )
	{
		__super::SetColor( color, notify );
	}

	BOOL CColorMenuButton::OpenColorDialog( const COLORREF colorDefault, OUT COLORREF& colorRes )
	{
		COLORREF color = colorDefault;

		if ( !ui::EditColor( &color, GetMessageWnd(), true) )
			return false;

		colorRes = color;
		return true;
	}

	void CColorMenuButton::CopyFrom( const CMFCToolBarButton& src )
	{
		__super::CopyFrom( src );

		const CColorMenuButton& srcButton = (const CColorMenuButton&)src;

		m_pColorTable = srcButton.m_pColorTable;
	}

	CWnd* CColorMenuButton::GetMessageWnd( void ) const
	{
		if ( m_pWndMessage != nullptr )
			return m_pWndMessage;

		if ( CMFCPopupMenu* pPopupMenu = GetPopupMenu() )
			return pPopupMenu->GetMessageWnd();

		return nullptr;
	}
}