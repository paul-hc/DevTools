
#include "pch.h"
#include "ColorPickerButton.h"
#include "ColorRepository.h"
#include "CmdUpdate.h"
#include "PopupMenus.h"
#include "ContextMenuMgr.h"
#include "MenuUtilities.h"
#include "WndUtils.h"
#include "resource.h"
#include "utl/Algorithms.h"
#include "utl/EnumTags.h"
#include "utl/StringUtilities.h"
#include "utl/TextClipboard.h"
#include <math.h>

#include <afxcolorpopupmenu.h>
#include <afxcolorbar.h>
#include <afxtoolbarmenubutton.h>
#include <afxcolormenubutton.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	void DDX_ColorText( CDataExchange* pDX, int ctrlId, COLORREF* pColor, bool doInput /*= false*/ )
	{
		HWND hCtrl = pDX->PrepareEditCtrl( ctrlId );
		ASSERT_PTR( pColor );
		ASSERT_PTR( hCtrl );

		if ( DialogOutput == pDX->m_bSaveAndValidate )
			ui::SetWindowText( hCtrl, ui::FormatColor( *pColor ) );
		else if ( doInput && ui::IsWriteableEditBox( hCtrl ) )
		{
			std::tstring text = ui::GetWindowText( hCtrl );
			if ( !ui::ParseColor( pColor, text.c_str() ) )
				ui::ddx::FailInput( pDX, ctrlId, str::Format( _T("Error parsing color text: '%s'"), text.c_str() ) );
		}
	}

	void DDX_ColorRepoText( CDataExchange* pDX, int ctrlId, COLORREF color )
	{
		HWND hCtrl = pDX->PrepareEditCtrl( ctrlId );
		ASSERT_PTR( hCtrl );

		if ( DialogOutput == pDX->m_bSaveAndValidate )
			ui::SetWindowText( hCtrl, CColorRepository::Instance()->FormatColorMatch( color ) );
	}
}


