
#include "pch.h"
#include "PopupMenus.h"
#include "MenuUtilities.h"
#include "ContextMenuMgr.h"
#include "ColorRepository.h"
#include "CmdInfoStore.h"
#include "TooltipsHook.h"
#include "WndUtils.h"
#include "resource.h"
#include "utl/ScopedValue.h"
#include <afxcolorbutton.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	template< typename StringT >
	typename StringT::value_type* AllocStringCopy( const StringT& text )
	{
		typedef typename StringT::value_type TChar;
		TChar* pText = (TChar*)::calloc( text.length() + 1, sizeof( TChar ) );

		return pText != nullptr ? _tcscpy( pText, text.c_str() ) : nullptr;
	}
}


namespace mfc
{
	// CTrackingPopupMenu implementation

	CTrackingPopupMenu::CTrackingPopupMenu( ui::ICustomPopupMenu* pCustomPopupMenu /*= nullptr*/, int trackingMode /*= 0*/ )
		: CMFCPopupMenu()
		, m_pCustomPopupMenu( pCustomPopupMenu )
		, m_trackingMode( trackingMode )
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

		m_pCustomPopupMenu->OnCustomizeMenuBar( this, m_trackingMode );

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
	// CToolBarColorButton implementation

	IMPLEMENT_SERIAL( CToolBarColorButton, CMFCToolBarMenuButton, VERSIONABLE_SCHEMA | 1 );

	CToolBarColorButton::CToolBarColorButton( void )
		: CMFCToolBarMenuButton()
		, m_color( CLR_NONE )
	{
	}

	CToolBarColorButton::CToolBarColorButton( UINT btnId, COLORREF color, const TCHAR* pText /*= nullptr*/ )
		: CMFCToolBarMenuButton( btnId, nullptr, -1, pText, false )
		, m_color( color )
	{
		if ( !ui::IsUndefinedColor( m_color ) )
			mfc::Button_SetImageById( this, ID_TRANSPARENT );		// draw color on top of transparent background
	}

	CToolBarColorButton::CToolBarColorButton( UINT btnId, const CColorEntry* pColorEntry )
		: CMFCToolBarMenuButton( btnId, nullptr, -1, pColorEntry->GetName().c_str() )
		, m_color( pColorEntry->GetColor() )
	{
		m_dwdItemData = reinterpret_cast<DWORD_PTR>( pColorEntry );

		if ( !ui::IsUndefinedColor( m_color ) )
			mfc::Button_SetImageById( this, ID_TRANSPARENT );		// draw color on top of transparent background
	}

	CToolBarColorButton::CToolBarColorButton( const CMFCToolBarButton* pSrcButton, COLORREF color )
		: CMFCToolBarMenuButton()
		, m_color( CLR_NONE )
	{
		REQUIRE( is_a<CMFCToolBarMenuButton>( pSrcButton ) );		// standard source menu button, as created by CMFCPopupMenuBar::ImportFromMenu()
		CMFCToolBarMenuButton::CopyFrom( *pSrcButton );				// non-virtual call
		SetColor( color );
	}

	void CToolBarColorButton::SetColor( COLORREF color )
	{
		m_color = color;

		mfc::Button_SetImageById( this, m_color != CLR_NONE ? ID_TRANSPARENT : m_nID );		// draw color on top of transparent background, or standard image if null color
		mfc::Button_RedrawImage( this );
	}

	void CToolBarColorButton::SetChecked( bool checked )
	{
		UINT style = m_nStyle;
		SetFlag( style, TBBS_CHECKED, checked );
		SetStyle( style );
	}

	CToolBarColorButton* CToolBarColorButton::ReplaceWithColorButton( CMFCToolBar* pToolBar, UINT btnId, COLORREF color, OUT int* pIndex )
	{
		ASSERT_PTR( pToolBar->GetSafeHwnd() );
		int index = pToolBar->CommandToIndex( btnId );

		utl::AssignPtr( pIndex, index );
		if ( -1 == index )
			return nullptr;

		mfc::CToolBarColorButton colorButton( pToolBar->GetButton( index ), color );

		pToolBar->ReplaceButton( colorButton.m_nID, colorButton );
		return checked_static_cast<CToolBarColorButton*>( pToolBar->GetButton( index ) );	// the button clone
	}

