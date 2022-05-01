
#include "stdafx.h"
#include "DemoTemplate.h"
#include "Application.h"
#include "ITestMarkup.h"
#include "TestDialog.h"
#include "TestPropertySheet.h"
#include "utl/StringUtilities.h"
#include "utl/TextClipboard.h"
#include "utl/UI/LayoutEngine.h"
#include "utl/UI/CmdUpdate.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/VisualTheme.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/TandemControls.hxx"


namespace layout
{
	CLayoutStyle templateStyles[] =
	{
		{ IDC_OPEN_DIALOG_BUTTON, MoveX },
		{ IDC_OPEN_PROPERTIES_BUTTON, MoveX },
		{ IDC_CREATE_PROPERTIES_BUTTON, MoveX },
		{ IDC_DISABLE_SMOOTH_RESIZE_TOGGLE, MoveX },
		{ IDC_DISABLE_THEMES_TOGGLE, MoveX },

		{ IDC_FORMAT_COMBO, SizeX },
		{ IDC_DROP_RIGHT_ARROW_STATIC, MoveX },
		{ IDC_DROP_DOWN_STATIC, MoveX },
		{ IDC_DETAIL_SHEET_STATIC, Size },

		{ IDC_SOURCE_FILES_GROUP, MoveY | pctSizeX( 25 ) },
		{ IDC_COPY_SOURCE_PATHS_BUTTON, MoveY | pctSizeX( 25 ) },
		{ IDC_FORMAT_LEGEND_LABEL, MoveY | pctSizeX( 25 ) },

		{ IDC_DESTINATION_FILES_GROUP, Move },
		{ IDC_PASTE_FILES_BUTTON, Move },
		{ IDC_CLEAR_FILES_BUTTON, Move },
		{ IDC_CAPITALIZE_BUTTON, Move },
		{ ID_CHANGE_CASE, Move },

		{ IDC_REPLACE_FILES_BUTTON, Move },
		{ IDC_REPLACE_DELIMS_BUTTON, Move },
		{ IDC_DELIMITER_SET_COMBO, Move },
		{ IDC_DELIMITER_STATIC, Move },
		{ IDC_NEW_DELIMITER_EDIT, Move },
		{ IDC_GROUP_BOX_1, SizeX }
	};
}

ResizeStyle CDemoTemplate::m_dialogResizeStyle = ResizeHV;

CDemoTemplate::CDemoTemplate( CWnd* pOwner )
	: CCmdTarget()
	, m_pOwner( pOwner )
	, m_pLayoutEngine( dynamic_cast<ui::ILayoutEngine*>( m_pOwner ) )
	, m_selRadio( 0 )
	, m_seqCounterLabel( H_AlignLeft | V_AlignBottom | ui::V_TileMate )
	, m_dialogButton( &GetTags_ResizeStyle() )
	, m_pickFormatCheckedStatic( ui::DropDown )
	, m_resetSeqCounterButton( ID_RESET_DEFAULT )
	, m_changeCaseButton( &GetTags_ChangeCase() )
	, m_delimStatic( CThemeItem::m_null, CThemeItem( L"HEADER", vt::HP_HEADEROVERFLOW, vt::HOFS_NORMAL ) )
{
	ASSERT_PTR( m_pLayoutEngine );
	m_pLayoutEngine->RegisterCtrlLayout( layout::templateStyles, COUNT_OF( layout::templateStyles ) );

	m_seqCounterLabel.GetMateToolbar()->GetStrip()
		.AddButton( IDC_COPY_SOURCE_PATHS_BUTTON, ID_EDIT_COPY )
		.AddButton( IDC_PASTE_FILES_BUTTON, ID_EDIT_PASTE );

	m_pickFormatCheckedStatic.m_useText = true;
	m_changeCaseButton.SetSelValue( ExtLowerCase );
	m_resetSeqCounterButton.SetUseText( false );

	m_detailSheet.m_regSection = _T("TestForm\\Details");
	m_detailSheet.AddPage( new CListPage );
	m_detailSheet.AddPage( new CEditPage );
	m_detailSheet.AddPage( new CDetailsPage );
}

