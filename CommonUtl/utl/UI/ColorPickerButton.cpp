
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


namespace nosy
{
	struct CColorBar_ : public CMFCColorBar
	{
		// public access
		using CMFCColorBar::InitColors;
		using CMFCColorBar::m_bInternal;
	};
}


namespace reg
{
	static const TCHAR section_picker[] = _T("Settings\\ColorPicker");			// default section
	static const TCHAR entry_ColorTableName[] = _T("ColorTableName");
	static const TCHAR entry_HalftoneSize[] = _T("HalftoneSize");
}


// CColorPickerButton implementation

std::vector<CColorPickerButton*> CColorPickerButton::s_instances;

CColorPickerButton::CColorPickerButton( void )
	: CMFCColorButton()
	, m_pColorTable( nullptr )
{
	Construct();
}

CColorPickerButton::CColorPickerButton( ui::StdColorTable tableType )
	: CMFCColorButton()
	, m_pColorTable( nullptr )
{
	Construct();
	SetColorTable( tableType );
}

CColorPickerButton::~CColorPickerButton()
{
	std::vector<CColorPickerButton*>::iterator itThis = std::find( s_instances.begin(), s_instances.end(), this );		// for color table notifications
	ASSERT( itThis != s_instances.end() );
	s_instances.erase( itThis );
}

void CColorPickerButton::Construct( void )
{
	m_pColorTable = nullptr;
	m_halftoneSize = 0;
	m_useUserColors = false;
	m_regSection = reg::section_picker;
	m_accel.Load( IDR_EDIT_ACCEL );

	EnableOtherButton( _T("More...") );

	REQUIRE( !utl::Contains( s_instances, this ) );
	s_instances.push_back( this );
}

void CColorPickerButton::SetColors( const std::vector<COLORREF>& colors )
{
	m_Colors.SetSize( colors.size() );
	utl::Copy( colors.begin(), colors.end(), m_Colors.GetData() );

	m_pColorTable = nullptr;
	m_halftoneSize = 0;
}

void CColorPickerButton::SetHalftoneColors( size_t halftoneSize /*= 256*/ )
{
	std::vector<COLORREF> halftoneColors;

	ui::MakeHalftoneColorTable( halftoneColors, halftoneSize );
	SetColors( halftoneColors );

	switch ( halftoneSize )
	{
		case 16:	SetColumnsNumber( 8 ); break;
		case 256:	SetColumnsNumber( 16 ); break;
		default:	SetColumnsNumber( 0 ); break;
	}

	m_pColorTable = nullptr;
	m_halftoneSize = halftoneSize;
}

bool CColorPickerButton::SetColorTable( const CColorTable* pColorTable )
{
	ASSERT_PTR( pColorTable );
	if ( m_pColorTable == pColorTable )
		return false;

	m_pColorTable = pColorTable;
	m_halftoneSize = 0;

	m_Colors.RemoveAll();
	pColorTable->QueryMfcColors( m_Colors );
	pColorTable->RegisterColorButtonNames();

	SetColumnsNumber( pColorTable->GetColumnCount() );
	return true;
}

bool CColorPickerButton::SetColorTable( ui::StdColorTable tableType )
{
	return SetColorTable( CColorRepository::Instance()->FindTable( tableType ) );
}

void CColorPickerButton::SetDocumentColors( const CColorTable* pColorTable, const TCHAR* pDocLabel /*= nullptr*/ )
{
	ui::TMFCColorList docColors;

	if ( pColorTable != nullptr )
	{
		pColorTable->QueryMfcColors( docColors );
		pColorTable->RegisterColorButtonNames();

		if ( str::IsEmpty( pDocLabel ) )
			pDocLabel = pColorTable->GetTableName().c_str();
	}

	__super::SetDocumentColors( pDocLabel, docColors );
}

void CColorPickerButton::AddColorTablesSubMenu( CMenu* pContextMenu )
{
	ASSERT( ::IsMenu( pContextMenu->GetSafeHmenu() ) );

	int index = 0;

	if ( CMenu* pSetSubMenu = ui::SafeFromHandle( ui::FindMenuItemIndex( &index, pContextMenu->GetSafeHmenu(), ID_REPO_COLOR_TABLE_MIN ) ) )
	{
		pSetSubMenu->DeleteMenu( index, MF_BYPOSITION );

		// add color tables registered in the color repository
		const std::vector<CColorTable*>& colorTables = CColorRepository::Instance()->GetTables();

		for ( UINT i = 0; i != colorTables.size(); ++i )
			pSetSubMenu->InsertMenu( index + i, MF_STRING | MF_BYPOSITION, ID_REPO_COLOR_TABLE_MIN + i, colorTables[i]->GetTableName().c_str() );
	}
}

