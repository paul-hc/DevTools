
#include "pch.h"
#include "OptionsSheet.h"
#include "IncludeDirectories.h"
#include "ModuleSession.h"
#include "Application.h"
#include "resource.h"
#include "utl/Algorithms.h"
#include "utl/FileSystem.h"
#include "utl/TextClipboard.h"
#include "utl/UI/WndUtilsEx.h"
#include "utl/UI/resource.h"
#include <sstream>
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// COptionsSheet implementation

COptionsSheet::COptionsSheet( CWnd* pParent )
	: CLayoutPropertySheet( str::Load( IDS_OPTIONS_PROPERTY_SHEET_CAPTION ), pParent )
{
	m_regSection = _T("OptionsSheet");
	m_resizable = false;
	m_alwaysModified = true;
	SetTopDlg();
	SetSingleTransaction();
	LoadDlgIcon( ID_OPTIONS );

	AddPage( new CGeneralOptionsPage() );
	AddPage( new CCodingStandardPage() );
	AddPage( new CCppImplFormattingPage() );
	AddPage( new CBscPathPage() );
	AddPage( new CDirectoriesPage() );
}

COptionsSheet::~COptionsSheet()
{
	DeleteAllPages();
}


// CGeneralOptionsPage implementation

static const TCHAR s_codeTemplateFilter[] =
	_T("Code template Files (*.ctf)|*.ctf|")
	_T("All Files (*.*)|*.*||");


CGeneralOptionsPage::CGeneralOptionsPage( void )
	: CLayoutPropertyPage( IDD_OPTIONS_GENERAL_PAGE )
	, m_templateFileEdit( ui::FilePath, s_codeTemplateFilter )
{
	m_menuVertSplitCount = 1;
	m_autoCodeGeneration = false;
	m_displayErrorMessages = false;
	m_useCommentDecoration = false;
	m_duplicateLineMoveDown = false;
}

CGeneralOptionsPage::~CGeneralOptionsPage()
{
}

void CGeneralOptionsPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_menuVertSplitCountSpin.m_hWnd;

	DDX_Control( pDX, IDC_TEXT_TEMPLATE_FILE_EDIT, m_templateFileEdit );
	ui::DDX_Text( pDX, IDC_USER_NAME_EDIT, m_developerName );
	ui::DDX_Path( pDX, IDC_TEXT_TEMPLATE_FILE_EDIT, m_codeTemplatePath );
	DDX_Text( pDX, IDC_MENU_VERT_SPLIT_COUNT_EDIT, m_menuVertSplitCount );
	DDX_Control( pDX, IDC_MENU_VERT_SPLIT_COUNT_SPIN, m_menuVertSplitCountSpin );
	ui::DDX_Text( pDX, IDC_DEFAULT_COMMENT_COLUMN_EDIT, m_singleLineCommentToken );
	ui::DDX_Text( pDX, IDC_CLASS_PREFIX_EDIT, m_classPrefix );
	ui::DDX_Text( pDX, IDC_STRUCT_PREFIX_EDIT, m_structPrefix );
	ui::DDX_Text( pDX, IDC_ENUM_PREFIX_EDIT, m_enumPrefix );
	ui::DDX_Bool( pDX, IDC_AUTO_CODE_GENERATION_CHECK, m_autoCodeGeneration );
	ui::DDX_Bool( pDX, IDC_DISPLAY_ERROR_MESSAGES_CHECK, m_displayErrorMessages );
	ui::DDX_Bool( pDX, IDC_USE_COMMENT_DECORATION_CHECK, m_useCommentDecoration );
	ui::DDX_Bool( pDX, IDC_DUPLICATE_LINE_MOVE_DOWN_CHECK, m_duplicateLineMoveDown );

	if ( firstInit )
		m_menuVertSplitCountSpin.SetRange( 1, 300 );

	CLayoutPropertyPage::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CGeneralOptionsPage, CLayoutPropertyPage )
END_MESSAGE_MAP()


// CCodingStandardPage property page

namespace reg
{
	namespace csp
	{
		static const TCHAR section_page[] = _T("CodingStandardPage");
		static const TCHAR entry_selectedBraceRules[] = _T("SelectedBraceRules");
		static const TCHAR entry_selectedOperatorRules[] = _T("SelectedOperatorRules");
	}
}