class CColorMenuTrackingImpl : public CCmdTarget		// tracks a mfc::CColorPopupMenu as context menu drop-down
	, public ui::ICustomPopupMenu
{
public:
	CColorMenuTrackingImpl( ui::IColorEditorHost* pHost );
	virtual ~CColorMenuTrackingImpl();

	void SetupMenu( bool isContextMenu );

	CColorTable* LookupMenuColorTable( UINT colorBtnId ) const;
private:
	void ModifyMenuTableItems( const CColorStore* pColorStore );
	void InsertMenuTableItems( const CColorStore* pColorStore );

	mfc::CColorMenuButton* MakeColorMenuButton( UINT colorBtnId, const CColorTable* pColorTable ) const;
	static mfc::CColorMenuButton* FindColorMenuButton( UINT colorBtnId );

	// ui::ICustomPopupMenu interface
	virtual void OnCustomizeMenuBar( CMFCPopupMenu* pMenuPopup ) override;
private:
	ui::IColorEditorHost* m_pHost;
public:
	const CColorStore* m_pMainStore;		// by default CColorRepository::Instance(); enumerates the main ID_REPO_COLOR_TABLE_MIN/MAX color tables
	std::auto_ptr<CScratchColorStore> m_pScratchStore;

	CMenu m_menu;							// template for the tracking CMFCPopupMenu (created on the fly), or context menu for right-click tracking

	// generated stuff
protected:
	afx_msg void On_ColorSelected( UINT selColorBtnId );
	afx_msg void OnUpdate_ColorTable( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


CColorMenuTrackingImpl::CColorMenuTrackingImpl( ui::IColorEditorHost* pHost )
	: m_pHost( pHost )
	, m_pMainStore( CColorRepository::Instance() )
	, m_pScratchStore( new CScratchColorStore() )
{
	ASSERT_PTR( pHost );
}

CColorMenuTrackingImpl::~CColorMenuTrackingImpl()
{
}

void CColorMenuTrackingImpl::SetupMenu( bool isContextMenu )
{
	enum SubPopup { TrackingMenu, ContextMenu };
	VERIFY( ui::LoadMfcPopupMenu( &m_menu, IDR_STD_CONTEXT_MENU, ui::CPopupIndexPath( ui::ColorPickerPopup, isContextMenu ? ContextMenu : TrackingMenu ) ) );

	ModifyMenuTableItems( CHalftoneRepository::Instance() );
	ModifyMenuTableItems( m_pScratchStore.get() );
	InsertMenuTableItems( m_pMainStore );

	ui::CleanupMenuSeparators( &m_menu );
}

void CColorMenuTrackingImpl::ModifyMenuTableItems( const CColorStore* pColorStore )
{
	ASSERT_PTR( pColorStore );

	// handle predefined color tables: insert items
	int index = 0;
	for ( size_t i = 0; i != pColorStore->GetTables().size(); ++i )
	{
		const CColorTable* pColorTable = pColorStore->GetTableAt( i );

		if ( UINT itemId = pColorTable->GetTableItemId() )
			if ( CMenu* pParentMenu = ui::FindMenuItemIndex( &index, &m_menu, itemId ) )
			{
				if ( !pColorTable->IsEmpty() )
				{
					if ( str::HasPrefix( ui::GetMenuItemText( pParentMenu, index, MF_BYPOSITION ).c_str(), _T("<") ) )		// need standard item label?
						pParentMenu->ModifyMenu( index, MF_STRING | MF_BYPOSITION, itemId, pColorTable->GetTableName().c_str() );

					ui::SetMenuItemPtr( pParentMenu->GetSafeHmenu(), index, pColorTable );		// will get copied to CMFCToolBarButton::m_dwdItemData
				}
				else
					pParentMenu->DeleteMenu( index, MF_BYPOSITION );
			}
	}
}

void CColorMenuTrackingImpl::InsertMenuTableItems( const CColorStore* pColorStore )
{
	// handle dynamic color tables: insert items
	int index = 0;
	if ( CMenu* pParentMenu = ui::FindMenuItemIndex( &index, &m_menu, ID_REPO_COLOR_TABLE_MIN ) )
	{	// replace the dynamic store-based color table items
		pParentMenu->DeleteMenu( index, MF_BYPOSITION );

		if ( pColorStore != nullptr )
			for ( size_t i = 0; i != pColorStore->GetTables().size(); ++i )
			{
				const CColorTable* pColorTable = pColorStore->GetTableAt( i );

				if ( !pColorTable->IsEmpty() )
					if ( UINT itemId = pColorTable->GetTableItemId() )
					{
						pParentMenu->InsertMenu( index + i, MF_STRING | MF_BYPOSITION, itemId, pColorTable->GetTableName().c_str() );
						ui::SetMenuItemPtr( pParentMenu->GetSafeHmenu(), index + i, pColorTable );		// will get copied to CMFCToolBarButton::m_dwdItemData
					}
			}
	}
}

CColorTable* CColorMenuTrackingImpl::LookupMenuColorTable( UINT colorBtnId ) const
{	// based on the pointer stored in menu item dwItemData
	CColorTable* pColorTable = ui::GetMenuItemPtr<CColorTable>( m_menu.GetSafeHmenu(), colorBtnId, false );
	ASSERT_PTR( pColorTable );
	return pColorTable;
}

mfc::CColorMenuButton* CColorMenuTrackingImpl::MakeColorMenuButton( UINT colorBtnId, const CColorTable* pColorTable ) const
{
	mfc::CColorMenuButton* pColorButton = new mfc::CColorMenuButton( colorBtnId, pColorTable );

	if ( pColorTable == m_pHost->GetSelColorTable() )		// button for selected table?
	{	// richer configuration on the selected color table:
		pColorButton->EnableAutomaticButton( _T("Automatic"), /*this->GetAutomaticColor()*/ color::PaleBlue );
		pColorButton->EnableOtherButton( _T("More Colors...") );

		CColorTable* pShadesTable = m_pScratchStore->GetShadesTable();

		if ( !pShadesTable->IsEmpty() )
			pColorButton->SetDocColorTable( pShadesTable );

		pColorButton->SetColor( m_pHost->GetColor(), false );
		pColorButton->SetSelected();
	}

	return pColorButton;
}

mfc::CColorMenuButton* CColorMenuTrackingImpl::FindColorMenuButton( UINT colorBtnId )
{
	return checked_static_cast<mfc::CColorMenuButton*>( mfc::CTrackingPopupMenu::FindTrackingBarButton( colorBtnId ) );
}

void CColorMenuTrackingImpl::OnCustomizeMenuBar( CMFCPopupMenu* pMenuPopup )
{	// called in menu tracking mode
	ASSERT_PTR( pMenuPopup );
	REQUIRE( m_menu.GetSafeHmenu() == pMenuPopup->GetHMenu() );

	CMFCPopupMenuBar* pMenuBar = pMenuPopup->GetMenuBar();

	for ( UINT index = 0, count = m_menu.GetMenuItemCount(); index != count; ++index )
		if ( const CColorTable* pColorTable = ui::GetMenuItemPtr<CColorTable>( m_menu, index ) )
		{
			ASSERT( !pColorTable->IsEmpty() );		// should've been excluded on popup menu set up

			UINT colorBtnId = m_menu.GetMenuItemID( index );
			std::auto_ptr<mfc::CColorMenuButton> pColorButton( MakeColorMenuButton( colorBtnId, pColorTable ) );

			pMenuBar->ReplaceButton( colorBtnId, *pColorButton );
		}
}

// message handlers

BEGIN_MESSAGE_MAP( CColorMenuTrackingImpl, CCmdTarget )
	ON_CONTROL_RANGE( mfc::CColorMenuButton::CMBN_COLORSELECTED, ID_HALFTONE_TABLE_16, ID_REPO_COLOR_TABLE_MAX, On_ColorSelected )
	ON_UPDATE_COMMAND_UI_RANGE( ID_HALFTONE_TABLE_16, ID_REPO_COLOR_TABLE_MAX, OnUpdate_ColorTable )
END_MESSAGE_MAP()

void CColorMenuTrackingImpl::On_ColorSelected( UINT selColorBtnId )
{
	// Called when a color bar button (CMFCToolBarColorButton) is clicked in one of the mfc::CColorMenuButton sub-menus.
	//	selColorBtnId: the ID of the mfc::CColorMenuButton that pops-up the tracking color bar

	if ( mfc::CColorMenuButton* pSelTableButton = FindColorMenuButton( selColorBtnId ) )
	{
		m_pHost->SetColor( pSelTableButton->GetRawColor(), true );

		// if using user custom color table, color can be picked from any table, but the selected user table is retained (not switched to picked table)
		if ( !m_pHost->UseUserColors() )
			m_pHost->SetSelColorTable( pSelTableButton->GetColorTable() );
	}
	else
		ASSERT( false );
}

void CColorMenuTrackingImpl::OnUpdate_ColorTable( CCmdUI* pCmdUI )
{
	// For menu tracking, this is called on 3 cases:
	//	1) for the m_menu item: pCmdUI->m_pOther = nullptr
	//	2) for the store item: pCmdUI->m_pOther = CMFCPopupMenuBar*;
	//	3) for the embedded color bar when it pops-up: pCmdUI->m_pOther = CMFCColorBar*.

	pCmdUI->Enable();		// prevent disabling the color buttons by CMFCColorBar::OnUpdateCmdUI()
}


namespace reg
{
	static const TCHAR section_picker[] = _T("Settings\\ColorPicker");			// default section
	static const TCHAR entry_ColorTableName[] = _T("ColorTableName");
	static const TCHAR entry_PickingMode[] = _T("PickingMode");
}


// CColorPickerButton implementation

std::vector<CColorPickerButton*> CColorPickerButton::s_instances;

CColorPickerButton::CColorPickerButton( const CColorTable* pSelColorTable /*= nullptr*/ )
	: CMFCColorButton()
	, m_pSelColorTable( nullptr )
	, m_pickingMode( PickColorBar )
	, m_pDocColorTable( nullptr )
	, m_regSection( reg::section_picker )
	, m_accel( IDR_EDIT_ACCEL )
	, m_trackingMode( NoTracking )
{
	REQUIRE( !utl::Contains( s_instances, this ) );
	s_instances.push_back( this );

	m_pMenuImpl.reset( new CColorMenuTrackingImpl( this ) );

	EnableOtherButton( _T("More...") );

	SetSelColorTable( pSelColorTable != nullptr ? pSelColorTable : CHalftoneRepository::Instance()->FindTable( ui::Halftone16_Colors ) );
}

CColorPickerButton::~CColorPickerButton()
{
	std::vector<CColorPickerButton*>::iterator itThis = std::find( s_instances.begin(), s_instances.end(), this );		// for color table notifications

	ASSERT( itThis != s_instances.end() );
	s_instances.erase( itThis );
}

const CColorStore* CColorPickerButton::GetMainStore( void ) const
{
	return m_pMenuImpl->m_pMainStore;
}

void CColorPickerButton::SetMainStore( const CColorStore* pMainStore )
{
	if ( m_pMenuImpl->m_pMainStore == pMainStore )
		return;			// no change

	m_pMenuImpl->m_pMainStore = pMainStore;

	if ( nullptr == m_pSelColorTable && m_pMenuImpl->m_pMainStore != nullptr )
		SetSelColorTable( m_pMenuImpl->m_pMainStore->GetTables().front() );
}

bool CColorPickerButton::UseUserColors( void ) const
{
	return ( nullptr == m_pSelColorTable || ui::UserCustom_Colors == m_pSelColorTable->GetTableType() ) && !m_Colors.IsEmpty();
}

void CColorPickerButton::SetUserColors( const std::vector<COLORREF>& userColors, int columnCount /*= 0*/ )
{
	m_Colors.SetSize( userColors.size() );
	utl::Copy( userColors.begin(), userColors.end(), m_Colors.GetData() );

	CColorTable* pUserCustomTable = m_pMenuImpl->m_pScratchStore->GetUserCustomTable();
	pUserCustomTable->SetupMfcColors( m_Colors, columnCount );

	m_pSelColorTable = nullptr;		// to force a setup
	SetSelColorTable( pUserCustomTable );
}

const CColorEntry* CColorPickerButton::GetRawColor( void ) const
{
	// TODO...
	return nullptr;
}

void CColorPickerButton::SetColor( COLORREF rawColor, bool notify /*= false*/ )
{
	COLORREF color = ui::EvalColor( rawColor );

	if ( notify && m_hWnd != nullptr )
		UpdateColor( color );
	else
		CMFCColorButton::SetColor( color );

	UpdateShadesTable();
}

void CColorPickerButton::UpdateShadesTable( void )
{
	CColorTable* pShadesTable = m_pMenuImpl->m_pScratchStore->GetShadesTable();

	pShadesTable->SetupShadesTable( GetColor(), m_pSelColorTable != nullptr ? m_pSelColorTable->GetColumnCount() : 8 );

	ui::TMFCColorList shadesColorList;
	pShadesTable->QueryMfcColors( shadesColorList );
	SetDocumentColors( pShadesTable->IsEmpty() ? nullptr : pShadesTable->GetTableName().c_str(), shadesColorList );
}

void CColorPickerButton::SetSelColorTable( const CColorTable* pSelColorTable )
{
	ASSERT_PTR( pSelColorTable );
	if ( m_pSelColorTable == pSelColorTable )
		return;

	m_pSelColorTable = pSelColorTable;
	m_Colors.RemoveAll();

	if ( m_pSelColorTable != nullptr )
	{
		pSelColorTable->QueryMfcColors( m_Colors );
		pSelColorTable->RegisterNamesToColorButtons();

		SetColumnsNumber( pSelColorTable->GetColumnCount() );
	}
}

void CColorPickerButton::SetDocColorTable( const CColorTable* pDocColorTable )
{
	ui::TMFCColorList docColors;
	const TCHAR* pDocLabel = nullptr;

	m_pDocColorTable = pDocColorTable;
	if ( m_pDocColorTable != nullptr && !m_pDocColorTable->IsEmpty() )
	{
		m_pDocColorTable->QueryMfcColors( docColors );
		m_pDocColorTable->RegisterNamesToColorButtons();
		pDocLabel = m_pDocColorTable->GetTableName().c_str();
	}

	SetDocumentColors( pDocLabel, docColors );
}

void CColorPickerButton::LoadFromRegistry( void )
{
	REQUIRE( !UseUserColors() );		// there is no point in switching color tables for user-specified colors
	const CColorTable* pSelColorTable = m_pSelColorTable;

	if ( !m_regSection.empty() )
	{
		CWinApp* pApp = AfxGetApp();
		std::tstring colorTableName;

		if ( m_pSelColorTable != nullptr )
			colorTableName = m_pSelColorTable->GetTableName();

		colorTableName = pApp->GetProfileString( m_regSection.c_str(), reg::entry_ColorTableName, colorTableName.c_str() ).GetString();

		if ( !colorTableName.empty() )
		{
			if ( const CColorTable* pRegColorTable = CColorRepository::Instance()->FindTableByName( colorTableName ) )
				pSelColorTable = pRegColorTable;
			else
				pSelColorTable = CHalftoneRepository::Instance()->FindTable( ui::Halftone16_Colors );
		}

		m_pickingMode = static_cast<PickingMode>( pApp->GetProfileInt( m_regSection.c_str(), reg::entry_PickingMode, m_pickingMode ) );
	}

	if ( pSelColorTable != m_pSelColorTable )
		SetSelColorTable( pSelColorTable );
}

void CColorPickerButton::SaveToRegistry( void ) const
{
	if ( !m_regSection.empty() )
	{
		CWinApp* pApp = AfxGetApp();
		std::tstring colorTableName;

		if ( m_pSelColorTable != nullptr )
			colorTableName = m_pSelColorTable->GetTableName();

		pApp->WriteProfileString( m_regSection.c_str(), reg::entry_ColorTableName, colorTableName.c_str() );
		pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_PickingMode, m_pickingMode );
	}
}

void CColorPickerButton::NotifyMatchingPickers( ChangedField field )
{
	for ( std::vector<CColorPickerButton*>::const_iterator itPicker = s_instances.begin(); itPicker != s_instances.end(); ++itPicker )
		if ( *itPicker != this )
			if ( !( *itPicker )->m_regSection.empty() && ( *itPicker )->m_regSection == m_regSection && !( *itPicker )->UseUserColors() )		// matching sections of table-based picker button
				switch ( field )
				{
					case SelColorTableChanged:
						if ( m_pSelColorTable != nullptr )
							(*itPicker)->SetSelColorTable( m_pSelColorTable );
						break;
					case PickingModeChanged:
						(*itPicker)->SetPickingMode( m_pickingMode );
						break;
					default:
						ASSERT( false );
				}
}

void CColorPickerButton::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override
{
	cmdId, pTooltip;
	COLORREF color = GetColor();

	if ( m_pSelColorTable != nullptr )
		if ( const CColorEntry* pColorEntry = m_pSelColorTable->FindEvaluatedColor( color ) )
		{
			rText = pColorEntry->FormatColor();
			return;
		}

	rText = ui::FormatColor( color );
}

void CColorPickerButton::TrackMenuColorTables( void )
{
	if ( TrackingMenuColorTables == m_trackingMode )
		return;

	m_trackingMode = TrackingMenuColorTables;
	mfc::CContextMenuMgr::Instance()->ResetNewTrackingPopupMenu( new mfc::CTrackingPopupMenu( m_pMenuImpl.get() ) );

	CRect btnScreenRect;
	GetWindowRect( &btnScreenRect );

	CPoint screenPos( btnScreenRect.left, btnScreenRect.bottom );

	ui::TrackMfcPopupMenu( m_pMenuImpl->m_menu.GetSafeHmenu(), this, screenPos, false );		// no WM_COMMAND, notifications work via mfc::CColorMenuButton::CMBN_COLORSELECTED

	m_trackingMode = NoTracking;

	// CMFCColorButton boilerplate:
	m_bPushed = FALSE;
	m_bHighlighted = FALSE;

	Invalidate();
	UpdateWindow();

	if ( m_bCaptured )
	{
		ReleaseCapture();
		m_bCaptured = FALSE;
	}
}


// base overrides:

void CColorPickerButton::OnShowColorPopup( void ) override
{
	if ( -1 == m_nColumns )		// auto-layout for columns?
	{
		size_t colorCount = m_Colors.GetSize();

		if ( 16 == colorCount )
			m_nColumns = 8;
		else if ( colorCount >= 64 )
			m_nColumns = static_cast<int>( sqrt( static_cast<double>( colorCount ) ) );
	}

	// Note:
	//	- this class can't be customized with new buttons or sub-menus, since it's using CMFCToolBarColorButton defined privately in afxcolorbar.cpp
	//	- layout and drawing methods ignore buttons that are not CMFCToolBarColorButton

	m_pMenuImpl->SetupMenu( false );

	PickingMode pickingMode = m_pickingMode;
	if ( ui::IsKeyPressed( VK_CONTROL ) )
		pickingMode = PickColorBar == pickingMode ? PickMenuColorTables : PickColorBar;		// toggle picking mode (transiently)

	TrackingMode trackingMode = PickMenuColorTables == pickingMode && mfc::CContextMenuMgr::Instance() != nullptr ? TrackingMenuColorTables : TrackingColorBar;

	if ( TrackingMenuColorTables == trackingMode )
		TrackMenuColorTables();
	else
	{
		m_trackingMode = trackingMode;
		__super::OnShowColorPopup();	// modeless call
	}
}

void CColorPickerButton::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	if ( !UseUserColors() )
		LoadFromRegistry();
}