void CColorPickerButton::LoadFromRegistry( void )
{
	const CColorTable* pColorTable = m_pColorTable;
	size_t halftoneSize = m_halftoneSize;

	if ( !m_regSection.empty() )
	{
		CWinApp* pApp = AfxGetApp();
		std::tstring colorTableName;

		if ( m_pColorTable != nullptr )
			colorTableName = m_pColorTable->GetTableName();

		colorTableName = pApp->GetProfileString( m_regSection.c_str(), reg::entry_ColorTableName, colorTableName.c_str() ).GetString();
		halftoneSize = pApp->GetProfileInt( m_regSection.c_str(), reg::entry_HalftoneSize, static_cast<int>( halftoneSize ) );

		if ( !colorTableName.empty() )
		{
			if ( const CColorTable* pRegColorTable = CColorRepository::Instance()->FindTableByName( colorTableName ) )
				pColorTable = pRegColorTable;
			else
				pColorTable = CColorRepository::Instance()->FindTable( ui::Office2003_Colors );		// fallback to Office 2003 table

			REQUIRE( 0 == halftoneSize );
			halftoneSize = 0;
		}
	}

	if ( nullptr == pColorTable && 0 == halftoneSize && m_Colors.IsEmpty() )	// not initialized?
		halftoneSize = 16;			// fallback to halftone 16 colors

	if ( pColorTable != nullptr )
		SetColorTable( pColorTable );
	else if ( halftoneSize != m_halftoneSize )
		SetHalftoneColors( halftoneSize );
}

void CColorPickerButton::SaveToRegistry( void ) const
{
	ASSERT( nullptr == m_pColorTable || 0 == m_halftoneSize );		// consistent?

	if ( !m_regSection.empty() )
	{
		CWinApp* pApp = AfxGetApp();
		std::tstring colorTableName;

		if ( m_pColorTable != nullptr )
			colorTableName = m_pColorTable->GetTableName();

		pApp->WriteProfileString( m_regSection.c_str(), reg::entry_ColorTableName, colorTableName.c_str() );
		pApp->WriteProfileInt( m_regSection.c_str(), reg::entry_HalftoneSize, static_cast<int>( m_halftoneSize ) );
	}
}

void CColorPickerButton::NotifyColorTableChanged( void )
{
	for ( std::vector<CColorPickerButton*>::const_iterator itPicker = s_instances.begin(); itPicker != s_instances.end(); ++itPicker )
		if ( *itPicker != this )
			if ( !(*itPicker)->m_regSection.empty() && !(*itPicker)->m_useUserColors )
				if ( m_pColorTable != nullptr )
					(*itPicker)->SetColorTable( m_pColorTable );
				else if ( m_halftoneSize != 0 )
					(*itPicker)->SetHalftoneColors( m_halftoneSize );
}

void CColorPickerButton::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override
{
	cmdId, pTooltip;
	COLORREF color = GetColor();

	if ( m_pColorTable != nullptr )
		if ( const CColorEntry* pColorEntry = m_pColorTable->FindEvaluatedColor( color ) )
		{
			rText = pColorEntry->FormatColor();
			return;
		}

	rText = ui::FormatColor( color );
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

	__super::OnShowColorPopup();
}

void CColorPickerButton::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	m_useUserColors = nullptr == m_pColorTable && 0 == m_halftoneSize && !( m_Colors.IsEmpty() || m_lstDocColors.IsEmpty() );

	if ( !m_useUserColors )
		LoadFromRegistry();
}