	void CToolBarColorButton::SetImage( int iImage ) overrides( CMFCToolBarButton )
	{
			//__super::SetImage( iImage );

		// Need to override default processing, which alters this button image in afxCommandManager, and switches to m_bUserButton = TRUE.
		// This is to prevent the stickiness of the selected table button, after selecting a color from another table
		m_iImage = iImage;
		m_bImage = m_iImage != -1;

		mfc::Button_RedrawImage( this );
	}

	void CToolBarColorButton::CopyFrom( const CMFCToolBarButton& src ) overrides( CMFCToolBarMenuButton )
	{
		__super::CopyFrom( src );

		const CToolBarColorButton& srcButton = (const CToolBarColorButton&)src;

		m_color = srcButton.m_color;
	}

	BOOL CToolBarColorButton::OnToolHitTest( const CWnd* pWnd, TOOLINFO* pTI ) overrides( CMFCToolBarButton )
	{
		if ( CMFCToolBar::GetShowTooltips() && pTI != nullptr && !ui::IsUndefinedColor( m_color ) )		// prevent displaying the pointless "(null)"
		{
			const CColorEntry* pColorEntry = GetColorEntry();

			if ( nullptr == pColorEntry )
				pColorEntry = CColorRepository::Instance()->FindColorEntry( m_color );

			if ( pColorEntry != nullptr )
				pTI->lpszText = hlp::AllocStringCopy( pColorEntry->FormatColor( CColorEntry::s_fieldSep, false ) );
			else
				pTI->lpszText = hlp::AllocStringCopy( ui::FormatColor( m_color, CColorEntry::s_fieldSep ) );

			if ( pTI->lpszText != nullptr )
				return true;
		}

		return __super::OnToolHitTest( pWnd, pTI );
	}

	void CToolBarColorButton::OnDraw( CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages, BOOL bHorz /*= TRUE*/, BOOL bCustomizeMode /*= FALSE*/,
									  BOOL bHighlight /*= FALSE*/, BOOL bDrawBorder /*= TRUE*/, BOOL bGrayDisabledButtons /*= TRUE*/ ) overrides( CMFCToolBarMenuButton )
	{
		__super::OnDraw( pDC, rect, pImages, bHorz, bCustomizeMode, bHighlight, bDrawBorder, bGrayDisabledButtons );

		if ( ui::IsUndefinedColor( m_color ) || nullptr == pImages )
			return;

		// draw the color square
		CRect colorRect = pImages->GetLastImageRect();
		bool enabled = !HasFlag( m_nStyle, TBBS_DISABLED );

		if ( enabled && !HasFlag( m_nStyle, TBBS_CHECKED ) )	// avoid double frame drawing (due to base drawing)
			FillInterior( pDC, colorRect, bHighlight );			// fill button interior and frame

		colorRect.DeflateRect( 2, 2 );

		{
			COLORREF color = ui::EvalColor( m_color );
			CRect fillColorRect = colorRect;

			fillColorRect.DeflateRect( 1, 1 );
			ui::FillRect( *pDC, fillColorRect, color );		// core color square
		}

		// draw frame around the color square:
		if ( enabled )
			pDC->Draw3dRect( colorRect, GetGlobalData()->clrBarShadow, GetGlobalData()->clrBarShadow );
		else
		{
			colorRect.right--;
			colorRect.bottom--;

			pDC->Draw3dRect( colorRect, GetGlobalData()->clrBarHilite, GetGlobalData()->clrBarShadow );
			colorRect.OffsetRect( 1, 1 );
			pDC->Draw3dRect( colorRect, GetGlobalData()->clrBarShadow, GetGlobalData()->clrBarHilite );
		}
	}
}


namespace mfc
{
	// CColorMenuButton implementation

	IMPLEMENT_SERIAL( CColorMenuButton, CMFCColorMenuButton, VERSIONABLE_SCHEMA | 1 )

	CColorMenuButton::CColorMenuButton( void )
		: CMFCColorMenuButton()
		, m_pColorTable( nullptr )
		, m_pEditorHost( nullptr )
	{
	}