BOOL CColorPickerButton::PreTranslateMessage( MSG* pMsg )
{
	if ( m_accel.Translate( pMsg, this->m_hWnd ) )
		return TRUE;

	return __super::PreTranslateMessage( pMsg );
}

BOOL CColorPickerButton::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) override
{
	if ( TrackingMenuColorTables == m_trackingMode )
		if ( m_pMenuImpl->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
			return true;

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// message handlers

BEGIN_MESSAGE_MAP( CColorPickerButton, CMFCColorButton )
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_WM_INITMENUPOPUP()
	ON_COMMAND( ID_EDIT_COPY, OnCopy )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateEnable )
	ON_COMMAND( ID_EDIT_PASTE, OnPaste )
	ON_UPDATE_COMMAND_UI( ID_EDIT_PASTE, OnUpdatePaste )
	ON_COMMAND( ID_EDIT_LIST_ITEMS, On_CopyColorTable )
	ON_UPDATE_COMMAND_UI( ID_EDIT_LIST_ITEMS, OnUpdateEnable )
	ON_COMMAND_RANGE( ID_HALFTONE_TABLE_16, ID_REPO_COLOR_TABLE_MAX, On_SelectColorTable )
	ON_UPDATE_COMMAND_UI_RANGE( ID_HALFTONE_TABLE_16, ID_REPO_COLOR_TABLE_MAX, OnUpdate_SelectColorTable )
	ON_COMMAND_RANGE( ID_PICK_COLOR_BAR_RADIO, ID_PICK_COLOR_MENU_RADIO, On_PickingMode )
	ON_UPDATE_COMMAND_UI_RANGE( ID_PICK_COLOR_BAR_RADIO, ID_PICK_COLOR_MENU_RADIO, OnUpdate_PickingMode )
END_MESSAGE_MAP()

void CColorPickerButton::OnDestroy( void )
{
	if ( !UseUserColors() )
		SaveToRegistry();

	__super::OnDestroy();
}

void CColorPickerButton::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( this == pWnd )
	{
		m_trackingMode = TrackingContextMenu;

		m_pMenuImpl->SetupMenu( true );
		ui::TrackMfcPopupMenu( m_pMenuImpl->m_menu.GetSafeHmenu(), this, screenPos );

		m_trackingMode = NoTracking;
	}
	else
		__super::OnContextMenu( pWnd, screenPos );
}