const TCHAR CCodingStandardPage::m_breakSep[] = _T(" ");

CCodingStandardPage::CCodingStandardPage( void )
	: CLayoutPropertyPage( IDD_OPTIONS_CODE_FORMATTING_PAGE )
	, m_splitSeparatorsEdit( m_breakSep )
{
	m_splitMaxColumnEdit.SetValidRange( Range<int>( 1, 250 ) );

	const code::CFormatterOptions& formatterOptions = app::GetModuleSession().GetCodeFormatterOptions();

	m_splitMaxColumn = 0;
	m_preserveMultipleWhiteSpace = formatterOptions.m_preserveMultipleWhiteSpace;
	m_deleteTrailingWhiteSpace = formatterOptions.m_deleteTrailingWhiteSpace;

	m_breakSeparators = formatterOptions.m_breakSeparators;
	m_braceRules = formatterOptions.m_braceRules;
	m_operatorRules = formatterOptions.GetOperatorRules();
}

CCodingStandardPage::~CCodingStandardPage()
{
}

void CCodingStandardPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_splitMaxColumnEdit.m_hWnd;

	ui::DDX_Bool( pDX, IDC_PRESERVE_MULTIPLE_WHITESPACES_CHECK, m_preserveMultipleWhiteSpace );
	ui::DDX_Bool( pDX, IDC_DELETE_TRAILING_WHITESPACES_CHECK, m_deleteTrailingWhiteSpace );
	DDX_Control( pDX, IDC_SPLIT_MAX_COLUMN_EDIT, m_splitMaxColumnEdit );
	DDX_Text( pDX, IDC_SPLIT_MAX_COLUMN_EDIT, m_splitMaxColumn );
	DDX_Control( pDX, IDC_BREAK_SEPARATORS_EDIT, m_splitSeparatorsEdit );
	m_splitSeparatorsEdit.DDX_Items( pDX, m_breakSeparators );
	DDX_Control( pDX, IDC_BRACKETS_RULES_COMBO, m_braceRulesCombo );
	DDX_Control( pDX, IDC_OPERATORS_RULES_COMBO, m_operatorRulesCombo );

	if ( firstInit )
	{
		m_braceRulesCombo.SetFont( CTextEdit::GetFixedFont( CTextEdit::Large ) );
		m_operatorRulesCombo.SetFont( CTextEdit::GetFixedFont( CTextEdit::Large ) );

		for ( std::vector< code::CFormatterOptions::CBraceRule >::const_iterator itBraceRule = m_braceRules.begin();
			  itBraceRule != m_braceRules.end(); ++itBraceRule )
			m_braceRulesCombo.AddString( str::Format( _T("%c%c"), itBraceRule->m_braceOpen, itBraceRule->m_braceClose ).c_str() );

		for ( std::vector< code::CFormatterOptions::COperatorRule >::const_iterator itOpRule = m_operatorRules.begin();
			  itOpRule != m_operatorRules.end(); ++itOpRule )
			m_operatorRulesCombo.AddString( itOpRule->m_pOperator );

		m_braceRulesCombo.SetCurSel( AfxGetApp()->GetProfileInt( reg::csp::section_page, reg::csp::entry_selectedBraceRules, 0 ) );
		m_operatorRulesCombo.SetCurSel( AfxGetApp()->GetProfileInt( reg::csp::section_page, reg::csp::entry_selectedOperatorRules, 0 ) );

		CBnSelChange_BraceRules();
		CBnSelChange_OperatorRules();
	}

	CLayoutPropertyPage::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CCodingStandardPage, CLayoutPropertyPage )
	ON_CBN_SELCHANGE( IDC_BRACKETS_RULES_COMBO, CBnSelChange_BraceRules )
	ON_CBN_SELCHANGE( IDC_OPERATORS_RULES_COMBO, CBnSelChange_OperatorRules )
	ON_COMMAND_RANGE( IDC_IS_ARGUMENT_LIST_BRACKET_CHECK, IDC_PRESERVE_SPACES_RADIO, BnClicked_BraceRulesButton )
	ON_COMMAND_RANGE( IDC_OP_BEFORE_REMOVE_SPACES_RADIO, IDC_OP_AFTER_PRESERVE_SPACES_RADIO, BnClicked_OperatorRulesButton )
