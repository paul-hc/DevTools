
#include "pch.h"
#include "ColorPickerButton.h"
#include "ColorRepository.h"
#include "CmdUpdate.h"
#include "PopupMenus.h"
#include "ContextMenuMgr.h"
#include "MenuUtilities.h"
#include "CommandSvcHandler.h"
#include "UiCommands.h"
#include "WndUtils.h"
#include "resource.h"
#include "utl/Algorithms.h"
#include "utl/EnumTags.h"
#include "utl/ScopedValue.h"
#include "utl/StringUtilities.h"
#include "utl/TextClipboard.h"
#include <math.h>

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


class CColorMenuTrackingImpl
	: public CCmdTarget					// tracks a mfc::CColorPopupMenu as context menu drop-down
	, public ui::ICustomPopupMenu
{
public:
	CColorMenuTrackingImpl( ui::IColorEditorHost* pHost, const CCommandModel* pCmdModel );
	virtual ~CColorMenuTrackingImpl();

	void SetupMenu( bool isContextMenu );

	CColorTable* GetShadesTable( void ) { return m_pScratchStore->GetShadesTable(); }
	CRecentColorTable* GetRecentTable( void ) const { return m_pScratchStore->GetRecentTable(); }

	CColorTable* LookupMenuColorTable( UINT colorBtnId ) const;
private:
	void ModifyMenuTableItems( const CColorStore* pColorStore );
	void InsertMenuTableItems( const CColorStore* pColorStore );

	static mfc::CColorMenuButton* FindColorMenuButton( UINT colorBtnId );

	// ui::ICustomPopupMenu interface
	virtual void OnCustomizeMenuBar( CMFCPopupMenu* pMenuPopup, int trackingMode ) implement;

	COLORREF PeekUndoTopColor( void ) const;
	COLORREF PeekRedoTopColor( void ) const;
private:
	ui::IColorEditorHost* m_pHost;
	const CCommandModel* m_pCmdModel;
public:
	const CColorStore* m_pMainStore;		// by default CColorRepository::Instance(); enumerates the main ID_REPO_COLOR_TABLE_MIN/MAX color tables
	CScratchColorStore* m_pScratchStore;

	CMenu m_menu;							// template for the tracking CMFCPopupMenu (created on the fly), or context menu for right-click tracking

	// generated stuff
protected:
	afx_msg void On_ColorSelected( UINT selColorBtnId );
	afx_msg void OnUpdate_ColorTable( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


namespace reg
{
	static const TCHAR section_picker[] = _T("Settings\\ColorPicker");			// default section
	static const TCHAR entry_ColorTableName[] = _T("ColorTableName");
	static const TCHAR entry_PickingMode[] = _T("PickingMode");
	static const TCHAR entry_Commands[] = _T("Commands");
}


// CColorPickerButton implementation

enum PrivateIDs { IDP_COPY_COLOR_TABLE = ID_EDIT_LIST_ITEMS };

std::vector<CColorPickerButton*> CColorPickerButton::s_instances;

CColorPickerButton::CColorPickerButton( const CColorTable* pSelColorTable /*= nullptr*/ )
	: CMFCColorButton()
	, m_pSelColorTable( nullptr )
	, m_pickingMode( PickColorBar )
	, m_pSelColorEntry( nullptr )
	, m_pDocColorTable( nullptr )
	, m_regSection( reg::section_picker )
	, m_accel( IDR_EDIT_ACCEL )
	, m_trackingMode( NoTracking )
	, m_inCmd( false )
{
	REQUIRE( !utl::Contains( s_instances, this ) );
	s_instances.push_back( this );

	const ACCEL accelKeys[] =
	{
		{ FVIRTKEY, VK_DELETE, ID_SET_AUTO_COLOR },						// Delete
		{ FVIRTKEY | FCONTROL | FSHIFT, 'T', IDP_COPY_COLOR_TABLE },	// Ctrl+Shift+T
		{ FVIRTKEY | FCONTROL, 'Z', ID_EDIT_UNDO },						// Ctrl+Z
		{ FVIRTKEY | FALT, VK_BACK, ID_EDIT_UNDO },						// Alt+Backspace
		{ FVIRTKEY | FCONTROL, 'Y', ID_EDIT_REDO },						// Ctrl+Shift+Z
		{ FVIRTKEY | FCONTROL | FSHIFT, 'Z', ID_EDIT_REDO },			// Ctrl+Shift+Z
		{ FVIRTKEY | FALT | FSHIFT, VK_BACK, ID_EDIT_REDO }				// Shift+Alt+Backspace
	};
	m_accel.Augment( ARRAY_SPAN( accelKeys ) );

	m_pCmdSvc.reset( new ui::CCommandSvcHandler() );
	m_pMenuImpl.reset( new CColorMenuTrackingImpl( this, m_pCmdSvc->GetCmdModel() ) );

	EnableOtherButton( mfc::CColorLabels::s_moreLabel );

	SetSelColorTable( pSelColorTable != nullptr ? pSelColorTable : CHalftoneRepository::Instance()->FindTable( ui::Halftone16_Colors ) );
	SetDocColorTable( m_pMenuImpl->GetShadesTable() );
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

bool CColorPickerButton::UseUserColors( void ) const implement
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

void CColorPickerButton::SetColor( COLORREF rawColor, bool notify /*= false*/ ) implement
{
	if ( notify )
	{
		if ( !m_inCmd && m_pCmdSvc.get() != nullptr && utl::ExecDo == CCommandModel::GetExecMode() )
		{
			CScopedValue<bool> scInCmd( &m_inCmd, true );

			m_pCmdSvc->Execute( new CSetColorCmd( this, rawColor ) );
			return;
		}

		m_pMenuImpl->GetRecentTable()->PushColor( rawColor, GetAutoColor() );		// at stack top (most recent)
	}

	SetColorImpl( rawColor, notify );
}

void CColorPickerButton::SetColorImpl( COLORREF rawColor, bool notify )
{
	if ( notify )
	{
		if ( TrackingColorBar == m_trackingMode && m_pPopup->GetSafeHwnd() != nullptr )
		{
			m_pSelColorEntry = checked_static_cast<const mfc::CColorPopupMenu*>( m_pPopup )->FindClickedBarColorEntry();

			if ( m_pSelColorEntry != nullptr )
				rawColor = m_pSelColorEntry->GetColor();		// is a system color selected, assign the real raw color
		}
		else
			m_pSelColorEntry = nullptr;			// to be looked-up next

		if ( nullptr == m_pSelColorEntry )
			m_pSelColorEntry = m_pMenuImpl->m_pMainStore->FindColorEntry( rawColor );		// fallback to table lookup by raw color
	}

	CMFCColorButton::SetColor( rawColor );

	if ( notify && m_hWnd != nullptr )
		ui::SendCommandToParent( m_hWnd, BN_CLICKED );

	UpdateShadesTable();
}

void CColorPickerButton::UpdateColor( COLORREF color ) overrides(CMFCColorButton)
{
		//__super::UpdateColor( color );

	SetColor( color, true );
}

void CColorPickerButton::UpdateShadesTable( void )
{
	CColorTable* pShadesTable = m_pMenuImpl->GetShadesTable();

	pShadesTable->SetupShadesTable( GetColor(), ( m_pSelColorTable != nullptr && !m_pSelColorTable->IsSysColorTable() ) ? m_pSelColorTable->GetColumnCount() : 8 );

	mfc::TColorList shadesColorList;
	pShadesTable->QueryMfcColors( shadesColorList );
	SetDocumentColors( pShadesTable->IsEmpty() ? nullptr : pShadesTable->GetTableName().c_str(), shadesColorList );
}

void CColorPickerButton::SetSelColorTable( const CColorTable* pSelColorTable ) implement
{
	ASSERT_PTR( pSelColorTable );
	if ( m_pSelColorTable == pSelColorTable )
		return;

	m_pSelColorTable = pSelColorTable;
	m_Colors.RemoveAll();

	if ( m_pSelColorTable != nullptr )
	{
		SetColumnsNumber( pSelColorTable->GetColumnCount() );

		pSelColorTable->QueryMfcColors( m_Colors );

		// this has no functional impact for True Color mode, so it's not necessary:
		//delete m_pPalette;
		//VERIFY( pSelColorTable->BuildPalette( m_pPalette = new CPalette() ) );
	}
}

void CColorPickerButton::SetDocColorTable( const CColorTable* pDocColorTable )
{
	mfc::TColorList docColors;
	const TCHAR* pDocLabel = nullptr;

	m_pDocColorTable = pDocColorTable;
	if ( m_pDocColorTable != nullptr && !m_pDocColorTable->IsEmpty() )
	{
		m_pDocColorTable->QueryMfcColors( docColors );
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

		if ( m_pCmdSvc->LoadState( m_regSection.c_str(), FormatEntryCommands().c_str() ) )
		{
			m_pCmdSvc->RefCmdModel()->ReHostCommands<CSetColorCmd>( this );
			m_pMenuImpl->GetRecentTable()->PushColorHistory( m_pCmdSvc->GetCmdModel(), GetAutoColor() );
		}
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

		m_pCmdSvc->SaveState( m_regSection.c_str(), FormatEntryCommands().c_str() );
	}
}

std::tstring CColorPickerButton::FormatEntryCommands( void ) const
{
	REQUIRE( !m_regSection.empty() );

	if ( m_regSection == reg::section_picker )		// shared section?
		return str::Format( _T("%s_%d"), reg::entry_Commands, GetDlgCtrlID() );		// use unique Commands_CtrlID entries to disambiguate

	return reg::entry_Commands;
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

void CColorPickerButton::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override
{
	cmdId, pTooltip;
	std::tstring text;

	if ( text.empty() && m_pSelColorEntry != nullptr )
		text = m_pSelColorEntry->FormatColor( CColorEntry::s_multiLineTipSep );

	COLORREF color = GetActualColor();

	if ( text.empty() && m_pSelColorTable != nullptr )
		if ( const CColorEntry* pColorEntry = m_pSelColorTable->FindColor( color ) )
			text = pColorEntry->FormatColor( CColorEntry::s_multiLineTipSep );

	if ( text.empty() )
		text = ui::FormatColor( color );

	if ( !text.empty() )
		stream::Tag( rText, text, _T(":  ") );

	// append the help message
	static const std::tstring s_helpMessage = str::Load( IDS_COLOR_PICKER_BUTTON_HELP );
	stream::Tag( rText, s_helpMessage, _T("\r\n\r\n") );
}

bool CColorPickerButton::DoReleaseCapture( void )
{
	if ( !m_bCaptured )
		return false;

	::ReleaseCapture();
	m_bCaptured = FALSE;
	return true;
}

void CColorPickerButton::ShowColorTablePopup( void )
{	// modeless display of the color popup

		//__super::OnShowColorPopup();		// original behaviour using CMFCColorPopupMenu

	if ( m_pPopup != nullptr )
	{
		m_pPopup->SendMessage( WM_CLOSE );
		m_pPopup = nullptr;
		return;
	}

	if ( m_Colors.IsEmpty() )
	{
		ASSERT( false );
		mfc::ColorBar_InitColors( m_Colors );		// use default pallete
	}

	CRect rect;
	GetWindowRect( &rect ); ++rect.left;

	CMFCPopupMenu* pPopupMenu = nullptr;
	BOOL created = false;

	if ( m_pSelColorTable != nullptr && m_pSelColorTable->IsSysColorTable() )
	{
		// display modeless popup (modeless support not fully implemented, therefore not the preferred way)
		pPopupMenu = new mfc::CColorTablePopupMenu( this );
		created = pPopupMenu->Create( this, rect.left, rect.bottom, nullptr, FALSE, TRUE );
	}
	else
	{
		mfc::CColorPopupMenu* pColorPopupMenu = new mfc::CColorPopupMenu( this, m_Colors, m_Color,
																		  m_strAutoColorText, m_strOtherText, m_strDocColorsText,
																		  m_lstDocColors, m_nColumns, m_ColorAutomatic );
		pColorPopupMenu->SetColorEditorHost( this );
		pPopupMenu = m_pPopup = pColorPopupMenu;
		created = m_pPopup->Create( this, rect.left, rect.bottom, nullptr, m_bEnabledInCustomizeMode );

		if ( created && m_bEnabledInCustomizeMode )
			pColorPopupMenu->GetColorBar()->SetInternal( true );
	}

	if ( created )
	{
		pPopupMenu->GetWindowRect( &rect );
		pPopupMenu->UpdateShadow( &rect );

		if ( m_bAutoSetFocus )
			pPopupMenu->GetMenuBar()->SetFocus();
	}
	else
	{
		ASSERT( false );	// color menu can't be used in the customization mode; you need to set CMFCColorButton::m_bEnabledInCustomizeMode.
		delete pPopupMenu;
		pPopupMenu = m_pPopup = nullptr;
	}

	DoReleaseCapture();
}

void CColorPickerButton::TrackMenuColorTables( void )
{
	if ( TrackingMenuColorTables == m_trackingMode )
		return;

	m_trackingMode = TrackingMenuColorTables;

	UINT cmdId = TrackModalPopupImpl( m_pMenuImpl->m_menu.GetSafeHmenu(), new mfc::CTrackingPopupMenu( m_pMenuImpl.get(), m_trackingMode ), false );

	// no WM_COMMAND sent: notifications work via ui::IColorEditorHost::SetColor(), or mfc::CColorMenuButton::CMBN_COLORSELECTED
	if ( cmdId != 0 && !( cmdId >= ID_HALFTONE_TABLE_16 && cmdId <= ID_REPO_COLOR_TABLE_MAX ) )			// menu command other than a color table?
		ui::SendCommand( m_hWnd, cmdId );			// send ID_EDIT_COPY, etc
}

UINT CColorPickerButton::TrackModalPopupImpl( HMENU hMenuPopup, CMFCPopupMenu* pPopupMenu, bool sendCommand, CPoint screenPos /*= CPoint( -1, -1 )*/ )
{
	ASSERT_PTR( pPopupMenu );

	DoReleaseCapture();

	mfc::CContextMenuMgr::Instance()->ResetTrackingPopup( pPopupMenu );

	UINT cmdId = mfc::CContextMenuMgr::Instance()->TrackModalPopup( hMenuPopup, this, sendCommand, screenPos );

	m_trackingMode = NoTracking;

	// CMFCColorButton boilerplate:
	m_bPushed = FALSE;
	m_bHighlighted = FALSE;

	Invalidate();
	UpdateWindow();

	return cmdId;
}

// base overrides:

void CColorPickerButton::OnShowColorPopup( void ) overrides(CMFCColorButton)
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

		bool useNamedColorTable = m_pSelColorTable != nullptr && m_pSelColorTable->BrowseNamedPopupGrid();

		if ( ui::IsKeyPressed( VK_SHIFT ) )
			useNamedColorTable = !useNamedColorTable;

		if ( useNamedColorTable )
			TrackModalPopupImpl( nullptr, new mfc::CColorTablePopupMenu( this ), false );
		else
			ShowColorTablePopup();			// modeless call
	}
}

void CColorPickerButton::OnDraw( CDC* pDC, const CRect& rect, UINT uiState ) overrides(CMFCColorButton)
{
	COLORREF displayRawColor = m_Color != CLR_NONE ? m_Color : m_ColorAutomatic;
	CScopedValue<COLORREF> scColor( &m_Color );
	CScopedValue<COLORREF> scAutoColor( &m_ColorAutomatic );
	CScopedValue<CString>  scAutoLabel( &m_strAutoColorText );

	if ( ui::IsSysColor( displayRawColor ) )
		if ( const CColorEntry* pColorEntry = CColorRepository::Instance()->GetSystemColorTable()->FindColor( displayRawColor ) )
		{	// while drawing system colors, temporarily evaluate the color and display with label
			scAutoLabel.SetValue( pColorEntry->GetName().c_str() );
			scAutoColor.SetValue( ui::EvalColor( displayRawColor ) );
			scColor.SetValue( CLR_NONE );		// fallback to Auto drawing mode: small color square + label
		}

	__super::OnDraw( pDC, rect, uiState );
}

void CColorPickerButton::PreSubclassWindow( void ) override
{
	__super::PreSubclassWindow();

	if ( !UseUserColors() )
		LoadFromRegistry();
}

BOOL CColorPickerButton::PreTranslateMessage( MSG* pMsg ) override
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

	if ( m_pCmdSvc->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return true;		// handle Undo/Redo commands

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
	ON_COMMAND( ID_SET_AUTO_COLOR, On_SetAutoColor )
	ON_UPDATE_COMMAND_UI( ID_SET_AUTO_COLOR, OnUpdate_SetAutoColor )
	ON_COMMAND( ID_MORE_COLORS, On_MoreColors )
	ON_UPDATE_COMMAND_UI( ID_MORE_COLORS, OnUpdate_MoreColors )
	ON_COMMAND( IDP_COPY_COLOR_TABLE, On_CopyColorTable )
	ON_UPDATE_COMMAND_UI( IDP_COPY_COLOR_TABLE, OnUpdate_CopyColorTable )
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
		TrackModalPopupImpl( m_pMenuImpl->m_menu.GetSafeHmenu(), new mfc::CTrackingPopupMenu( m_pMenuImpl.get(), m_trackingMode ), true, screenPos );		// returns the command
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
	ui::CopyColor( GetColor(), m_pSelColorTable );		// copy the qualified color name
}

void CColorPickerButton::OnPaste( void )
{
	COLORREF newColor;

	if ( ui::PasteColor( &newColor ) )
		UpdateColor( newColor );
}

void CColorPickerButton::OnUpdatePaste( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( ui::CanPasteColor() );
}

void CColorPickerButton::On_SetAutoColor( void )
{
	UpdateColor( CLR_NONE );
}

void CColorPickerButton::OnUpdate_SetAutoColor( CCmdUI* pCmdUI )
{
	bool isAutoColor = CLR_NONE == GetColor();

	pCmdUI->Enable( !isAutoColor );
	pCmdUI->SetCheck( isAutoColor );
}

void CColorPickerButton::On_MoreColors( void )
{
	EditColorDialog();
}

void CColorPickerButton::OnUpdate_MoreColors( CCmdUI* pCmdUI )
{
	bool isForeignColor = IsForeignColor();

	pCmdUI->SetCheck( isForeignColor );
}

void CColorPickerButton::On_CopyColorTable( void )
{
	ASSERT_PTR( m_pSelColorTable );

	std::tostringstream oss;

	m_pSelColorTable->TabularOut( oss );

	CTextClipboard::CopyText( oss.str(), m_hWnd );
}

void CColorPickerButton::OnUpdate_CopyColorTable( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_pSelColorTable != nullptr );
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
	bool enable = pColorTable->GetParentStore() != CScratchColorStore::Instance();			// a switchable table

	if ( UseUserColors() )
		enable = CScratchColorStore::Instance()->GetUserCustomTable() == pColorTable;
	else if ( CScratchColorStore::Instance()->GetRecentTable() == pColorTable )
		enable = !pColorTable->IsEmpty();

	pCmdUI->Enable( enable );		// user table is exclusive (can't be switched off to another table)
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
	pCmdUI->SetRadio( ( pCmdUI->m_nID - ID_PICK_COLOR_BAR_RADIO ) == (UINT)m_pickingMode );
	//ui::SetRadio( pCmdUI, ( pCmdUI->m_nID - ID_PICK_COLOR_BAR_RADIO ) == (UINT)m_pickingMode );
}

void CColorPickerButton::OnUpdateEnable( CCmdUI* pCmdUI )
{
	pCmdUI;
}


// CColorMenuTrackingImpl implementation

CColorMenuTrackingImpl::CColorMenuTrackingImpl( ui::IColorEditorHost* pHost, const CCommandModel* pCmdModel )
	: m_pHost( pHost )
	, m_pCmdModel( pCmdModel )
	, m_pMainStore( CColorRepository::Instance() )
	, m_pScratchStore( CScratchColorStore::Instance() )
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
	ModifyMenuTableItems( m_pScratchStore );
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
			for ( UINT i = 0; i != pColorStore->GetTables().size(); ++i )
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

mfc::CColorMenuButton* CColorMenuTrackingImpl::FindColorMenuButton( UINT colorBtnId )
{
	return checked_static_cast<mfc::CColorMenuButton*>( mfc::CTrackingPopupMenu::FindTrackingBarButton( colorBtnId ) );
}

void CColorMenuTrackingImpl::OnCustomizeMenuBar( CMFCPopupMenu* pMenuPopup, int trackingMode ) implement
{	// called in menu tracking mode
	ASSERT_PTR( pMenuPopup );
	REQUIRE( m_menu.GetSafeHmenu() == pMenuPopup->GetHMenu() );

	CMFCPopupMenuBar* pMenuBar = pMenuPopup->GetMenuBar();
	int index = 0;

	if ( CColorPickerButton::TrackingMenuColorTables == trackingMode )		// need to replace all color table sub-menus?
	{
		for ( int count = m_menu.GetMenuItemCount(); index != count; ++index )
			if ( const CColorTable* pColorTable = ui::GetMenuItemPtr<CColorTable>( m_menu, index ) )
			{
				ASSERT( !pColorTable->IsEmpty() );		// should've been excluded on popup menu set up

				UINT colorBtnId = m_menu.GetMenuItemID( index );
				mfc::CColorMenuButton::ReplaceBarButton( pMenuBar, colorBtnId, pColorTable, m_pHost );
			}
	}
	else if ( CColorPickerButton::TrackingContextMenu == trackingMode )
	{
		if ( const CColorTable* pColorTable = ui::GetMenuItemPtr<CColorTable>( m_menu, ID_RECENT_COLOR_SET, false ) )		// item not removed due to empty MRU colors?
			mfc::CColorMenuButton::ReplaceBarButton( pMenuBar, ID_RECENT_COLOR_SET, pColorTable, m_pHost );
	}

	mfc::CToolBarColorButton::ReplaceBarButton( pMenuBar, ID_SET_AUTO_COLOR, m_pHost->GetAutoColor() );		// to display the Automatic color box on the menu item
	mfc::CToolBarColorButton::ReplaceBarButton( pMenuBar, ID_MORE_COLORS, m_pHost->GetForeignColor() );		// to display the More Colors color box on the menu item

	mfc::CToolBarColorButton::ReplaceBarButton( pMenuBar, ID_EDIT_UNDO, PeekUndoTopColor() );					// to display the Undo top old color
	mfc::CToolBarColorButton::ReplaceBarButton( pMenuBar, ID_EDIT_REDO, PeekRedoTopColor() );					// to display the Redo top color
}

COLORREF CColorMenuTrackingImpl::PeekUndoTopColor( void ) const
{
	if ( const CSetColorCmd* pUndoCmd = checked_static_cast<const CSetColorCmd*>( m_pCmdModel->PeekUndo() ) )
		return m_pHost->GetFallbackColor( pUndoCmd->GetOldColor() );

	return CLR_NONE;
}

COLORREF CColorMenuTrackingImpl::PeekRedoTopColor( void ) const
{
	if ( const CSetColorCmd* pRedoCmd = checked_static_cast<const CSetColorCmd*>( m_pCmdModel->PeekRedo() ) )
		return m_pHost->GetFallbackColor( pRedoCmd->GetColor() );

	return CLR_NONE;
}

// message handlers

BEGIN_MESSAGE_MAP( CColorMenuTrackingImpl, CCmdTarget )
	ON_CONTROL_RANGE( mfc::CColorMenuButton::CMBN_COLORSELECTED, ID_HALFTONE_TABLE_16, ID_REPO_COLOR_TABLE_MAX, On_ColorSelected )	// not called, just for illustration
	ON_UPDATE_COMMAND_UI_RANGE( ID_HALFTONE_TABLE_16, ID_REPO_COLOR_TABLE_MAX, OnUpdate_ColorTable )
END_MESSAGE_MAP()

void CColorMenuTrackingImpl::On_ColorSelected( UINT selColorBtnId )
{
	// Called when a color bar button (CMFCToolBarColorButton) is clicked in one of the mfc::CColorMenuButton sub-menus.
	//	selColorBtnId: the ID of the mfc::CColorMenuButton that pops-up the tracking color bar

	ASSERT( false );	// new: in our case shouldn't get called, since we're passing ui::IColorEditorHost to the mfc::CColorMenuButton => will handle this internally

	if ( mfc::CColorMenuButton* pSelTableButton = FindColorMenuButton( selColorBtnId ) )
	{
		m_pHost->SetColor( pSelTableButton->GetColor(), true );
		m_pHost->SwitchSelColorTable( pSelTableButton->GetColorTable() );
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