void CColorPickerButton::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	ui::HandleInitMenuPopup( this, pPopupMenu, !isSysMenu );
	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

void CColorPickerButton::OnCopy( void )
{
	ui::CopyColor( GetColor() );
}

void CColorPickerButton::OnPaste( void )
{
	COLORREF newColor;

	if ( ui::PasteColor( &newColor ) )
		SetColor( newColor, true );
}

void CColorPickerButton::OnUpdatePaste( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( ui::CanPasteColor() );
}

void CColorPickerButton::On_CopyColorTable( void )
{
	static const std::tstring s_header[] = { _T("INDEX\tRGB\tHTML\tHEX"), _T("INDEX\tNAME\tRGB\tHTML\tHEX"), _T("INDEX\tNAME\tSYS_COLOR\tRGB\tHTML\tHEX") };
	static const TCHAR s_lineEnd[] = _T("\r\n");
	static const TCHAR s_tab[] = _T("\t");
	enum ColorType { RgbColor, NamedColor, WindowsSysColor };

	std::tstring tabbedText;

	tabbedText.reserve( ( m_Colors.GetSize() + 1 ) * 32 );
	if ( m_pSelColorTable != nullptr )
	{
		stream::Tag( tabbedText, str::Format( _T("Color Table: %s  [%d colors]"), m_pSelColorTable->GetTableName().c_str(), m_Colors.GetSize() ), s_lineEnd );
		stream::Tag( tabbedText, s_header[ ui::WindowsSys_Colors == m_pSelColorTable->GetTableType() ? WindowsSysColor : NamedColor ], s_lineEnd );

		const std::vector<CColorEntry>& colorEntries = m_pSelColorTable->GetColors();

		for ( size_t i = 0; i != colorEntries.size(); ++i )
			stream::Tag( tabbedText, num::FormatNumber( i + 1 ) + s_tab + colorEntries[ i ].FormatColor( s_tab, false ), s_lineEnd );		// 1-based indexMin
	}
	else
	{
		stream::Tag( tabbedText, str::Format( _T("%s [%d colors]"), _T("User"), m_Colors.GetSize() ), s_lineEnd );
		stream::Tag( tabbedText, s_header[ RgbColor ], s_lineEnd );

		for ( INT_PTR i = 0; i != m_Colors.GetSize(); ++i )
			stream::Tag( tabbedText, num::FormatNumber( i + 1 ) + s_tab + ui::FormatColor( m_Colors[ i ], s_tab ), s_lineEnd );		// 1-based indexMin
	}

	CTextClipboard::CopyText( tabbedText, m_hWnd );
}