END_MESSAGE_MAP()

void CCodingStandardPage::CBnSelChange_BraceRules( void )
{
	unsigned int selIndex = m_braceRulesCombo.GetCurSel();
	const code::CFormatterOptions::CBraceRule& braceRule = m_braceRules[ selIndex ];

	ASSERT( selIndex < m_braceRules.size() );

	CheckDlgButton( IDC_IS_ARGUMENT_LIST_BRACKET_CHECK, braceRule.m_isArgList );
	CheckRadioButton( IDC_REMOVE_SPACES_RADIO, IDC_PRESERVE_SPACES_RADIO, IDC_REMOVE_SPACES_RADIO + braceRule.m_spacing );

	AfxGetApp()->WriteProfileInt( reg::csp::section_page, reg::csp::entry_selectedBraceRules, selIndex );
}

void CCodingStandardPage::CBnSelChange_OperatorRules( void )
{
	unsigned int selIndex = m_operatorRulesCombo.GetCurSel();
	ASSERT( selIndex < m_operatorRules.size() );

	const code::CFormatterOptions::COperatorRule& operatorRule = m_operatorRules[ selIndex ];

	CheckRadioButton( IDC_OP_BEFORE_REMOVE_SPACES_RADIO, IDC_OP_BEFORE_PRESERVE_SPACES_RADIO,
					  IDC_OP_BEFORE_REMOVE_SPACES_RADIO + operatorRule.m_spaceBefore );
	CheckRadioButton( IDC_OP_AFTER_REMOVE_SPACES_RADIO, IDC_OP_AFTER_PRESERVE_SPACES_RADIO,
					  IDC_OP_AFTER_REMOVE_SPACES_RADIO + operatorRule.m_spaceAfter );

	AfxGetApp()->WriteProfileInt( reg::csp::section_page, reg::csp::entry_selectedOperatorRules, selIndex );
}

void CCodingStandardPage::BnClicked_BraceRulesButton( UINT cmdId )
{
	cmdId;
	unsigned int selIndex = m_braceRulesCombo.GetCurSel();
	ASSERT( selIndex < m_braceRules.size() );

	code::CFormatterOptions::CBraceRule& rBraceRule = m_braceRules[ selIndex ];

	rBraceRule.m_isArgList = IsDlgButtonChecked( IDC_IS_ARGUMENT_LIST_BRACKET_CHECK ) != FALSE;
	rBraceRule.m_spacing = code::TokenSpacing( GetCheckedRadioButton( IDC_REMOVE_SPACES_RADIO, IDC_PRESERVE_SPACES_RADIO ) - IDC_REMOVE_SPACES_RADIO );
}

void CCodingStandardPage::BnClicked_OperatorRulesButton( UINT cmdId )
{
	cmdId;
	unsigned int selIndex = m_operatorRulesCombo.GetCurSel();
	ASSERT( selIndex < m_operatorRules.size() );

	code::CFormatterOptions::COperatorRule& rOperatorRule = m_operatorRules[ selIndex ];

	rOperatorRule.m_spaceBefore = code::TokenSpacing( GetCheckedRadioButton( IDC_OP_BEFORE_REMOVE_SPACES_RADIO, IDC_OP_BEFORE_PRESERVE_SPACES_RADIO ) - IDC_OP_BEFORE_REMOVE_SPACES_RADIO );
	rOperatorRule.m_spaceAfter = code::TokenSpacing( GetCheckedRadioButton( IDC_OP_AFTER_REMOVE_SPACES_RADIO, IDC_OP_AFTER_PRESERVE_SPACES_RADIO ) - IDC_OP_AFTER_REMOVE_SPACES_RADIO );
}


// CCppImplFormattingPage property page


CCppImplFormattingPage::CCppImplFormattingPage( void )
	: CLayoutPropertyPage( IDD_OPTIONS_CPP_IMPL_FORMATTING_PAGE )
{
	const code::CFormatterOptions& formatterOptions = app::GetModuleSession().GetCodeFormatterOptions();

	m_returnTypeOnSeparateLine = formatterOptions.m_returnTypeOnSeparateLine;
	m_commentOutDefaultParams = formatterOptions.m_commentOutDefaultParams;
	m_linesBetweenFunctionImpls = formatterOptions.m_linesBetweenFunctionImpls;
}