	CColorMenuButton::CColorMenuButton( UINT btnId, const CColorTable* pColorTable )
		: CMFCColorMenuButton( btnId, safe_ptr( pColorTable )->GetTableName().c_str() )
		, m_pColorTable( pColorTable )
		, m_pEditorHost( nullptr )
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

	void CColorMenuButton::SetEditorHost( ui::IColorEditorHost* pEditorHost )
	{
		m_pEditorHost = pEditorHost;

		if ( m_pEditorHost != nullptr && m_pColorTable == m_pEditorHost->GetSelColorTable() )		// button of the selected table?
			SetSelectedTable( m_pEditorHost->GetColor(), m_pEditorHost->GetAutoColor(), m_pEditorHost->GetDocColorTable() );
	}

	void CColorMenuButton::SetSelectedTable( COLORREF color, COLORREF autoColor, const CColorTable* pDocColorsTable )
	{
		SetDisplayColorBox( ID_SELECTED_COLOR_BUTTON );

		if ( autoColor != CLR_NONE )
			EnableAutomaticButton( mfc::CColorLabels::s_autoLabel, color );

		EnableOtherButton( mfc::CColorLabels::s_moreLabel );

		if ( pDocColorsTable != nullptr && !pDocColorsTable->IsEmpty() )
			EnableDocumentColors( pDocColorsTable->GetTableName().c_str() );

		SetColor( color, false );
	}

	void CColorMenuButton::SetDisplayColorBox( UINT imageId )
	{
		if ( imageId != UINT_MAX )
			mfc::Button_SetImageById( this, imageId != 0 ? imageId : ID_TRANSPARENT );
		else
			SetImage( -1 );
	}

	CWnd* CColorMenuButton::GetMessageWnd( void ) const
	{
		if ( m_pWndMessage != nullptr )
			return m_pWndMessage;

		if ( CMFCPopupMenu* pPopupMenu = GetPopupMenu() )
			return pPopupMenu->GetMessageWnd();

		return nullptr;
	}

	void CColorMenuButton::SetImage( int iImage ) overrides( CMFCToolBarButton )
	{
			//__super::SetImage( iImage );

		// Need to override default processing, which alters this button image in afxCommandManager, and switches to m_bUserButton = TRUE.
		// This is to prevent the stickiness of the selected table button, after selecting a color from another table
		m_iImage = iImage;
		m_bImage = m_iImage != -1;
		m_bText = TRUE;
		m_bUserButton = FALSE;

		if ( m_pWndParent != NULL )
		{
			CRect rectImage;
			GetImageRect( rectImage );

			m_pWndParent->InvalidateRect( &rectImage );
			m_pWndParent->UpdateWindow();
		}
	}

	void CColorMenuButton::SetColor( COLORREF color, BOOL notify ) overrides( CMFCColorMenuButton )
	{
		bool isTrackingMFCColorBar = m_pPopupMenu != nullptr && is_a<CMFCColorBar>( m_pPopupMenu->GetMenuBar() );

		if ( notify )
		{
			// if using a user custom color table, color can be picked from any table, but the selected user table is retained (not switched to picked table)
			if ( m_pEditorHost != nullptr )
				m_pEditorHost->SwitchSelColorTable( m_pColorTable );

			if ( isTrackingMFCColorBar )
				if ( const CColorEntry* pClickedColorEntry = checked_static_cast<const mfc::CColorPopupMenu*>( m_pPopupMenu )->FindClickedBarColorEntry() )
					color = pClickedColorEntry->GetColor();		// lookup the proper raw sys-color
		}

		// if notify is true, this gets called by CMFCColorBar::OnSendCommand() on user color selection
		__super::SetColor( color, notify );

		if ( notify )
			if ( m_pEditorHost != nullptr )
				m_pEditorHost->SetColor( color, true );
			else if ( m_pPopupMenu != nullptr )			// tracking?
				if ( CWnd* pMessageWnd = m_pPopupMenu->GetMessageWnd() )
					ui::SendCommand( pMessageWnd->GetSafeHwnd(), m_nID, CMBN_COLORSELECTED, m_pPopupMenu->GetMenuBar()->GetSafeHwnd() );
	}