void CColorPickerButton::On_SelectColorTable( UINT colorTableId )
{
	const CColorTable* pColorTable = m_pMenuImpl->LookupMenuColorTable( colorTableId );

	SetSelColorTable( pColorTable );

	NotifyMatchingPickers( SelColorTableChanged );
	OnShowColorPopup();
}

void CColorPickerButton::OnUpdate_SelectColorTable( CCmdUI* pCmdUI )
{
	const CColorTable* pColorTable = m_pMenuImpl->LookupMenuColorTable( pCmdUI->m_nID );

	pCmdUI->Enable( !UseUserColors() || ui::UserCustom_Colors == pColorTable->GetTableType() );		// user table is exclusive (can't be switched off to another table)
	pCmdUI->SetRadio( pColorTable != nullptr && pColorTable == m_pSelColorTable );
}

void CColorPickerButton::On_PickingMode( UINT pickRadioId )
{
	m_pickingMode = static_cast<PickingMode>( pickRadioId - ID_PICK_COLOR_BAR_RADIO );

	NotifyMatchingPickers( PickingModeChanged );
	OnShowColorPopup();
}

void CColorPickerButton::OnUpdate_PickingMode( CCmdUI* pCmdUI )
{
	ui::SetRadio( pCmdUI, ( pCmdUI->m_nID - ID_PICK_COLOR_BAR_RADIO ) == (UINT)m_pickingMode );
	//pCmdUI->SetRadio( ( pCmdUI->m_nID - ID_PICK_COLOR_BAR_RADIO ) == (UINT)m_pickingMode );
}