CCppImplFormattingPage::~CCppImplFormattingPage()
{
}

void CCppImplFormattingPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_linesBetweenFunctionImplsSpin.m_hWnd;

	ui::DDX_Bool( pDX, IDC_RETURN_TYPE_ON_SEPARATE_LINE_CHECK, m_returnTypeOnSeparateLine );
	ui::DDX_Bool( pDX, IDC_COMMENT_OUT_DEFAULT_PARAMS_CHECK, m_commentOutDefaultParams );
	DDX_Text( pDX, IDC_LINES_BETWEEN_FUNCTION_IMPLS_EDIT, m_linesBetweenFunctionImpls );
	DDX_Control( pDX, IDC_LINES_BETWEEN_FUNCTION_IMPLS_SPIN, m_linesBetweenFunctionImplsSpin );

	if ( firstInit )
		m_linesBetweenFunctionImplsSpin.SetRange( 0, 10 );

	CLayoutPropertyPage::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CCppImplFormattingPage, CLayoutPropertyPage )
END_MESSAGE_MAP()


// CBscPathPage property page

CBscPathPage::CBscPathPage( void )
	: CLayoutPropertyPage( IDD_OPTIONS_BSC_PATH_PAGE )
	, m_pathsListBox( this )
	, m_folderContent( ui::DirPath, _T("C++ Browse Files (*.bsc)|*.bsc|All Files (*.*)|*.*||") )
	, m_isUserUpdate( true )
{
}

CBscPathPage::~CBscPathPage()
{
}

void CBscPathPage::SetupPathListBox( int selIndex /*= LB_ERR*/ )
{
	{
		CScopedLockRedraw freeze( &m_pathsListBox );
		m_pathsListBox.ResetContent();

		for ( unsigned int i = 0; i != m_pathItems.size(); ++i )
			m_pathsListBox.AddString( m_pathItems[ i ].m_searchPath.GetPtr() );
	}

	if ( selIndex != LB_ERR )
		m_pathsListBox.SetCurSel( selIndex );

	LBnSelChange_BrowseFilesPath();
}

int CBscPathPage::MoveItemTo( int srcIndex, int destIndex, bool isDropped /*= false*/ )
{
	TRACE( _T(" # Dropped(): srcIndex=%d, destIndex=%d\n"), srcIndex, destIndex );

	CSearchPathItem movedItem = m_pathItems[ srcIndex ];

	m_pathItems.erase( m_pathItems.begin() + srcIndex );

	if ( isDropped )
		if ( srcIndex < destIndex )
			--destIndex;

	m_pathItems.insert( m_pathItems.begin() + destIndex, movedItem );

	return destIndex;
}

void CBscPathPage::UpdateToolBarCmdUI( void )
{
	m_toolBar.SendMessage( WM_IDLEUPDATECMDUI, (WPARAM)TRUE );
}

void CBscPathPage::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_BROWSE_FILES_PATH_LIST, m_pathsListBox );
	DDX_Control( pDX, IDC_SEARCH_FILTER_EDIT, m_searchFilterEdit );
	DDX_Control( pDX, IDC_DISPLAY_TAG_EDIT, m_displayTagEdit );
	m_toolBar.DDX_Placeholder( pDX, IDC_TOOLBAR_PLACEHOLDER, H_AlignRight | V_AlignCenter, IDR_BSC_PATH_STRIP );

	std::vector< std::tstring > items;

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{	// load controls
		str::Split( items, m_browseInfoPath.GetPtr(), EDIT_SEP );

		m_pathItems.resize( items.size() );
		for ( unsigned int i = 0; i != items.size(); ++i )
			m_pathItems[ i ].Parse( items[ i ] );

		SetupPathListBox( 0 );
	}
	else
	{	// save controls
		std::vector< fs::CPath > uniqueDirs;
		for ( unsigned int i = 0; i != m_pathItems.size(); ++i )
			if ( utl::AddUnique( uniqueDirs, m_pathItems[ i ].m_searchPath ) )
				items.push_back( m_pathItems[ i ].Format() );

		m_folderContent.FilterItems( items );
		m_browseInfoPath.Set( str::Join( items, EDIT_SEP ) );
	}

	CLayoutPropertyPage::DoDataExchange( pDX );
}