CDemoTemplate::~CDemoTemplate()
{
}

const std::vector< std::tstring >& CDemoTemplate::GetTextItems( void )
{
	static std::vector< std::tstring > items;
	if ( items.empty() )
	{
		items.push_back( _T("Zoot Allures|1976|55 MB (57 767 672 bytes)") );
		items.push_back( _T("Sheik Yerbouti|1979|90 MB (94 384 896 bytes)") );
		items.push_back( _T("Orchestral Favorites|1979|42.9 MB (45 031 523 bytes)") );
		items.push_back( _T("Joe's Garage|1979|146 MB (153 933 796 bytes)") );
		items.push_back( _T("Tinsel Town Rebellion|1981|79.7 MB (83 671 231 bytes)") );
		items.push_back( _T("Shut Up 'n Play Yer Guitar|1981|44.1 MB (46 268 558 bytes)") );
		items.push_back( _T("Return of the Son of Shut Up 'n Play Yer Guitar|1982|46.8 MB (49 152 810 bytes)") );
		items.push_back( _T("Baby Snakes|1983|49.1 MB (51 529 533 bytes)") );
		items.push_back( _T("Them or Us|1984|90 MB (94 419 467 bytes)") );
	}
	return items;
}

void CDemoTemplate::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	pTooltip;
	switch ( cmdId )
	{
		case IDC_OPEN_DIALOG_BUTTON:
		{
			static const std::vector< std::tstring > tooltips = str::LoadStrings( IDC_OPEN_DIALOG_BUTTON );
			rText = tooltips.at( m_dialogButton.GetSelEnum< ResizeStyle >() );
			break;
		}
		case ID_CHANGE_CASE:
		{
			static const std::vector< std::tstring > tooltips = str::LoadStrings( ID_CHANGE_CASE );
			rText = tooltips.at( m_changeCaseButton.GetSelEnum< ChangeCase >() );
			break;
		}
		case IDC_CAPITALIZE_BUTTON:
		{
			if ( m_capitalizeButton.GetRhsPartRect().PtInRect( ui::GetCursorPos( m_capitalizeButton ) ) )
			{
				static const std::tstring details = str::Load( ID_EDIT_DETAILS );
				rText = details;
			}
			break;
		}
	}
}

void CDemoTemplate::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_formatCombo.m_hWnd;
	if ( firstInit )
		if ( GetMarkupDepth( dynamic_cast<CDemoPage*>( m_pOwner ) ) <= MaxDemoDepth )
			m_detailSheet.AddPage( new CDemoPage() );

	DDX_Control( pDX, IDC_SEQ_COUNTER_LABEL, m_seqCounterLabel );
	DDX_Control( pDX, IDC_OPEN_DIALOG_BUTTON, m_dialogButton );
	DDX_Control( pDX, IDC_FORMAT_COMBO, m_formatCombo );
	DDX_Control( pDX, IDC_DROP_RIGHT_ARROW_STATIC, m_pickFormatStatic );
	DDX_Control( pDX, IDC_DROP_DOWN_STATIC, m_pickFormatCheckedStatic );
	DDX_Control( pDX, IDC_SEQ_COUNTER_EDIT, m_counterEdit );
	DDX_Control( pDX, IDC_SEQ_COUNTER_RESET_BUTTON, m_resetSeqCounterButton );
	DDX_Control( pDX, IDC_CAPITALIZE_BUTTON, m_capitalizeButton );
	DDX_Control( pDX, ID_CHANGE_CASE, m_changeCaseButton );
	DDX_Control( pDX, IDC_DELIMITER_SET_COMBO, m_delimiterSetCombo );
	DDX_Control( pDX, IDC_NEW_DELIMITER_EDIT, m_newDelimiterEdit );
	DDX_Control( pDX, IDC_DELIMITER_STATIC, m_delimStatic );
	ui::DDX_ButtonIcon( pDX, IDC_COPY_SOURCE_PATHS_BUTTON, ID_EDIT_COPY );
	ui::DDX_ButtonIcon( pDX, IDC_PASTE_FILES_BUTTON, ID_EDIT_PASTE );
	ui::DDX_ButtonIcon( pDX, IDC_CLEAR_FILES_BUTTON );		// ID_REMOVE_ALL_ITEMS
	ui::DDX_ButtonIcon( pDX, IDC_REPLACE_FILES_BUTTON, ID_EDIT_REPLACE );
	m_detailSheet.DDX_DetailSheet( pDX, IDC_DETAIL_SHEET_STATIC );

	if ( !pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
			ui::StretchWindow( m_pickFormatStatic, m_formatCombo, ui::Height, CSize( 0, 1 ) );

		m_dialogButton.SetSelValue( m_dialogResizeStyle );
		m_pOwner->CheckDlgButton( IDC_DISABLE_SMOOTH_RESIZE_TOGGLE, CLayoutEngine::Normal == CLayoutEngine::m_defaultFlags );
		m_pOwner->CheckDlgButton( IDC_DISABLE_THEMES_TOGGLE, CVisualTheme::IsDisabled() );
	}
}