	BOOL CColorMenuButton::OpenColorDialog( const COLORREF colorDefault, OUT COLORREF& rColor ) overrides( CMFCColorMenuButton )
	{
		if ( m_pEditorHost != nullptr )
		{
			m_pEditorHost->EditColorDialog();
			return false;		// handled internally already
		}

		COLORREF color = colorDefault;

		if ( !ui::EditColor( &color, GetMessageWnd(), true ) )
			return false;

		rColor = color;
		return true;
	}

	void CColorMenuButton::CopyFrom( const CMFCToolBarButton& src ) overrides( CMFCColorMenuButton )
	{
		__super::CopyFrom( src );

		const CColorMenuButton& srcButton = (const CColorMenuButton&)src;

		m_pColorTable = srcButton.m_pColorTable;
		m_pEditorHost = srcButton.m_pEditorHost;
	}

	void CColorMenuButton::OnDraw( CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages,
								   BOOL bHorz, BOOL bCustomizeMode, BOOL bHighlight, BOOL bDrawBorder, BOOL bGrayDisabledButtons ) overrides( CMFCColorMenuButton )
	{
		CScopedValue<COLORREF> scDisplayColor( &m_Color, ui::EvalColor( m_pEditorHost != nullptr ? m_pEditorHost->GetActualColor() : m_Color ) );		// show the display color while drawing

		__super::OnDraw( pDC, rect, pImages, bHorz, bCustomizeMode, bHighlight, bDrawBorder, bGrayDisabledButtons );
	}