BEGIN_MESSAGE_MAP( CBscPathPage, CLayoutPropertyPage )
	ON_LBN_SELCHANGE( IDC_BROWSE_FILES_PATH_LIST, LBnSelChange_BrowseFilesPath )
	ON_LBN_DBLCLK( IDC_BROWSE_FILES_PATH_LIST, LBnDblClk_BrowseFilesPath )
	ON_EN_CHANGE( IDC_DISPLAY_TAG_EDIT, EnChange_DisplayTag )
	ON_EN_CHANGE( IDC_SEARCH_FILTER_EDIT, EnChange_SearchFilter )
	ON_COMMAND( CM_ADD_BSC_DIR_PATH, CmAddBscPath )
	ON_COMMAND( CM_REMOVE_BSC_DIR_PATH, CmRemoveBscPath )
	ON_UPDATE_COMMAND_UI( CM_REMOVE_BSC_DIR_PATH, UUI_RemoveBscPath )
	ON_COMMAND( CM_EDIT_BSC_DIR_PATH, CmEditBscPath )
	ON_UPDATE_COMMAND_UI( CM_EDIT_BSC_DIR_PATH, UUI_EditBscPath )
	ON_COMMAND( CM_MOVE_UP_BSC_DIR_PATH, CmMoveUpBscPath )
	ON_UPDATE_COMMAND_UI( CM_MOVE_UP_BSC_DIR_PATH, UUI_MoveUpBscPath )
	ON_COMMAND( CM_MOVE_DOWN_BSC_DIR_PATH, CmMoveDownBscPath )
	ON_UPDATE_COMMAND_UI( CM_MOVE_DOWN_BSC_DIR_PATH, UUI_MoveDownBscPath )
END_MESSAGE_MAP()

void CBscPathPage::LBnSelChange_BrowseFilesPath( void )
{
	std::tstring searchFilter, m_displayName;
	int selIndex = m_pathsListBox.GetCurSel();
	if ( selIndex != LB_ERR )
	{
		searchFilter = m_pathItems[ selIndex ].m_searchPath.GetFilename();
		m_displayName = m_pathItems[ selIndex ].m_displayName;
	}

	m_isUserUpdate = false;

	ui::SetWindowText( m_searchFilterEdit, searchFilter );
	m_searchFilterEdit.SetReadOnly( LB_ERR == selIndex );
	ui::SetWindowText( m_displayTagEdit, m_displayName );
	m_displayTagEdit.SetReadOnly( LB_ERR == selIndex );

	m_isUserUpdate = true;

	UpdateToolBarCmdUI();
}

void CBscPathPage::LBnDblClk_BrowseFilesPath( void )
{
	int selIndex = m_pathsListBox.GetCurSel();
	if ( selIndex != LB_ERR )
		SendMessage( WM_COMMAND, MAKEWPARAM( CM_EDIT_BSC_DIR_PATH, BN_CLICKED ), 0L );
}

void CBscPathPage::EnChange_SearchFilter( void )
{
	if ( m_isUserUpdate )
	{
		int selIndex = m_pathsListBox.GetCurSel();
		if ( selIndex != LB_ERR )
			m_pathItems[ selIndex ].m_searchPath.SetFilename( ui::GetWindowText( m_searchFilterEdit ) );
	}
}

void CBscPathPage::EnChange_DisplayTag( void )
{
	if ( m_isUserUpdate )
	{
		int selIndex = m_pathsListBox.GetCurSel();
		if ( selIndex != LB_ERR )
			m_pathItems[ selIndex ].m_displayName = ui::GetWindowText( m_displayTagEdit );
	}
}