BOOL CDemoTemplate::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return CCmdTarget::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

BEGIN_MESSAGE_MAP( CDemoTemplate, CCmdTarget )
	ON_BN_CLICKED( IDC_OPEN_DIALOG_BUTTON, OnBnClicked_OpenDialog )
	ON_BN_CLICKED( IDC_OPEN_PROPERTIES_BUTTON, OnBnClicked_OpenPropertySheet )
	ON_BN_CLICKED( IDC_CREATE_PROPERTIES_BUTTON, OnBnClicked_CreatePropertySheet )
	ON_BN_CLICKED( IDC_DISABLE_SMOOTH_RESIZE_TOGGLE, OnToggle_DisableSmoothResize )
	ON_BN_CLICKED( IDC_DISABLE_THEMES_TOGGLE, OnToggle_DisableThemes )
	ON_BN_CLICKED( IDC_DROP_RIGHT_ARROW_STATIC, OnBnClicked_DropFormat )
	ON_BN_CLICKED( IDC_DROP_DOWN_STATIC, OnBnClicked_DropDownFormat )
	ON_BN_CLICKED( IDC_CAPITALIZE_BUTTON, OnBnClicked_CapitalizeDestFiles )
	ON_SBN_RIGHTCLICKED( IDC_CAPITALIZE_BUTTON, OnBnClicked_CapitalizeOptions )
	ON_COMMAND_RANGE( ID_NUMERIC_SEQUENCE_2DIGITS, ID_NUMERIC_SEQUENCE_5DIGITS, OnNumSequence )
	ON_UPDATE_COMMAND_UI_RANGE( ID_NUMERIC_SEQUENCE_2DIGITS, ID_NUMERIC_SEQUENCE_5DIGITS, OnUpdateNumSequence )
	ON_COMMAND_RANGE( ID_DROP_RIGHT, ID_DROP_UP, OnDropAlignCheckedPicker )
	ON_UPDATE_COMMAND_UI_RANGE( ID_DROP_RIGHT, ID_DROP_UP, OnUpdateDropAlignCheckedPicker )
	ON_COMMAND( IDC_COPY_SOURCE_PATHS_BUTTON, OnClipboardCopy )
	ON_UPDATE_COMMAND_UI( IDC_COPY_SOURCE_PATHS_BUTTON, OnUpdateClipboardCopy )
	ON_COMMAND( IDC_PASTE_FILES_BUTTON, OnClipboardPaste )
	ON_UPDATE_COMMAND_UI( IDC_PASTE_FILES_BUTTON, OnUpdateClipboardPaste )
END_MESSAGE_MAP()

void CDemoTemplate::OnToggle_DisableSmoothResize( void )
{
	CLayoutEngine::m_defaultFlags = m_pOwner->IsDlgButtonChecked( IDC_DISABLE_SMOOTH_RESIZE_TOGGLE ) ? CLayoutEngine::Normal : CLayoutEngine::Smooth;
	ui::SendCommand( *AfxGetMainWnd(), ID_FILE_CLOSE );
	ui::SendCommand( *AfxGetMainWnd(), ID_FILE_NEW );
}