void CColorPickerButton::OnUpdateEnable( CCmdUI* pCmdUI )
{
	pCmdUI;
}


// CMenuPickerButton implementation

CMenuPickerButton::CMenuPickerButton( CWnd* pTargetWnd /*= nullptr*/ )
	: CMFCMenuButton()
	, m_pTargetWnd( pTargetWnd )
{
	m_bOSMenu = false;

	if ( nullptr == m_pTargetWnd )
		m_pTargetWnd = this;
}

CMenuPickerButton::~CMenuPickerButton()
{
}

CWnd* CMenuPickerButton::GetTargetWnd( void ) const
{
	return m_pTargetWnd != nullptr ? m_pTargetWnd : const_cast<CMenuPickerButton*>( this );
}

void CMenuPickerButton::OnShowMenu( void ) override
{
	__super::OnShowMenu();
}


// message handlers

BEGIN_MESSAGE_MAP( CMenuPickerButton, CMFCMenuButton )
	ON_WM_INITMENUPOPUP()
END_MESSAGE_MAP()

void CMenuPickerButton::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
	ui::HandleInitMenuPopup( GetTargetWnd(), pPopupMenu, !isSysMenu );		// dialog implements the CmdUI updates
}


// CColorStorePicker implementation

CColorStorePicker::CColorStorePicker( CWnd* pTargetWnd /*= nullptr*/ )
	: CMenuPickerButton( pTargetWnd )
	, m_color( CLR_NONE )
	, m_pMainStore( nullptr )
	, m_pSelColorTable( nullptr )
	, m_pScratchStore( new CScratchColorStore() )
{
	SetMainStore( CColorRepository::Instance() );
}