void CBscPathPage::CmAddBscPath( void )
{
	CSearchPathItem newItem;
	int selIndex = m_pathsListBox.GetCurSel();

	if ( selIndex != LB_ERR )
		newItem.m_searchPath = m_pathItems[ selIndex ].m_searchPath;

	fs::CPath searchPath = m_folderContent.EditItem( newItem.m_searchPath.GetPtr(), this );
	if ( !searchPath.IsEmpty() )
	{
		searchPath.SetBackslash( false );

		std::tstring error;
		size_t pos = utl::FindPos( m_pathItems, searchPath );
		if ( pos != utl::npos )
			error = str::Format( _T("Path is not unique:\r\n%s"), searchPath.GetPtr() );
		else
		{
			pos = m_pathItems.size();

			newItem.m_searchPath = searchPath;
			newItem.AugmentWildSpec();

			m_pathItems.push_back( newItem );
			pos = m_pathsListBox.AddString( m_pathItems[ pos ].m_searchPath.GetPtr() );
		}

		m_pathsListBox.SetCurSel( static_cast<int>( pos ) );
		LBnSelChange_BrowseFilesPath();

		if ( !error.empty() )
			AfxMessageBox( error.c_str() );
	}
}

void CBscPathPage::CmRemoveBscPath( void )
{
	int selIndex = m_pathsListBox.GetCurSel();

	ASSERT( selIndex != LB_ERR );
	m_pathItems.erase( m_pathItems.begin() + selIndex );
	m_pathsListBox.DeleteString( selIndex );

	if ( selIndex == m_pathsListBox.GetCount() )
		--selIndex;

	if ( selIndex >= 0 )
		m_pathsListBox.SetCurSel( selIndex );

	LBnSelChange_BrowseFilesPath();
}

void CBscPathPage::UUI_RemoveBscPath( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_pathsListBox.GetCurSel() != LB_ERR );
}

void CBscPathPage::CmEditBscPath( void )
{
	int selIndex = m_pathsListBox.GetCurSel();
	ASSERT( selIndex != LB_ERR );

	CSearchPathItem* pSearchItem = &m_pathItems[ selIndex ];
	fs::CPath searchPath = m_folderContent.EditItem( pSearchItem->m_searchPath.GetPtr(), this );

	if ( !searchPath.IsEmpty() )
	{
		pSearchItem->m_searchPath = searchPath;
		pSearchItem->AugmentWildSpec();

		m_pathsListBox.DeleteString( selIndex );
		m_pathsListBox.InsertString( selIndex, pSearchItem->m_searchPath.GetPtr() );
		m_pathsListBox.SetCurSel( selIndex );
		LBnSelChange_BrowseFilesPath();
	}
}

void CBscPathPage::UUI_EditBscPath( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_pathsListBox.GetCurSel() != LB_ERR );
}

void CBscPathPage::CmMoveUpBscPath( void )
{
	int srcIndex = m_pathsListBox.GetCurSel(), destIndex = srcIndex - 1;

	destIndex = MoveItemTo( srcIndex, destIndex );
	SetupPathListBox( destIndex );
}

void CBscPathPage::UUI_MoveUpBscPath( CCmdUI* pCmdUI )
{
	int selIndex = m_pathsListBox.GetCurSel();
	pCmdUI->Enable( selIndex != LB_ERR && selIndex > 0 );
}

void CBscPathPage::CmMoveDownBscPath( void )
{
	int srcIndex = m_pathsListBox.GetCurSel(), destIndex = srcIndex + 1;

	destIndex = MoveItemTo( srcIndex, destIndex );
	SetupPathListBox( destIndex );
}

void CBscPathPage::UUI_MoveDownBscPath( CCmdUI* pCmdUI )
{
	int selIndex = m_pathsListBox.GetCurSel();
	pCmdUI->Enable( selIndex != LB_ERR && selIndex < m_pathsListBox.GetCount() - 1 );
}


// CBscPathPage::CSearchPathItem implementation

fs::CPath& CBscPathPage::CSearchPathItem::AugmentWildSpec( void )
{
	if ( str::IsEmpty( m_searchPath.GetFilenamePtr() ) || fs::IsValidDirectory( m_searchPath.GetPtr() ) )
		m_searchPath.SetFilename( _T("*.bsc") );

	return m_searchPath;
}

std::tstring CBscPathPage::CSearchPathItem::Format( void )
{
	AugmentWildSpec();

	std::tstring itemText = m_searchPath.Get();
	if ( !m_displayName.empty() )
		itemText += PROF_SEP + m_displayName;
	return itemText;
}

