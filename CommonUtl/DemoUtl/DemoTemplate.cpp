
#include "stdafx.h"
#include "DemoTemplate.h"
#include "Application.h"
#include "ITestMarkup.h"
#include "TestDialog.h"
#include "TestPropertySheet.h"
#include "utl/LayoutEngine.h"
#include "utl/MenuUtilities.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"
#include "utl/VisualTheme.h"
#include "utl/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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
	, m_pLayoutEngine( dynamic_cast< ui::ILayoutEngine* >( m_pOwner ) )
	, m_selRadio( 0 )
	, m_dialogButton( &GetTags_ResizeStyle() )
	, m_pickFormatCheckedStatic( ui::DropDown )
	, m_resetSeqCounterButton( ID_RESET_DEFAULT )
	, m_changeCaseButton( &GetTags_ChangeCase() )
	, m_delimStatic( CThemeItem::m_null, CThemeItem( L"HEADER", vt::HP_HEADEROVERFLOW, vt::HOFS_NORMAL ) )
{
	ASSERT_PTR( m_pLayoutEngine );
	m_pLayoutEngine->RegisterCtrlLayout( layout::templateStyles, COUNT_OF( layout::templateStyles ) );

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

const std::vector< std::pair< std::tstring, std::tstring > >& CDemoTemplate::GetItems( void )
{
	static std::vector< std::pair< std::tstring, std::tstring > > items;
	if ( items.empty() )
	{
		items.push_back( std::make_pair( _T("Zoot Allures"), _T("The Dub Room Special") ) );
		items.push_back( std::make_pair( _T("Sheik Yerbouti"), _T("Joe's Menage") ) );
		items.push_back( std::make_pair( _T("Orchestral Favorites"), _T("Lumpy Money") ) );
		items.push_back( std::make_pair( _T("Joe's Garage"), _T("Joe's Camouflage") ) );
		items.push_back( std::make_pair( _T("Tinsel Town Rebellion"), _T("Feeding the Monkies at Ma Maison") ) );
		items.push_back( std::make_pair( _T("Shut Up 'n Play Yer Guitar"), _T("Roxy by Proxy") ) );
		items.push_back( std::make_pair( _T("Return of the Son of Shut Up 'n Play Yer Guitar"), _T("Dance Me This") ) );
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
		if ( GetMarkupDepth( dynamic_cast< CDemoPage* >( m_pOwner ) ) <= MaxDemoDepth )
			m_detailSheet.AddPage( new CDemoPage );

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
		case ResizeH:			dialog.GetLayoutEngine().MaxClientSize().cy = 0; break;
		case ResizeV:			dialog.GetLayoutEngine().MaxClientSize().cx = 0; break;
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
	ui::PopupAlign dropAlign = static_cast< ui::PopupAlign >( cmdId - ID_DROP_RIGHT );
	m_pickFormatCheckedStatic.SetPopupAlign( dropAlign );
}

void CDemoTemplate::OnUpdateDropAlignCheckedPicker( CCmdUI* pCmdUI )
{
	ui::PopupAlign dropAlign = static_cast< ui::PopupAlign >( pCmdUI->m_nID - ID_DROP_RIGHT );
	ui::SetRadio( pCmdUI, m_pickFormatCheckedStatic.GetPopupAlign() == dropAlign );				// keep the nice radio checkmark
}


// CListPage implementation

namespace layout
{
	CLayoutStyle listPageStyles[] =
	{
		{ IDC_FILE_RENAME_LIST, Size }
	};
}

CListPage::CListPage( void )
	: CLayoutPropertyPage( IDD_LIST_PAGE )
	, m_fileListView( IDC_FILE_RENAME_LIST, LVS_EX_GRIDLINES | CReportListControl::DefaultStyleEx )
{
	RegisterCtrlLayout( layout::listPageStyles, COUNT_OF( layout::listPageStyles ) );
}

void CListPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_fileListView.m_hWnd;
	DDX_Control( pDX, IDC_FILE_RENAME_LIST, m_fileListView );
	if ( firstInit )
	{	// fill in the files list (Source|Destination)
		const std::vector< std::pair< std::tstring, std::tstring > >& items = CDemoTemplate::GetItems();
		int pos = 0;

		m_fileListView.AddInternalChange();
		for ( std::vector< std::pair< std::tstring, std::tstring > >::const_iterator itItem = items.begin();
			  itItem != items.end(); ++itItem, ++pos )
		{
			m_fileListView.InsertItem( LVIF_TEXT, pos, (LPTSTR)itItem->first.c_str(), 0, 0, 0, (LPARAM)0 );
			m_fileListView.SetItemText( pos, Destination, itItem->second.c_str() );
		}
		m_fileListView.ReleaseInternalChange();
	}

	CLayoutPropertyPage::DoDataExchange( pDX );
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

		const std::vector< std::pair< std::tstring, std::tstring > >& items = CDemoTemplate::GetItems();
		for ( std::vector< std::pair< std::tstring, std::tstring > >::const_iterator itItem = items.begin();
			  itItem != items.end(); ++itItem )
		{
			if ( itItem != items.begin() )
			{
				src << lineEnd;
				dest << lineEnd;
			}
			src << itItem->first;
			dest << itItem->second;
			m_sourceCombo.AddString( itItem->first.c_str() );
			m_destCombo.AddString( itItem->second.c_str() );
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