CColorStorePicker::~CColorStorePicker()
{
}

void CColorStorePicker::SetMainStore( const CColorStore* pMainStore )
{
	m_pMainStore = pMainStore;
	m_pSelColorTable = m_pMainStore != nullptr ? m_pMainStore->GetTableAt( 0 ) : nullptr;
}

void CColorStorePicker::SetColor( COLORREF color )
{
	m_color = color;

	UpdateShadesTable();
}

void CColorStorePicker::SetSelColorTable( const CColorTable* pSelColorTable )
{
	m_pSelColorTable = pSelColorTable;
}

void CColorStorePicker::UpdateColor( COLORREF newColor )
{
	SetColor( newColor );
	/*TEMP*/ ui::SendCommandToParent( m_hWnd, BN_CLICKED );
}

void CColorStorePicker::UpdateShadesTable( void )
{
	CColorTable* pShadesTable = m_pScratchStore->GetShadesTable();

	pShadesTable->SetupShadesTable( m_color, m_pSelColorTable != nullptr ? m_pSelColorTable->GetColumnCount() : 8 );

	// TODO: when merging into CColorPickerButton - uncomment:
	//ui::TMFCColorList shadesColorList;
	//pShadesTable->QueryMfcColors( shadesColorList );
	// SetDocumentColors( pShadesTable->GetTableName().c_str(), shadesColorList );
}

void CColorStorePicker::SetupMenu( void )
{
	VERIFY( ui::LoadMfcPopupMenu( &m_contextMenu, IDR_STD_CONTEXT_MENU, ui::ColorStorePickerPopup ) );
	m_hMenu = m_contextMenu.GetSafeHmenu();

	ModifyMenuTableItems( CHalftoneRepository::Instance() );
	ModifyMenuTableItems( m_pScratchStore.get() );
	InsertMenuTableItems( m_pMainStore );

	ui::CleanupMenuSeparators( &m_contextMenu );
}

void CColorStorePicker::ModifyMenuTableItems( const CColorStore* pColorStore )
{
	ASSERT_PTR( pColorStore );

	// handle predefined color tables: insert items
	int index = 0;
	for ( size_t i = 0; i != pColorStore->GetTables().size(); ++i )
	{
		const CColorTable* pColorTable = pColorStore->GetTableAt( i );

		if ( UINT itemId = pColorTable->GetTableItemId() )
			if ( CMenu* pParentMenu = ui::FindMenuItemIndex( &index, &m_contextMenu, itemId ) )
			{
				if ( !pColorTable->IsEmpty() )
				{
					if ( str::HasPrefix( ui::GetMenuItemText( pParentMenu, index, MF_BYPOSITION ).c_str(), _T("<") ) )		// need standard item label?
						pParentMenu->ModifyMenu( index, MF_STRING | MF_BYPOSITION, itemId, pColorTable->GetTableName().c_str() );

					ui::SetMenuItemPtr( pParentMenu->GetSafeHmenu(), index, pColorTable );		// will get copied to CMFCToolBarButton::m_dwdItemData
				}
				else
					pParentMenu->DeleteMenu( index, MF_BYPOSITION );
			}
	}
}

void CColorStorePicker::InsertMenuTableItems( const CColorStore* pColorStore )
{
	// handle dynamic color tables: insert items
	int index = 0;
	if ( CMenu* pParentMenu = ui::FindMenuItemIndex( &index, &m_contextMenu, ID_REPO_COLOR_TABLE_MIN ) )
	{	// replace the dynamic store-based color table items
		pParentMenu->DeleteMenu( index, MF_BYPOSITION );

		if ( pColorStore != nullptr )
			for ( size_t i = 0; i != pColorStore->GetTables().size(); ++i )
			{
				const CColorTable* pColorTable = pColorStore->GetTableAt( i );

				if ( !pColorTable->IsEmpty() )
					if ( UINT itemId = pColorTable->GetTableItemId() )
					{
						pParentMenu->InsertMenu( index + i, MF_STRING | MF_BYPOSITION, itemId, pColorTable->GetTableName().c_str() );
						ui::SetMenuItemPtr( pParentMenu->GetSafeHmenu(), index + i, pColorTable );		// will get copied to CMFCToolBarButton::m_dwdItemData
					}
			}
	}
}

CColorTable* CColorStorePicker::LookupMenuColorTable( UINT colorBtnId ) const
{	// based on the pointer stored in menu item dwItemData
	CColorTable* pColorTable = ui::GetMenuItemPtr<CColorTable>( m_contextMenu.GetSafeHmenu(), colorBtnId, false );
	ASSERT_PTR( pColorTable );
	return pColorTable;
}