void CBscPathPage::CSearchPathItem::Parse( const std::tstring& itemText )
{
	size_t sepPos = itemText.find( PROF_SEP );
	if ( sepPos != std::tstring::npos )
	{
		m_searchPath.Set( itemText.substr( 0, sepPos ) );
		m_displayName = itemText.substr( sepPos + 1 );
	}
	else
	{
		m_searchPath.Set( itemText );
		m_displayName.clear();
	}
}


class CDirSetDialog : public CDialog
{
public:
	CDirSetDialog( CWnd* pParent ) : CDialog( IDD_DIRECTORY_SET_DIALOG, pParent ) {}
public:
	std::tstring m_name;
protected:
	virtual void DoDataExchange( CDataExchange* pDX )
	{
		ui::DDX_Text( pDX, IDC_DIR_SET_NAME_EDIT, m_name, true );
		CDialog::DoDataExchange( pDX );
	}
};

namespace hlp
{
	void DDX_DirPath( CDataExchange* pDX, int ctrlId, CItemListEdit& rEdit, inc::CDirPathGroup& rDirPaths )
	{
		DDX_Control( pDX, ctrlId, rEdit );

		if ( DialogOutput == pDX->m_bSaveAndValidate )
			rEdit.SetText( rDirPaths.Join() );
		else
			rDirPaths.Split( rEdit.GetText().c_str() );
	}
}


// CDirectoriesPage property page

CDirectoriesPage::CDirectoriesPage( void )
	: CLayoutPropertyPage( IDD_OPTIONS_DIRECTORIES_PAGE )
	, m_pDirSets( new CIncludeDirectories( CIncludeDirectories::Instance() ) )
{
	m_includePathEdit.SetContentType( ui::DirPath );
	m_sourcePathEdit.SetContentType( ui::DirPath );
	m_libraryPathEdit.SetContentType( ui::DirPath );
	m_binaryPathEdit.SetContentType( ui::DirPath );

	m_toolbar.GetStrip()
		.AddButton( ID_ADD_ITEM )
		.AddButton( ID_REMOVE_ITEM )
		.AddSeparator()
		.AddButton( ID_RENAME_ITEM )
		.AddSeparator()
		.AddButton( ID_RESET_DEFAULT )
		.AddSeparator()
		.AddButton( ID_EDIT_COPY )
		.AddButton( ID_EDIT_PASTE );
}

CDirectoriesPage::~CDirectoriesPage()
{
}

std::tstring CDirectoriesPage::MakeNewUniqueName( void ) const
{
	std::tstring name;
	for ( size_t count = 1; ; ++count )
	{
		name = str::Format( _T("Folder Set #%d"), count );
		if ( !m_pDirSets->Get().Contains( name ) )
			break;
	}

	return name;
}

void CDirectoriesPage::ApplyPageChanges( void ) throws_( CRuntimeException )
{
	CIncludeDirectories& rDestDirSets = CIncludeDirectories::Instance();
	rDestDirSets.Assign( *m_pDirSets );
	rDestDirSets.Save();
}

void CDirectoriesPage::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_DIR_SETS_COMBO, m_dirSetsCombo );
	m_toolbar.DDX_Placeholder( pDX, IDC_TOOLBAR_PLACEHOLDER, H_AlignLeft | V_AlignCenter );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		std::vector< std::tstring > setNames; setNames.reserve( m_pDirSets->Get().size() );
		for ( utl::vector_map< std::tstring, CIncludePaths* > ::const_iterator itPathSet = m_pDirSets->Get().begin(); itPathSet != m_pDirSets->Get().end(); ++itPathSet )
			setNames.push_back( itPathSet->first );

		ui::WriteComboItems( m_dirSetsCombo, setNames );
		m_dirSetsCombo.SetCurSel( m_pDirSets->GetCurrentPos() );
	}
	else
		m_pDirSets->SetCurrentPos( m_dirSetsCombo.GetCurSel() );

	CIncludePaths* pIncludePaths = m_pDirSets->RefCurrentPaths();

	hlp::DDX_DirPath( pDX, IDC_INCLUDE_PATH_EDIT, m_includePathEdit, pIncludePaths->m_standard );
	hlp::DDX_DirPath( pDX, IDC_SOURCE_PATH_EDIT, m_sourcePathEdit, pIncludePaths->m_source );
	hlp::DDX_DirPath( pDX, IDC_LIBRARY_PATH_EDIT, m_libraryPathEdit, pIncludePaths->m_library );
	hlp::DDX_DirPath( pDX, IDC_BINARY_PATH_EDIT, m_binaryPathEdit, pIncludePaths->m_binary );

	CLayoutPropertyPage::DoDataExchange( pDX );
}