BOOL CColorPickerButton::PreTranslateMessage( MSG* pMsg )
{
	if ( m_accel.Translate( pMsg, this->m_hWnd ) )
		return TRUE;

	return __super::PreTranslateMessage( pMsg );
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
	ON_COMMAND_RANGE( ID_HALFTONE_TABLE_16, ID_USER_CUSTOM_COLOR_SET, On_HalftoneTable )
	ON_UPDATE_COMMAND_UI_RANGE( ID_HALFTONE_TABLE_16, ID_USER_CUSTOM_COLOR_SET, OnUpdate_HalftoneTable )
	ON_COMMAND_RANGE( ID_REPO_COLOR_TABLE_MIN, ID_REPO_COLOR_TABLE_MAX, On_UseColorTable )
	ON_UPDATE_COMMAND_UI_RANGE( ID_REPO_COLOR_TABLE_MIN, ID_REPO_COLOR_TABLE_MAX, OnUpdate_UseColorTable )
END_MESSAGE_MAP()

void CColorPickerButton::OnDestroy( void )
{
	if ( !m_useUserColors )
		SaveToRegistry();

	__super::OnDestroy();
}

void CColorPickerButton::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( this == pWnd )
	{
		CMenu popupMenu;

		ui::LoadMfcPopupMenu( &popupMenu, IDR_STD_CONTEXT_MENU, ui::ColorPickerPopup );
		AddColorTablesSubMenu( &popupMenu );

		ui::TrackMfcPopupMenu( popupMenu.GetSafeHmenu(), this, screenPos );
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
		SetColor( newColor );
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
	if ( m_pColorTable != nullptr )
	{
		stream::Tag( tabbedText, str::Format( _T("Color Table: %s  [%d colors]"), m_pColorTable->GetTableName().c_str(), m_Colors.GetSize() ), s_lineEnd );
		stream::Tag( tabbedText, s_header[ ui::WindowsSys_Colors == m_pColorTable->GetTableType() ? WindowsSysColor : NamedColor ], s_lineEnd );

		const std::vector<CColorEntry>& colorEntries = m_pColorTable->GetColors();

		for ( size_t i = 0; i != colorEntries.size(); ++i )
			stream::Tag( tabbedText, num::FormatNumber( i + 1 ) + s_tab + colorEntries[ i ].FormatColor( s_tab, false ), s_lineEnd );		// 1-based indexMin
	}
	else
	{
		stream::Tag( tabbedText, str::Format( _T("%s [%d colors]"), m_halftoneSize != 0 ? _T("User") : _T("Halftone"), m_Colors.GetSize()), s_lineEnd);
		stream::Tag( tabbedText, s_header[ RgbColor ], s_lineEnd );

		for ( INT_PTR i = 0; i != m_Colors.GetSize(); ++i )
			stream::Tag( tabbedText, num::FormatNumber( i + 1 ) + s_tab + ui::FormatColor( m_Colors[ i ], s_tab ), s_lineEnd );		// 1-based indexMin
	}

	CTextClipboard::CopyText( tabbedText, m_hWnd );
}

void CColorPickerButton::On_HalftoneTable( UINT cmdId )
{
	switch ( cmdId )
	{
		case ID_HALFTONE_TABLE_16:
			SetHalftoneColors( 16 );
			break;
		case ID_HALFTONE_TABLE_20:
			SetHalftoneColors( 20 );
			break;
		case ID_HALFTONE_TABLE_256:
			SetHalftoneColors( 256 );
			break;
	}

	NotifyColorTableChanged();
	OnShowColorPopup();
}

void CColorPickerButton::OnUpdate_HalftoneTable( CCmdUI* pCmdUI )
{
	switch ( pCmdUI->m_nID )
	{
		case ID_HALFTONE_TABLE_16:
			pCmdUI->SetRadio( 16 == m_halftoneSize );
			break;
		case ID_HALFTONE_TABLE_20:
			pCmdUI->SetRadio( 20 == m_halftoneSize );
			break;
		case ID_HALFTONE_TABLE_256:
			pCmdUI->SetRadio( 256 == m_halftoneSize );
			break;
		case ID_USER_CUSTOM_COLOR_SET:
			pCmdUI->Enable( false );		// read-only option
			pCmdUI->SetRadio( nullptr == m_pColorTable && 0 == m_halftoneSize );
			break;
	}
}

void CColorPickerButton::On_UseColorTable( UINT cmdId )
{
	ui::StdColorTable cmdTableType = static_cast<ui::StdColorTable>( cmdId - ID_REPO_COLOR_TABLE_MIN );

	SetColorTable( cmdTableType );
	NotifyColorTableChanged();
	OnShowColorPopup();
}

void CColorPickerButton::OnUpdate_UseColorTable( CCmdUI* pCmdUI )
{
	ui::StdColorTable cmdTableType = static_cast<ui::StdColorTable>( pCmdUI->m_nID - ID_REPO_COLOR_TABLE_MIN );
	bool isCurrent = m_pColorTable != nullptr && m_pColorTable->GetTableType() == cmdTableType;

	pCmdUI->SetRadio( isCurrent );
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

	ResetPopupMenu();
}

void CColorStorePicker::SetColor( COLORREF color )
{
	m_color = color;
	// TODO: update all the sub color bars
}

void CColorStorePicker::SetSelectedColorTable( const CColorTable* pSelColorTable )
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

void CColorStorePicker::SetupPopupMenu( void )
{
	UpdateShadesTable();

	VERIFY( ui::LoadMfcPopupMenu( &m_popupMenu, IDR_STD_CONTEXT_MENU, ui::ColorStorePickerPopup ) );
	m_hMenu = m_popupMenu.GetSafeHmenu();

	ModifyPopupMenuTableItems( CHalftoneRepository::Instance() );
	ModifyPopupMenuTableItems( m_pScratchStore.get() );
	InsertPopupMenuTableItems( m_pMainStore );

	ui::CleanupMenuSeparators( &m_popupMenu );
}

void CColorStorePicker::ModifyPopupMenuTableItems( const CColorStore* pColorStore )
{
	ASSERT_PTR( pColorStore );

	// handle predefined color tables: insert items
	int index = 0;
	for ( size_t i = 0; i != pColorStore->GetTables().size(); ++i )
	{
		const CColorTable* pColorTable = pColorStore->GetTableAt( i );

		if ( UINT itemId = pColorTable->GetTableItemId() )
			if ( CMenu* pParentMenu = ui::FindMenuItemIndex( &index, &m_popupMenu, itemId ) )
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

void CColorStorePicker::InsertPopupMenuTableItems( const CColorStore* pColorStore )
{
	// handle dynamic color tables: insert items
	int index = 0;
	if ( CMenu* pParentMenu = ui::FindMenuItemIndex( &index, &m_popupMenu, ID_REPO_COLOR_TABLE_MIN ) )
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

CColorTable* CColorStorePicker::LookupPopupColorTable( UINT colorBtnId ) const
{	// based on the pointer stored in menu item dwItemData
	CColorTable* pColorTable = ui::GetMenuItemPtr<CColorTable>( m_popupMenu.GetSafeHmenu(), colorBtnId, false );
	ASSERT_PTR( pColorTable );
	return pColorTable;
}


mfc::CColorMenuButton* CColorStorePicker::FindPopupColorButton( UINT colorBtnId )
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

		pColorButton->SetColor( ui::EvalColor( m_color ), false );
		pColorButton->SetSelected();
	}

	return pColorButton;
}

void CColorStorePicker::OnCustomizeMenuBar( CMFCPopupMenu* pMenuPopup ) override
{
	ASSERT_PTR( pMenuPopup );
	REQUIRE( m_popupMenu.GetSafeHmenu() == pMenuPopup->GetHMenu() );

	CMFCPopupMenuBar* pMenuBar = pMenuPopup->GetMenuBar();

	for ( UINT index = 0, count = m_popupMenu.GetMenuItemCount(); index != count; ++index )
		if ( const CColorTable* pColorTable = ui::GetMenuItemPtr<CColorTable>( m_popupMenu, index ) )
		{
			ASSERT( !pColorTable->IsEmpty() );		// should've been excluded on popup menu set up

			UINT colorBtnId = m_popupMenu.GetMenuItemID( index );
			std::auto_ptr<mfc::CColorMenuButton> pColorButton( MakeColorMenuButton( colorBtnId, pColorTable ) );

			pMenuBar->ReplaceButton( colorBtnId, *pColorButton );
		}
}


void CColorStorePicker::OnShowMenu( void ) override
{
	SetupPopupMenu();

	if ( mfc::CContextMenuMgr::Instance() != nullptr )
		mfc::CContextMenuMgr::Instance()->ResetNewTrackingPopupMenu( new mfc::CTrackingPopupMenu( this ) );
	else
		ASSERT( false );		// the store picker requires the use of ui::CContextMenuMgr singleton set up via InitContextMenuMgr() in CApplication::InitInstance()

	__super::OnShowMenu();
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

	if ( const mfc::CColorMenuButton* pSelTableButton = FindPopupColorButton( selColorBtnId ) )
	{
		SetSelectedColorTable( pSelTableButton->GetColorTable() );
		// TODO: if table is WindowsSys_Colors, decode the clicked color by hit-testing the clicked CMFCColorBar
		UpdateColor( pSelTableButton->GetColor() );
	}
	else
		ASSERT( false );
}

void CColorStorePicker::OnUpdate_ColorTable( CCmdUI* pCmdUI )
{
	// For menu tracking, this is called on 3 cases:
	//	1) for the m_popupMenu item: pCmdUI->m_pOther = nullptr
	//	2) for the store item: pCmdUI->m_pOther = CMFCPopupMenuBar*;
	//	3) for the embedded color bar when it pops-up: pCmdUI->m_pOther = CMFCColorBar*.

	//CColorTable* pColorTable = LookupPopupColorTable( pCmdUI->m_nID );
	pCmdUI->Enable();		// prevent disabling the color buttons by CMFCColorBar::OnUpdateCmdUI()
}
