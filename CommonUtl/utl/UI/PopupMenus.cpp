
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

		// maybe, main application frame should update the popup menu context before it displayed (example: windows list)
		if ( !ActivatePopupMenu( AFXGetTopLevelFrame( this ), this ) )
			return FALSE;

		RecalcLayout();
		return TRUE;
	}
}


namespace mfc
{
	// CColorPopupMenu implementation

	CColorPopupMenu::CColorPopupMenu( CMFCColorButton* pParentBtn, const ui::TMFCColorArray& colors, COLORREF color,
									  const TCHAR* pAutoColorLabel, const TCHAR* pMoreColorLabel, const TCHAR* pDocColorsLabel,
									  ui::TMFCColorList& docColors, int columns, COLORREF colorAuto )
		: CMFCColorPopupMenu( pParentBtn, colors, color, pAutoColorLabel, pMoreColorLabel, pDocColorsLabel, docColors, columns, colorAuto )
	{
	}

	CColorPopupMenu::~CColorPopupMenu()
	{
	}


	// message handlers

	BEGIN_MESSAGE_MAP(CColorPopupMenu, CMFCColorPopupMenu)
		//ON_WM_CREATE()
	END_MESSAGE_MAP()

	/*
	int CColorPopupMenu::OnCreate( CREATESTRUCT* pCreateStruct )
	{
		// verbatim from CMFCColorPopupMenu::OnCreate()
		//
		if ( CMFCToolBar::IsCustomizeMode() && !m_bEnabledInCustomizeMode )
			return -1;			// don't show color popup in customization mode

		if ( -1 == CMiniFrameWnd::OnCreate( pCreateStruct ) )
			return -1;

		DWORD toolbarStyle = AFX_DEFAULT_TOOLBAR_STYLE;

		if ( GetAnimationType() != NO_ANIMATION && !CMFCToolBar::IsCustomizeMode() )
			toolbarStyle &= ~WS_VISIBLE;

		if ( !m_wndColorBar.Create( this, toolbarStyle | CBRS_TOOLTIPS | CBRS_FLYBY, 1 ) )
			return -1;			// can't create popup menu bar

		CWnd* pWndParent = GetParent();
		ASSERT_VALID( pWndParent );

		m_wndColorBar.SetOwner( pWndParent );
		m_wndColorBar.SetPaneStyle( m_wndColorBar.GetPaneStyle() | CBRS_TOOLTIPS );

		//if ( m_pCustomPopupMenu != nullptr )
		//	m_pCustomPopupMenu->OnCustomizeMenuBar( this );		// UTL-UI: inject the menu bar customization code

		// maybe, main application frame should update the popup menu context before it displayed (example: windows list)
		ActivatePopupMenu( AFXGetTopLevelFrame( pWndParent ), this );
		RecalcLayout();
		return 0;
	}*/
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

	void CColorMenuButton::SetColor( COLORREF color, BOOL notify )
	{
		__super::SetColor( color, notify );

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

	void CColorMenuButton::OnDraw( CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages, BOOL bHorz, BOOL bCustomizeMode, BOOL bHighlight, BOOL bDrawBorder, BOOL bGrayDisabledButtons )
	{
		__super::OnDraw( pDC, rect, pImages, bHorz, bCustomizeMode, bHighlight, bDrawBorder, bGrayDisabledButtons );
	}
}