BEGIN_MESSAGE_MAP( CDirectoriesPage, CLayoutPropertyPage )
	ON_CBN_SELCHANGE( IDC_DIR_SETS_COMBO, CBnSelChange_DirSets )
	ON_COMMAND( ID_ADD_ITEM, OnAddDirSet )
	ON_COMMAND( ID_REMOVE_ITEM, OnRemoveDirSet )
	ON_COMMAND( ID_RENAME_ITEM, OnRenameDirSet )
	ON_COMMAND( ID_RESET_DEFAULT, OnResetDefault )
	ON_COMMAND( ID_EDIT_COPY, OnCopyPaths )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateCopyPaths )
	ON_COMMAND( ID_EDIT_PASTE, OnPastePaths )
	ON_UPDATE_COMMAND_UI( ID_EDIT_PASTE, OnUpdatePastePaths )
END_MESSAGE_MAP()

void CDirectoriesPage::CBnSelChange_DirSets( void )
{
	m_pDirSets->SetCurrentPos( m_dirSetsCombo.GetCurSel() );
	SetModified( true );
	UpdateData( DialogOutput );
}

void CDirectoriesPage::OnAddDirSet( void )
{
	CDirSetDialog dlg( this );
	dlg.m_name = MakeNewUniqueName();

	if ( IDOK == dlg.DoModal() )
		if ( !dlg.m_name.empty() && !m_pDirSets->Get().Contains( dlg.m_name ) )
		{
			m_pDirSets->Add( dlg.m_name );
			UpdateData( DialogOutput );
		}
		else
			AfxMessageBox( _T("Invalid directory set name!\n\nIt must me non-empty and unique."), MB_ICONERROR );
}

void CDirectoriesPage::OnRemoveDirSet( void )
{
	m_pDirSets->RemoveCurrent();
	UpdateData( DialogOutput );
}

void CDirectoriesPage::OnRenameDirSet( void )
{
	CDirSetDialog dlg( this );
	dlg.m_name = m_pDirSets->GetCurrentName();

	if ( IDOK == dlg.DoModal() )
		if ( !dlg.m_name.empty() && !m_pDirSets->Get().Contains( dlg.m_name ) )
		{
			m_pDirSets->RenameCurrent( dlg.m_name );
			UpdateData( DialogOutput );
		}
		else if ( dlg.m_name != m_pDirSets->GetCurrentName() )
			AfxMessageBox( _T("Invalid directory set name!\n\nIt must me non-empty and unique."), MB_ICONERROR );
}

void CDirectoriesPage::OnResetDefault( void )
{
	if ( IDYES == AfxMessageBox( _T("Are you sure you want to delete all working sets and initialize to the default one?"), MB_YESNO | MB_ICONQUESTION ) )
	{
		m_pDirSets->Reset();
		UpdateData( DialogOutput );
	}
}

void CDirectoriesPage::OnCopyPaths( void )
{
	const CIncludePaths* pIncludePaths = m_pDirSets->GetCurrentPaths();
	CTextClipboard::CopyText( pIncludePaths->FormatLines(), m_hWnd );
}

void CDirectoriesPage::OnUpdateCopyPaths( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_pDirSets->GetCurrentPaths() != NULL );
}

void CDirectoriesPage::OnPastePaths( void )
{
	std::tstring linesSpec;
	if ( CTextClipboard::PasteText( linesSpec, m_hWnd ) )
	{
		CIncludePaths* pIncludePaths = m_pDirSets->RefCurrentPaths();
		if ( pIncludePaths->ParseLines( linesSpec ) )
			UpdateData( DialogOutput );
		else
			ui::ReportError( str::Format( _T("Error parsing input text:\n\n%s"), linesSpec.c_str() ) );
	}
}

void CDirectoriesPage::OnUpdatePastePaths( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( CTextClipboard::CanPasteText() && m_pDirSets->GetCurrentPaths() != NULL );
}