void CDemoTemplate::OnToggle_DisableThemes( void )
{
	CVisualTheme::SetEnabled( !m_pOwner->IsDlgButtonChecked( IDC_DISABLE_THEMES_TOGGLE ) );

	AfxGetMainWnd()->RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
	m_pOwner->RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
}

void CDemoTemplate::OnBnClicked_OpenDialog( void )
{
	CTestDialog dialog( m_pOwner );

	switch ( m_dialogResizeStyle = m_dialogButton.GetSelEnum< ResizeStyle >() )
	{
		case ResizeHV:			break;
		case ResizeH:			dialog.GetLayoutEngine().DisableResizeVertically(); break;
		case ResizeV:			dialog.GetLayoutEngine().DisableResizeHorizontally(); break;
		case ResizeMaxLimit:	dialog.GetLayoutEngine().MaxClientSize() = CSize( 700, 700 ); break;
	}

	dialog.DoModal();
}

void CDemoTemplate::OnBnClicked_OpenPropertySheet( void )
{
	CTestPropertySheet sheet( m_pOwner );
	sheet.DoModal();
}

void CDemoTemplate::OnBnClicked_CreatePropertySheet( void )
{
	CTestPropertySheet* pModelessSheet = new CTestPropertySheet;
	VERIFY( pModelessSheet->Create( m_pOwner ) );
}

void CDemoTemplate::OnBnClicked_DropFormat( void )
{
	m_pickFormatStatic.TrackMenu( m_pOwner, IDR_FORMAT_PICKER_MENU, 0, false );
}

void CDemoTemplate::OnBnClicked_DropDownFormat( void )
{
	m_pickFormatCheckedStatic.TrackMenu( m_pOwner, IDR_FORMAT_PICKER_MENU, 1, true );
}

void CDemoTemplate::OnBnClicked_CapitalizeDestFiles( void )
{
	AfxMessageBox( _T("Capitalize Destination...") );
}

void CDemoTemplate::OnBnClicked_CapitalizeOptions( void )
{
	AfxMessageBox( _T("Capitalization rules...") );
}

void CDemoTemplate::OnNumSequence( UINT cmdId )
{
	m_selRadio = cmdId - ID_NUMERIC_SEQUENCE_2DIGITS;
}

void CDemoTemplate::OnUpdateNumSequence( CCmdUI* pCmdUI )
{
	ui::SetRadio( pCmdUI, m_selRadio == ( pCmdUI->m_nID - ID_NUMERIC_SEQUENCE_2DIGITS ) );		// keep the nice radio checkmark
}

void CDemoTemplate::OnDropAlignCheckedPicker( UINT cmdId )
{
	ui::PopupAlign dropAlign = static_cast<ui::PopupAlign>( cmdId - ID_DROP_RIGHT );
	m_pickFormatCheckedStatic.SetPopupAlign( dropAlign );
}

void CDemoTemplate::OnUpdateDropAlignCheckedPicker( CCmdUI* pCmdUI )
{
	ui::PopupAlign dropAlign = static_cast<ui::PopupAlign>( pCmdUI->m_nID - ID_DROP_RIGHT );
	ui::SetRadio( pCmdUI, m_pickFormatCheckedStatic.GetPopupAlign() == dropAlign );				// keep the nice radio checkmark
}

void CDemoTemplate::OnClipboardCopy( void )
{
	fs::CPath execDirPath = app::GetModulePath().GetParentPath().GetParentPath();
	const std::vector< std::tstring >& srcItems = GetTextItems();
	std::vector< fs::CPath > filePaths;

	for ( std::vector< std::tstring >::const_iterator itSrc = srcItems.begin(); itSrc != srcItems.end(); ++itSrc )
	{
		std::vector< std::tstring > subItems;
		str::Split( subItems, itSrc->c_str(), _T("|") );

		filePaths.push_back( execDirPath / subItems.front() );
	}

	if ( CTextClipboard::CopyToLines( filePaths, m_pOwner->GetSafeHwnd() ) )
		ui::MessageBox( str::Format( _T("Copied %d file paths to clipboard."), filePaths.size() ) );
	else
		ui::MessageBox( _T("Error copying file paths to clipboard!") );
}

