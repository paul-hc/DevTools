
#include "pch.h"
#include "PopupMenus.h"
#include "MenuUtilities.h"
#include "ContextMenuMgr.h"
#include "ColorRepository.h"
#include "CmdInfoStore.h"
#include "TooltipsHook.h"
#include "WndUtils.h"
#include "resource.h"
#include "utl/Range.h"
#include <afxcolorbutton.h>

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
		if ( notify && m_pPopupMenu != nullptr )
			if ( const CColorEntry* pClickedColorEntry = checked_static_cast<const mfc::CColorPopupMenu*>( m_pPopupMenu )->FindClickedBarColorEntry() )
				color = pClickedColorEntry->GetColor();		// lookup the proper raw sys-color

		// if notify is true, this gets called by CMFCColorBar::OnSendCommand() on user color selection
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
		mfc::TColorList docColors;

		if ( m_bIsDocumentColors != FALSE )
		{
			REQUIRE( m_pDocColorTable != nullptr && !m_pDocColorTable->IsEmpty() );
			m_pDocColorTable->QueryMfcColors( docColors );
		}

		return new CColorPopupMenu( this, m_Colors, m_Color,
									m_bIsAutomaticButton ? m_strAutomaticButtonLabel.GetString() : nullptr,
									m_bIsOtherButton ? m_strOtherButtonLabel.GetString() : nullptr,
									m_bIsDocumentColors ? m_strDocumentColorsLabel.GetString() : nullptr,
									docColors, m_nColumns, m_nHorzDockRows, m_nVertDockColumns, m_colorAutomatic, m_nID, m_bStdColorDlg );
	}
}


namespace mfc
{
	// CColorPopupMenu implementation

	CColorPopupMenu::CColorPopupMenu( CColorMenuButton* pParentMenuBtn,
									  const mfc::TColorArray& colors, COLORREF color,
									  const TCHAR* pAutoColorLabel, const TCHAR* pMoreColorLabel, const TCHAR* pDocColorsLabel,
									  mfc::TColorList& docColors, int columns, int horzDockRows, int vertDockColumns,
									  COLORREF colorAuto, UINT uiCmdID, BOOL stdColorDlg /*= false*/ )
		: CMFCColorPopupMenu( colors, color, pAutoColorLabel, pMoreColorLabel, pDocColorsLabel, docColors, columns, horzDockRows, vertDockColumns, colorAuto, uiCmdID, stdColorDlg )
		, m_pParentMenuBtn( pParentMenuBtn )
		, m_pColorTable( nullptr )
		, m_pDocColorTable( nullptr )
		, m_rawAutoColor( CLR_NONE )
		, m_rawSelColor( CLR_NONE )
	{	// general constructor, for e.g. CColorMenuButton
		m_pColorBar = mfc::nosy_cast<nosy::CColorBar_>( &m_wndColorBar );

		if ( m_pParentMenuBtn != nullptr )
		{
			m_pColorTable = m_pParentMenuBtn->GetColorTable();
			m_pDocColorTable = m_pParentMenuBtn->GetDocColorTable();
		}
	}

	CColorPopupMenu::CColorPopupMenu( CMFCColorButton* pParentPickerBtn,
									  const mfc::TColorArray& colors, COLORREF color,
									  const TCHAR* pAutoColorLabel, const TCHAR* pMoreColorLabel, const TCHAR* pDocColorsLabel,
									  mfc::TColorList& docColors, int columns, COLORREF colorAuto )
		: CMFCColorPopupMenu( pParentPickerBtn, colors, color, pAutoColorLabel, pMoreColorLabel, pDocColorsLabel, docColors, columns, colorAuto )
		, m_pParentMenuBtn( nullptr )
		, m_pColorTable( nullptr )
		, m_pDocColorTable( nullptr )
		, m_rawAutoColor( CLR_NONE )
		, m_rawSelColor( CLR_NONE )
	{	// color picker constructor
		m_pColorBar = mfc::nosy_cast<nosy::CColorBar_>( &m_wndColorBar );
		m_bEnabledInCustomizeMode = pParentPickerBtn->m_bEnabledInCustomizeMode;
	}

	CColorPopupMenu::~CColorPopupMenu()
	{
	}

	void CColorPopupMenu::SetColorHost( const ui::IColorHost* pColorHost )
	{	// called by picker button that implements ui::IColorHost interface
		ASSERT_PTR( pColorHost );
		m_pColorTable = pColorHost->GetSelColorTable();
		m_pDocColorTable = pColorHost->GetDocColorTable();
	}

	const CColorEntry* CColorPopupMenu::FindClickedBarColorEntry( void ) const
	{
		int clickedBtnPos = m_pColorBar->HitTest( ui::GetCursorPos( m_pColorBar->GetSafeHwnd() ) );
		if ( -1 == clickedBtnPos )
			return nullptr;

		return reinterpret_cast<const CColorEntry*>( mfc::GetButtonItemData( m_pColorBar->GetButton( clickedBtnPos ) ) );
	}