	CMFCPopupMenu* CColorMenuButton::CreatePopupMenu( void ) overrides( CMFCColorMenuButton )
	{
			//return __super::CreatePopupMenu();

		// customize document colors table (if any)
		mfc::TColorList docColors;

		if ( m_bIsDocumentColors )
		{
			ASSERT_PTR( m_pEditorHost );

			const CColorTable* pShadesTable = m_pEditorHost->GetDocColorTable();

			if ( !pShadesTable->IsEmpty() )
				pShadesTable->QueryMfcColors( docColors );
		}

		if ( m_pColorTable->IsSysColorTable() )
			return new mfc::CColorTablePopupMenu( this );

		return new mfc::CColorPopupMenu( this, m_Colors, m_Color,
										 m_strAutomaticButtonLabel, m_strOtherButtonLabel, m_strDocumentColorsLabel,
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
		, m_pEditorHost( nullptr )
		, m_pColorTable( nullptr )
		, m_pDocColorTable( nullptr )
		, m_rawAutoColor( CLR_NONE )
		, m_rawSelColor( CLR_NONE )
	{	// general constructor: pParentMenuBtn could be null, and it provides the color table
		m_pParentBtn = pParentMenuBtn;
		m_pColorBar = mfc::nosy_cast<nosy::CColorBar_>( &m_wndColorBar );

		if ( pParentMenuBtn != nullptr )
		{	// keep the order of calls:
			m_pColorTable = pParentMenuBtn->GetColorTable();
			SetColorEditorHost( pParentMenuBtn->GetEditorHost() );
		}
	}

	CColorPopupMenu::CColorPopupMenu( CMFCColorButton* pParentPickerBtn,
									  const mfc::TColorArray& colors, COLORREF color,
									  const TCHAR* pAutoColorLabel, const TCHAR* pMoreColorLabel, const TCHAR* pDocColorsLabel,
									  mfc::TColorList& docColors, int columns, COLORREF colorAuto )
		: CMFCColorPopupMenu( pParentPickerBtn, colors, color, pAutoColorLabel, pMoreColorLabel, pDocColorsLabel, docColors, columns, colorAuto )
		, m_pEditorHost( nullptr )
		, m_pColorTable( nullptr )
		, m_pDocColorTable( nullptr )
		, m_rawAutoColor( CLR_NONE )
		, m_rawSelColor( CLR_NONE )
	{	// color picker constructor (uses the selected color table)
		m_pColorBar = mfc::nosy_cast<nosy::CColorBar_>( &m_wndColorBar );
		m_bEnabledInCustomizeMode = pParentPickerBtn->m_bEnabledInCustomizeMode;
	}

	CColorPopupMenu::~CColorPopupMenu()
	{
	}

	void CColorPopupMenu::SetColorEditorHost( ui::IColorEditorHost* pEditorHost )
	{	// called by picker button that implements ui::IColorHost interface
		m_pEditorHost = pEditorHost;

		if ( m_pEditorHost != nullptr )
		{
			m_pDocColorTable = m_pEditorHost->GetDocColorTable();

			if ( nullptr == m_pColorTable )
				m_pColorTable = m_pEditorHost->GetSelColorTable();
		}
	}

	const CColorEntry* CColorPopupMenu::FindClickedBarColorEntry( void ) const
	{
		int clickedBtnPos = m_pColorBar->HitTest( ui::GetCursorPos( m_pColorBar->GetSafeHwnd() ) );
		if ( -1 == clickedBtnPos )
			return nullptr;

		return reinterpret_cast<const CColorEntry*>( mfc::Button_GetItemData( m_pColorBar->GetButton( clickedBtnPos ) ) );
	}

	void CColorPopupMenu::StoreBtnColorEntries( void )
	{
		Range<int> btnIndex;		// ranges are [start, end) for iteration

		// for each Color button: store pointers color entry into button's m_dwdItemData
		if ( m_pColorTable != nullptr && !m_pColorBar->m_colors.IsEmpty() )
		{
			btnIndex.SetEmptyRange( m_pColorBar->HasAutoBtn() ? 1 : 0 );	// skip Automatic, if any
			btnIndex.m_end += static_cast<int>( m_pColorBar->m_colors.GetSize() );

			StoreBtnColorTableEntries( btnIndex, m_pColorTable );
		}

		// for each Document Color button: store pointers color entry
		if ( m_pDocColorTable != nullptr && !m_pColorBar->m_lstDocColors.IsEmpty() )
		{
			btnIndex.SetEmptyRange( btnIndex.m_end + 2 );				// skip Separator + Doc Label
			btnIndex.m_end += static_cast<INT>( m_pColorBar->m_lstDocColors.GetSize() );

			StoreBtnColorTableEntries( btnIndex, m_pDocColorTable );
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

	void CColorPopupMenu::StoreBtnColorTableEntries( IN OUT Range<int>& rBtnIndex, const CColorTable* pColorTable )
	{
		REQUIRE( rBtnIndex.GetSpan<size_t>() == pColorTable->GetColors().size() );		// button range matches the table size?

		for ( size_t colorPos = 0; rBtnIndex.m_start != rBtnIndex.m_end; ++rBtnIndex.m_start, ++colorPos )
			StoreButtonColorEntry( m_pColorBar->GetButton( rBtnIndex.m_start ), &pColorTable->GetColorAt( colorPos ) );
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
		mfc::Button_SetItemData( pButton, pColorEntry );
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
		const CColorEntry* pColorEntry = reinterpret_cast<const CColorEntry*>( mfc::Button_GetItemData( pButton ) );

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

	bool CColorPopupMenu::Handle_HookMessage( OUT LRESULT& rResult, const MSG& msg, const CWindowHook* /*pHook*/ )
	{
		if ( WM_LBUTTONUP == msg.message )
		{
			REQUIRE( m_pColorBar->GetSafeHwnd() == msg.hwnd );

			if ( nullptr == m_pParentBtn && m_pEditorHost != nullptr )
			{
				if ( CMFCToolBarButton* pClickedButton = mfc::ToolBar_ButtonHitTest( m_pColorBar, CPoint( msg.lParam ) ) )
				{
					if ( m_pColorBar->IsMoreBtn( pClickedButton ) )
					{
						ui::IColorEditorHost* pEditorHost = m_pEditorHost;		// store it since this will get destroyed next

						m_pColorBar->InvokeMenuCommand( 0, pClickedButton );	// destroy this popup menu the graceful way - it will delete 'this'
						pEditorHost->EditColorDialog();
						rResult = 0;
						return true;			// handled the message
					}
				}
			}
			else
			{	// no need to override: CMFCColorBar redirects properly to parent CMFCColorMenuButton the OpenColorDialog() call, in this case CColorMenuButton::OpenColorDialog()
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
		m_pColorBarHook.reset( CToolTipsHandlerHook::CreateHook( m_pColorBar, this, mfc::ToolBar_GetToolTip( m_pColorBar ) ) );
		m_pColorBarHook->SetHookHandler( this );		// for custom handling of WM_LBUTTONUP on More Color button
		return 0;
	}
}


namespace mfc
{
	// CColorTablePopupMenu implementation

	CColorTablePopupMenu::CColorTablePopupMenu( CColorMenuButton* pParentMenuBtn )
		: CMFCPopupMenu()
	{
		ASSERT_PTR( pParentMenuBtn );
		m_pParentBtn = pParentMenuBtn;
		m_pColorBar.reset( new CColorTableBar( pParentMenuBtn->GetColorTable(), pParentMenuBtn->GetEditorHost() ) );
	}

	CColorTablePopupMenu::CColorTablePopupMenu( ui::IColorEditorHost* pEditorHost )
		: CMFCPopupMenu()
	{
		ASSERT_PTR( pEditorHost );
		m_pColorBar.reset( new CColorTableBar( pEditorHost->GetSelColorTable(), pEditorHost ) );

		m_pColorBar->StoreParentPicker( dynamic_cast<CMFCColorButton*>( pEditorHost->GetHostWindow() ) );		// changes the modeless popup behaviour
	}

	CColorTablePopupMenu::~CColorTablePopupMenu()
	{
	}

	CMFCPopupMenuBar* CColorTablePopupMenu::GetMenuBar( void ) overrides( CMFCPopupMenuBar )
	{
		return m_pColorBar.get();
	}

	BOOL CColorTablePopupMenu::OnCmdMsg( UINT btnId, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) overrides( CMFCPopupMenu )
	{
		if ( m_pColorBar->IsColorBtnId( btnId ) )		// note: color buttons status and command processing is managed internally
			return true;		// so prevent disabling while tracking and handling CColorTableBar::OnUpdateCmdUI() updates

		return __super::OnCmdMsg( btnId, code, pExtra, pHandlerInfo );
	}

	// message handlers

	BEGIN_MESSAGE_MAP( CColorTablePopupMenu, CMFCPopupMenu )
		ON_WM_CREATE()
	END_MESSAGE_MAP()

	int CColorTablePopupMenu::OnCreate( CREATESTRUCT* pCreateStruct )
	{
		int result = 0;

		if ( m_bTrackMode )			// popup in modal tracking mode via CContextMenuManager::TrackPopupMenu()?
			result = __super::OnCreate( pCreateStruct );
		else
		{	// verbatim from CMFCColorBar::OnCreate():
			result = CMiniFrameWnd::OnCreate( pCreateStruct );

			if ( 0 == result )		// creation succeeded?
				if ( m_pColorBar->Create( this, ToolBarStyle, ToolBarId ) )
				{
					CWnd* pWndParent = GetParent();
					ASSERT_PTR( pWndParent->GetSafeHwnd() );

					m_pColorBar->SetOwner( pWndParent );

					ActivatePopupMenu( AFXGetTopLevelFrame( pWndParent ), this );
					RecalcLayout();
				}
				else
				{
					ASSERT( false );		// * can't create the color table popup menu bar!
					result = -1;
				}
		}

		return result;
	}


	// CColorTableBar implementation

	CColorTableBar::CColorTableBar( const CColorTable* pColorTable, ui::IColorEditorHost* pEditorHost )
		: CMFCPopupMenuBar()
		, m_pColorTable( pColorTable )
		, m_pEditorHost( pEditorHost )
		, m_columnCount( 0 )
		, m_pParentPickerButton( nullptr )
		, m_isModelessPopup( false )
	{
		ASSERT_PTR( m_pColorTable );
		ASSERT_PTR( m_pEditorHost );

		m_columnCount = pColorTable->IsSysColorTable() ? 2 : pColorTable->GetColumnCount();

		// customize menu bar aspect
		m_bIsDlgControl = true;					// stretch separators to the entire bar width
		m_bDisableSideBarInXPMode = true;		// disable filling the image gutter: blue-gray zone on bar left
	}

	CColorTableBar::~CColorTableBar()
	{
	}

	void CColorTableBar::SetupButtons( void )
	{
		if ( nullptr == GetSafeHwnd() )
			return;

		RemoveAllButtons();

		COLORREF selColor = m_pEditorHost->GetColor();
		bool isSelTable = m_pColorTable == m_pEditorHost->GetSelColorTable();	// selected table?
		bool isForeignColor = selColor != CLR_NONE && !m_pColorTable->ContainsColor( selColor );
		CToolBarColorButton* pColorButton = nullptr;

		if ( isSelTable && !ui::IsUndefinedColor( m_pEditorHost->GetAutoColor() ) )
		{
			InsertButton( pColorButton = new CToolBarColorButton( AutoId, m_pEditorHost->GetAutoColor(), mfc::CColorLabels::s_autoLabel ) );
			pColorButton->SetChecked( CLR_NONE == selColor );
			InsertSeparator();
		}

		for ( UINT i = 0; i != m_pColorTable->GetColors().size(); ++i )
		{
			InsertButton( pColorButton = new CToolBarColorButton( ColorIdMin + i, &m_pColorTable->GetColorAt( i ) ) );
			pColorButton->UpdateSelectedColor( selColor );
		}

		if ( isSelTable )
		{
			InsertSeparator();
			InsertButton( pColorButton = new CToolBarColorButton( MoreColorsId, isForeignColor ? m_pEditorHost->GetActualColor() : CLR_NONE, mfc::CColorLabels::s_moreLabel ) );

			if ( isForeignColor )
				pColorButton->UpdateSelectedColor( selColor );
		}
	}

	bool CColorTableBar::IsColorBtnId( UINT btnId ) const
	{
		return btnId >= AutoId && btnId < ColorIdMin + m_pColorTable->GetColors().size();
	}

	void CColorTableBar::AdjustLocations( void ) overrides( CMFCPopupMenuBar )
	{
		__super::AdjustLocations();
	}

	BOOL CColorTableBar::OnSendCommand( const CMFCToolBarButton* pButton ) overrides( CMFCPopupMenuBar )
	{
		if ( m_pParentPickerButton != nullptr )
			ReleaseCapture();

		int btnId = static_cast<int>( pButton->m_nID );
		COLORREF newColor = CLR_DEFAULT;
		ui::IColorEditorHost* pEditorHost = m_pEditorHost;			// store since 'this' will get deleted

		if ( IsColorBtnId( btnId ) )
		{
			newColor = AutoId == btnId ? CLR_NONE : checked_static_cast<const CToolBarColorButton*>( pButton )->GetColor();

			m_pEditorHost->SwitchSelColorTable( m_pColorTable );	// select the new table
		}

		if ( !__super::OnSendCommand( pButton ) )		// will destroy this popup menu the graceful way - it will delete 'this'
			return FALSE;

		if ( MoreColorsId == btnId )
			pEditorHost->EditColorDialog();
		else if ( newColor != CLR_DEFAULT )
			pEditorHost->SetColor( newColor, true );

		return TRUE;
	}

	// message handlers

	BEGIN_MESSAGE_MAP( CColorTableBar, CMFCPopupMenuBar )
		ON_WM_CREATE()
	END_MESSAGE_MAP()

	int CColorTableBar::OnCreate( CREATESTRUCT* pCreateStruct )
	{
		if ( -1 == __super::OnCreate( pCreateStruct ) )
			return -1;

		const CMFCPopupMenu* pParentPopupMenu = dynamic_cast<const CMFCPopupMenu*>( GetParent() );
		m_isModelessPopup = nullptr == pParentPopupMenu || !mfc::PopupMenu_InTrackMode( pParentPopupMenu );

		SetupButtons();

		// Note: support for modeless execution is not fully implemented (but fairly functional), so modal tracking should be the preferred way to use this.
		//
		if ( m_isModelessPopup )	// modeless popup mode?
			SetCapture();			// note: should not be called in menu popup tracking, since it freezes other sibling popups that need expanding

		if ( m_pParentPickerButton != NULL )
			mfc::MfcButton_SetCaptured( m_pParentPickerButton, false );

		return 0;
	}
}