void CDemoTemplate::OnUpdateClipboardCopy( CCmdUI* pCmdUI )
{
	pCmdUI;
}

void CDemoTemplate::OnClipboardPaste( void )
{
	std::vector< fs::CPath > filePaths;
	if ( CTextClipboard::PasteFromLines( filePaths, m_pOwner->GetSafeHwnd() ) )
		ui::MessageBox( str::Format( _T("Pasted %d file paths from clipboard:\n\n%s"), filePaths.size(), str::JoinLines( filePaths, _T("\n") ).c_str() ) );
	else
		ui::MessageBox( _T("Error pasting file paths from clipboard!") );
}

void CDemoTemplate::OnUpdateClipboardPaste( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( CTextClipboard::CanPasteText() );
}


// CListPage implementation

namespace reg
{
	static const TCHAR section_listPage[] = _T("ListPage");
	static const TCHAR entry_useAlternateRows[] = _T("UseAlternateRows");
	static const TCHAR entry_useTextEffects[] = _T("UseTextEffects");
}

namespace layout
{
	CLayoutStyle listPageStyles[] =
	{
		{ IDC_FILE_RENAME_LIST, Size },
		{ IDC_USE_ALTERNATE_ROWS_CHECK, MoveX },
		{ IDC_USE_TEXT_EFFECTS_CHECK, MoveX }
	};
}

CListPage::CListPage( void )
	: CLayoutPropertyPage( IDD_LIST_PAGE )
	, m_fileListView( IDC_FILE_RENAME_LIST, LVS_EX_GRIDLINES | lv::DefaultStyleEx )
{
	RegisterCtrlLayout( layout::listPageStyles, COUNT_OF( layout::listPageStyles ) );
}

void CListPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_fileListView.m_hWnd;

	DDX_Control( pDX, IDC_FILE_RENAME_LIST, m_fileListView );
	if ( firstInit )
	{	// fill in the files list (Source|Destination)
		CScopedLockRedraw freeze( &m_fileListView );
		CScopedInternalChange internalChange( &m_fileListView );

		const std::vector< std::tstring >& srcItems = CDemoTemplate::GetTextItems();

		int pos = 0;
		for ( std::vector< std::tstring >::const_iterator itSrc = srcItems.begin(); itSrc != srcItems.end(); ++itSrc, ++pos )
		{
			std::vector< std::tstring > subItems;
			str::Split( subItems, itSrc->c_str(), _T("|") );
			ASSERT( !subItems.empty() );

			std::vector< std::tstring >::const_iterator itSubItem = subItems.begin();

			m_fileListView.InsertItem( LVIF_TEXT | LVIF_PARAM, pos, (LPTSTR)itSubItem->c_str(), 0, 0, 0, (LPARAM)itSrc->c_str() );
			if ( ++itSubItem != subItems.end() )
				m_fileListView.SetItemText( pos, Destination, itSubItem->c_str() );
			if ( ++itSubItem != subItems.end() )
				m_fileListView.SetItemText( pos, Notes, itSubItem->c_str() );
		}

		CheckDlgButton( IDC_USE_ALTERNATE_ROWS_CHECK, AfxGetApp()->GetProfileInt( reg::section_listPage, reg::entry_useAlternateRows, true ) );
		CheckDlgButton( IDC_USE_TEXT_EFFECTS_CHECK, AfxGetApp()->GetProfileInt( reg::section_listPage, reg::entry_useTextEffects, true ) );

		OnToggle_UseAlternateRows();
		OnToggle_UseTextEffects();
	}

	CLayoutPropertyPage::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CListPage, CLayoutPropertyPage )
	ON_BN_CLICKED( IDC_USE_ALTERNATE_ROWS_CHECK, OnToggle_UseAlternateRows )
	ON_BN_CLICKED( IDC_USE_TEXT_EFFECTS_CHECK, OnToggle_UseTextEffects )
END_MESSAGE_MAP()

void CListPage::OnToggle_UseAlternateRows( void )
{
	bool useAlternateRows = IsDlgButtonChecked( IDC_USE_ALTERNATE_ROWS_CHECK ) != FALSE;
	AfxGetApp()->WriteProfileInt( reg::section_listPage, reg::entry_useAlternateRows, useAlternateRows );

	m_fileListView.SetUseAlternateRowColoring( useAlternateRows );
	m_fileListView.Invalidate();
}