	void CColorPopupMenu::StoreBtnColorEntries( void )
	{
		Range<int> btnIndex;		// ranges are [start, end) for iteration
		const CColorEntry* pColorEntry = nullptr;

		// for each Color button: store pointers color entry into button's m_dwdItemData
		if ( m_pColorTable != nullptr && !m_pColorBar->m_colors.IsEmpty() )
		{
			btnIndex.SetEmptyRange( m_pColorBar->HasAutoBtn() ? 1 : 0 );	// skip Automatic, if any
			btnIndex.m_end += static_cast<int>( m_pColorBar->m_colors.GetSize() );

			for ( pColorEntry = &m_pColorTable->GetColors().front();
				  btnIndex.m_start != btnIndex.m_end;
				  ++btnIndex.m_start, ++pColorEntry )
				StoreButtonColorEntry( m_pColorBar->GetButton( btnIndex.m_start ), pColorEntry );
		}

		// for each Document Color button: store pointers color entry
		if ( m_pDocColorTable != nullptr && !m_pColorBar->m_lstDocColors.IsEmpty() )
		{
			btnIndex.SetEmptyRange( btnIndex.m_end + 2 );				// skip Separator + Doc Label
			btnIndex.m_end += static_cast<INT>( m_pColorBar->m_lstDocColors.GetSize() );

			for ( pColorEntry = &m_pDocColorTable->GetColors().front();
				  btnIndex.m_start != btnIndex.m_end;
				  ++btnIndex.m_start, ++pColorEntry )
				StoreButtonColorEntry( m_pColorBar->GetButton( btnIndex.m_start ), pColorEntry );
		}

		// replace display colors with evaluated colors
		if ( m_pColorBar->HasAutoBtn() )
		{
			m_rawAutoColor = m_pColorBar->GetAutoColor();
			m_pColorBar->SetAutoColor( ui::EvalColor( m_rawAutoColor ) );	// show the display color
		}

		if ( m_pColorBar->HasMoreBtn() )
		{
			m_rawSelColor = m_pColorBar->GetColor();
			m_pColorBar->SetColor( ui::EvalColor( m_rawSelColor ) );		// show the display color
		}
	}

	void CColorPopupMenu::StoreButtonColorEntry( CMFCToolBarButton* pButton, const CColorEntry* pColorEntry )
	{
		ASSERT_PTR( pButton );
		ASSERT_PTR( pColorEntry );

		ASSERT( pButton->m_nStyle != TBBS_SEPARATOR );
		ASSERT( pButton->m_bImage );

		// store the color entry for:
		//	1) so that we can handle TTN_NEEDTEXT only for these color buttons, and do default handling for the other color buttons;
		//	2) get the raw color when a bar button is pressed.
		mfc::SetButtonItemData( pButton, pColorEntry );
	}

	const CColorEntry* CColorPopupMenu::FindColorEntry( COLORREF rawColor ) const
	{
		if ( m_pColorTable != nullptr )
			if ( const CColorEntry* pColorEntry = m_pColorTable->FindColor( rawColor ) )
				return pColorEntry;

		if ( m_pDocColorTable != nullptr )
			if ( const CColorEntry* pColorEntry = m_pDocColorTable->FindColor( rawColor ) )
				return pColorEntry;

		return nullptr;
	}

	bool CColorPopupMenu::FormatColorTipText( OUT std::tstring& rTipText, const CMFCToolBarButton* pButton, int hitBtnIndex ) const
	{
		const CColorEntry* pColorEntry = reinterpret_cast<const CColorEntry*>( mfc::GetButtonItemData( pButton ) );

		if ( nullptr == pColorEntry )
		{
			COLORREF color = CLR_NONE;

			if ( 0 == hitBtnIndex && m_pColorBar->HasAutoBtn() )
				pColorEntry = FindColorEntry( color = m_rawAutoColor );
			else if ( m_pColorBar->HasMoreBtn() )
				pColorEntry = FindColorEntry( color = m_rawSelColor );

			//if (0)
			if ( nullptr == pColorEntry && color != CLR_NONE )
			{
				rTipText = ui::FormatColor( color );		// format the auto/other color
				return true;
			}
		}

		if ( pColorEntry != nullptr )
		{
			rTipText = pColorEntry->FormatColor();			// format the table-qualified color entry
			return true;
		}

		return false;
	}

	bool CColorPopupMenu::Handle_TtnNeedText( NMTTDISPINFO* pNmDispInfo, const CPoint& point )
	{
		int hitBtnIndex = m_pColorBar->HitTest( point );

		if ( hitBtnIndex != -1 )
			if ( const CMFCToolBarButton* pButton = m_pColorBar->GetButton( hitBtnIndex ) )
			{
				std::tstring tipText;

				if ( FormatColorTipText( tipText, pButton, hitBtnIndex ) )
				{
					ui::CTooltipTextMessage message( pNmDispInfo );
					ASSERT( message.IsValidNotification() );		// stray notifications should've been filtered-out

					// equivalent with CTooltipManager::SetTooltipText(pTI, m_pToolTip, AFX_TOOLTIP_TYPE_TOOLBAR, strText, strDescr) - excluding strDescr on Status Bar:
					message.AssignTooltipText( tipText );
					return true;
				}
			}

		return false;
	}

	// message handlers

	BEGIN_MESSAGE_MAP( CColorPopupMenu, CMFCColorPopupMenu )
		ON_WM_CREATE()
	END_MESSAGE_MAP()

	int CColorPopupMenu::OnCreate( CREATESTRUCT* pCreateStruct )
	{
		if ( -1 == __super::OnCreate( pCreateStruct ) )
			return -1;

		StoreBtnColorEntries();			// store pointers color entry into m_dwdItemData

		// install hook to override handling of TTN_NEEDTEXT notifications for color buttons with an attached CColorEntry
		m_pColorBarTipsHook.reset( CToolTipsHandlerHook::CreateHook( m_pColorBar, this, mfc::ToolBar_GetToolTip( m_pColorBar ) ) );
		return 0;
	}
}