mfc::CColorMenuButton* CColorStorePicker::FindColorMenuButton( UINT colorBtnId )
{
	return checked_static_cast<mfc::CColorMenuButton*>( mfc::CTrackingPopupMenu::FindTrackingBarButton( colorBtnId ) );
}

mfc::CColorMenuButton* CColorStorePicker::MakeColorMenuButton( UINT colorBtnId, const CColorTable* pColorTable ) const
{
	mfc::CColorMenuButton* pColorButton = new mfc::CColorMenuButton( colorBtnId, pColorTable );

	if ( pColorTable == m_pSelColorTable )		// button for selected table?
	{	// richer configuration on the selected color table:
		pColorButton->EnableAutomaticButton( _T("Automatic"), /*this->GetAutomaticColor()*/ color::PaleBlue );
		pColorButton->EnableOtherButton( _T("More Colors...") );

		CColorTable* pShadesTable = m_pScratchStore->GetShadesTable();

		if ( !pShadesTable->IsEmpty() )
			pColorButton->SetDocColorTable( pShadesTable );

		pColorButton->SetColor( m_color, false );
		pColorButton->SetSelected();
	}

	return pColorButton;
}

void CColorStorePicker::OnCustomizeMenuBar( CMFCPopupMenu* pMenuPopup ) override
{	// called in menu tracking mode
	ASSERT_PTR( pMenuPopup );
	REQUIRE( m_contextMenu.GetSafeHmenu() == pMenuPopup->GetHMenu() );

	CMFCPopupMenuBar* pMenuBar = pMenuPopup->GetMenuBar();

	for ( UINT index = 0, count = m_contextMenu.GetMenuItemCount(); index != count; ++index )
		if ( const CColorTable* pColorTable = ui::GetMenuItemPtr<CColorTable>( m_contextMenu, index ) )
		{
			ASSERT( !pColorTable->IsEmpty() );		// should've been excluded on popup menu set up

			UINT colorBtnId = m_contextMenu.GetMenuItemID( index );
			std::auto_ptr<mfc::CColorMenuButton> pColorButton( MakeColorMenuButton( colorBtnId, pColorTable ) );

			pMenuBar->ReplaceButton( colorBtnId, *pColorButton );
		}
}


void CColorStorePicker::OnShowMenu( void ) override
{
	SetupMenu();

	if ( mfc::CContextMenuMgr::Instance() != nullptr )
		mfc::CContextMenuMgr::Instance()->ResetNewTrackingPopupMenu( new mfc::CTrackingPopupMenu( this ) );
	else
		ASSERT( false );		// the store picker requires the use of ui::CContextMenuMgr singleton set up via InitContextMenuMgr() in CApplication::InitInstance()

	__super::OnShowMenu();		// modal call
}

void CColorStorePicker::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	if ( nullptr == m_pSelColorTable )
	{
		// TODO: uncomment when merging into CColorPickerButton
		//m_pScratchStore->GetUserCustomTable()->SetupMfcColors( m_Colors );

		/*TEMP*/ m_pScratchStore->GetUserCustomTable()->CopyFrom( *CHalftoneRepository::Instance()->FindTable( ui::Halftone20_Colors ) );
	}
}


// message handlers

BEGIN_MESSAGE_MAP( CColorStorePicker, CMenuPickerButton )
	ON_CONTROL_RANGE( mfc::CColorMenuButton::CMBN_COLORSELECTED, ID_HALFTONE_TABLE_16, ID_REPO_COLOR_TABLE_MAX, On_ColorSelected )
	ON_UPDATE_COMMAND_UI_RANGE( ID_HALFTONE_TABLE_16, ID_REPO_COLOR_TABLE_MAX, OnUpdate_ColorTable )
END_MESSAGE_MAP()

void CColorStorePicker::On_ColorSelected( UINT selColorBtnId )
{
	// Called when a color bar button (CMFCToolBarColorButton) is clicked in one of the mfc::CColorMenuButton sub-menus.
	//	selColorBtnId: the ID of the mfc::CColorMenuButton that pops-up the tracking color bar

	if ( mfc::CColorMenuButton* pSelTableButton = FindColorMenuButton( selColorBtnId ) )
	{
		SetSelColorTable( pSelTableButton->GetColorTable() );
		UpdateColor( pSelTableButton->GetRawColor() );
	}
	else
		ASSERT( false );
}

void CColorStorePicker::OnUpdate_ColorTable( CCmdUI* pCmdUI )
{
	// For menu tracking, this is called on 3 cases:
	//	1) for the m_contextMenu item: pCmdUI->m_pOther = nullptr
	//	2) for the store item: pCmdUI->m_pOther = CMFCPopupMenuBar*;
	//	3) for the embedded color bar when it pops-up: pCmdUI->m_pOther = CMFCColorBar*.

	//CColorTable* pColorTable = LookupMenuColorTable( pCmdUI->m_nID );
	pCmdUI->Enable();		// prevent disabling the color buttons by CMFCColorBar::OnUpdateCmdUI()
}