void CListPage::OnToggle_UseTextEffects( void )
{
	bool useTextEffects = IsDlgButtonChecked( IDC_USE_TEXT_EFFECTS_CHECK ) != FALSE;
	AfxGetApp()->WriteProfileInt( reg::section_listPage, reg::entry_useTextEffects, useTextEffects );

	if ( useTextEffects )
	{
		// list global text effects
		m_fileListView.m_ctrlTextEffect.m_textColor = color::Violet;

		// rows/cells text effects
		m_fileListView.MarkCellAt( 0, Source, ui::CTextEffect( ui::Bold ) );
		m_fileListView.MarkCellAt( 2, Destination, ui::CTextEffect( ui::Bold | ui::Italic | ui::Underline ) );
		m_fileListView.MarkCellAt( 3, CReportListControl::EntireRecord, ui::CTextEffect( ui::Bold | ui::Underline, color::Red, color::PaleYellow ) );
		m_fileListView.MarkCellAt( 5, CReportListControl::EntireRecord, ui::CTextEffect( ui::Bold | ui::Underline, color::Magenta, color::LightGreenish ) );
		m_fileListView.MarkCellAt( 5, Destination, ui::CTextEffect( ui::Bold, color::Blue ) );
		m_fileListView.MarkCellAt( 6, Destination, ui::CTextEffect( ui::Bold, color::Red ) );
		m_fileListView.MarkCellAt( 7, CReportListControl::EntireRecord, ui::CTextEffect( ui::Regular, color::Black ) );
		m_fileListView.MarkCellAt( 8, CReportListControl::EntireRecord, ui::CTextEffect( ui::Underline, color::Green, color::PastelPink ) );
	}
	else
	{
		m_fileListView.m_ctrlTextEffect = ui::CTextEffect::s_null;
		m_fileListView.ClearMarkedCells();
	}

	m_fileListView.Invalidate();
}


// CEditPage implementation

namespace layout
{
	CLayoutStyle editPageStyles[] =
	{
		{ IDC_ALBUMS_GROUP, SizeX | pctSizeY( 70 ) },

		{ IDC_SOURCE_GROUP, pctSizeX( 50 ) | pctSizeY( 70 ) },
		{ IDC_SOURCE_LABEL, pctSizeX( 50 ) },
		{ IDC_SOURCE_EDIT, pctSizeX( 50 ) | pctSizeY( 35 ) },
		{ IDC_SOURCE_COMBO, pctSizeX( 50 ) | pctMoveY( 35 ) | pctSizeY( 35 ) | DoRepaint },

		{ IDC_DEST_GROUP, pctMoveX( 50 ) | pctSizeX( 50 ) | pctSizeY( 70 ) },
		{ IDC_DEST_LABEL, pctMoveX( 50 ) | pctSizeX( 50 ) },
		{ IDC_DEST_EDIT, pctMoveX( 50 ) | pctSizeX( 50 ) | pctSizeY( 35 ) },
		{ IDC_DEST_COMBO, pctMoveX( 50 ) | pctSizeX( 50 ) | pctMoveY( 35 ) | pctSizeY( 35 ) | DoRepaint },

		{ IDC_A_GROUP, SizeX | pctMoveY( 70 ) | pctSizeY( 15 ) },
		{ IDC_BUTTON1, pctMoveX( 0 ) | pctMoveY( 70 ) | pctSizeX( 25 ) | pctSizeY( 15 ) },
		{ IDC_BUTTON2, pctMoveX( 25 ) | pctMoveY( 70 ) | pctSizeY( 15 ) | pctSizeX( 50 ) },
		{ IDC_BUTTON3, pctMoveX( 75 ) | pctMoveY( 70 ) | pctSizeY( 15 ) | pctSizeX( 25 ) },

		{ IDC_B_GROUP, SizeX | pctMoveY( 85 ) | pctSizeY( 15 ) },
		{ IDC_BUTTON4, pctMoveX( 0 ) | pctMoveY( 85 ) | pctSizeX( 33 ) | pctSizeY( 15 ) },
		{ IDC_BUTTON5, pctMoveX( 33 ) | pctMoveY( 85 ) | pctSizeX( 34 ) | pctSizeY( 15 ) },
		{ IDC_BUTTON6, pctMoveX( 67 ) | pctMoveY( 85 ) | pctSizeX( 33 ) | pctSizeY( 15 ) }
	};
}

CEditPage::CEditPage( void )
	: CLayoutPropertyPage( IDD_EDIT_PAGE )
{
	RegisterCtrlLayout( layout::editPageStyles, COUNT_OF( layout::editPageStyles ) );
}

void CEditPage::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	rText, cmdId, pTooltip;
	if ( cmdId == GetTemplateId() )
		rText = _T("my edit page (callback)");
}

void CEditPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_sourceEdit.m_hWnd;
	DDX_Control( pDX, IDC_SOURCE_EDIT, m_sourceEdit );
	DDX_Control( pDX, IDC_SOURCE_COMBO, m_sourceCombo );
	DDX_Control( pDX, IDC_DEST_EDIT, m_destEdit );
	DDX_Control( pDX, IDC_DEST_COMBO, m_destCombo );

	if ( firstInit )
	{
		static const TCHAR lineEnd[] = _T("\r\n");
		std::tostringstream src, dest;

		const std::vector< std::tstring >& srcItems = CDemoTemplate::GetTextItems();
		for ( std::vector< std::tstring >::const_iterator itSrc = srcItems.begin(); itSrc != srcItems.end(); ++itSrc )
		{
			if ( itSrc != srcItems.begin() )
			{
				src << lineEnd;
				dest << lineEnd;
			}

			std::vector< std::tstring > subItems;
			str::Split( subItems, itSrc->c_str(), _T("|") );
			ASSERT( !subItems.empty() );

			std::vector< std::tstring >::const_iterator itSubItem = subItems.begin();
			src << *itSubItem;
			m_sourceCombo.AddString( itSubItem->c_str() );

			dest << *++itSubItem;
			m_destCombo.AddString( itSubItem->c_str() );
		}
		ui::SetWindowText( m_sourceEdit, src.str() );
		ui::SetWindowText( m_destEdit, dest.str() );

		m_sourceCombo.SetCurSel( 0 );
		m_destCombo.SetCurSel( 5 );
	}
	CLayoutPropertyPage::DoDataExchange( pDX );
}


// CDemoPage implementation

CDemoPage::CDemoPage( void )
	: CLayoutPropertyPage( IDD_DEMO_DIALOG )
	, m_pDemo( new CDemoTemplate( this ) )
{
}

CDemoPage::~CDemoPage()
{
}

void CDemoPage::DoDataExchange( CDataExchange* pDX )
{
	m_pDemo->DoDataExchange( pDX );
	CLayoutPropertyPage::DoDataExchange( pDX );
}

BOOL CDemoPage::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		m_pDemo->OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		CLayoutPropertyPage::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// CDetailsPage implementation

namespace layout
{
	CLayoutStyle detailsPageStyles[] =
	{
		{ IDC_DETAIL_SHEET_STATIC, Size }
	};
}

CDetailsPage::CDetailsPage( void )
	: CLayoutPropertyPage( IDD_DETAILS_PAGE )
{
	RegisterCtrlLayout( layout::detailsPageStyles, COUNT_OF( layout::detailsPageStyles ) );

	m_detailSheet.AddPage( new CListPage );
	m_detailSheet.AddPage( new CEditPage );
}

CDetailsPage::~CDetailsPage()
{
}

void CDetailsPage::DoDataExchange( CDataExchange* pDX )
{
	if ( NULL == m_detailSheet.m_hWnd )
		if ( GetMarkupDepth( this ) <= MaxDepth )
			m_detailSheet.AddPage( new CDetailsPage );

	m_detailSheet.DDX_DetailSheet( pDX, IDC_DETAIL_SHEET_STATIC );
	CLayoutPropertyPage::DoDataExchange( pDX );
}
