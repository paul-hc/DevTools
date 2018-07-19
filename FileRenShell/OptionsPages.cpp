
#include "stdafx.h"
#include "OptionsSheet.h"
#include "OptionsPages.h"
#include "FileModel.h"
#include "FileCommands.h"
#include "IFileEditor.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"
#include "utl/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// COptionsSheet implementation

COptionsSheet::COptionsSheet( CFileModel* pFileModel, CWnd* pParent, UINT initialPageIndex /*= UINT_MAX*/ )
	: CLayoutPropertySheet( _T("Options"), pParent )
	, m_pFileModel( pFileModel )
	, m_pFileEditor( dynamic_cast< IFileEditor* >( pParent ) )
{
	m_regSection = _T("OptionsSheet");
	SetTopDlg();
	SetInitialPageIndex( initialPageIndex );
	LoadDlgIcon( ID_OPTIONS );

	AddPage( new CGeneralOptionsPage );
	AddPage( new CCapitalizeOptionsPage );
}

void COptionsSheet::OnChangesApplied( void )
{
	if ( m_pApplyMacroCmd.get() != NULL && !m_pApplyMacroCmd->IsEmpty() )
		m_pFileModel->SafeExecuteCmd( m_pFileEditor, m_pApplyMacroCmd.release() );

	if ( !CGeneralOptions::Instance().m_undoEditingCmds )
		m_pFileModel->GetCommandModel()->RemoveCommandsThat( pred::IsNotA< cmd::CFileMacroCmd >() );
}


// CGeneralOptionsPage implementation

namespace layout
{
	static CLayoutStyle stylesGen[] =
	{
		{ IDC_GROUP_BOX_1, SizeX },
		{ IDC_GROUP_BOX_2, SizeX },
		{ IDC_SET_DEFAULT_ALL, MoveY }
	};
}

const CGeneralOptions CGeneralOptionsPage::s_defaultOptions;

CGeneralOptionsPage::CGeneralOptionsPage( void )
	: CLayoutPropertyPage( IDD_OPTIONS_GENERAL_PAGE )
	, m_options( CGeneralOptions::Instance() )
{
	RegisterCtrlLayout( layout::stylesGen, COUNT_OF( layout::stylesGen ) );
}

CGeneralOptionsPage::~CGeneralOptionsPage()
{
}

const CEnumTags& CGeneralOptionsPage::GetTags_IconDim( void )
{
	static const CEnumTags tags( _T("16 x 16|24 x 24|32 x 32|48 x 48|96 x 96|128 x 128|256 x 256"), NULL, -1, SmallIcon );		// matches IconStdSize starting from SmallIcon
	return tags;
}

void CGeneralOptionsPage::UpdateStatus( void )
{
	static const UINT s_comboIds[] = { IDC_SMALL_ICON_SIZE_COMBO, IDC_LARGE_ICON_SIZE_COMBO };
	ui::EnableControls( m_hWnd, s_comboIds, COUNT_OF( s_comboIds ), m_options.m_useListThumbs );

	ui::EnableControl( m_hWnd, IDC_SET_DEFAULT_ALL, m_options != s_defaultOptions );
}

void CGeneralOptionsPage::ApplyPageChanges( void ) throws_( CRuntimeException )
{
	if ( m_options != CGeneralOptions::Instance() )
		GetApplyMacroCmd()->AddCmd( new CEditOptionsCmd< CGeneralOptions >( &CGeneralOptions::Instance(), m_options ) );
}

void CGeneralOptionsPage::DoDataExchange( CDataExchange* pDX )
{
	ui::DDX_ButtonIcon( pDX, IDC_SET_DEFAULT_ALL, ID_RESET_DEFAULT );

	IconStdSize smallStdSize, largeStdSize;
	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		smallStdSize = ui::LookupIconStdSize( m_options.m_smallIconDim, ui::LookupIconStdSize( s_defaultOptions.m_smallIconDim ) );
		largeStdSize = ui::LookupIconStdSize( m_options.m_largeIconDim, ui::LookupIconStdSize( s_defaultOptions.m_largeIconDim ) );
	}

	ui::DDX_EnumCombo( pDX, IDC_SMALL_ICON_SIZE_COMBO, m_smallIconDimCombo, smallStdSize, GetTags_IconDim() );
	ui::DDX_EnumCombo( pDX, IDC_LARGE_ICON_SIZE_COMBO, m_largeIconDimCombo, largeStdSize, GetTags_IconDim() );

	if ( DialogSaveChanges == pDX->m_bSaveAndValidate )
	{
		m_options.m_smallIconDim = ui::GetIconDimension( smallStdSize );
		m_options.m_largeIconDim = ui::GetIconDimension( largeStdSize );
	}

	ui::DDX_Bool( pDX, IDC_USE_LIST_THUMBS_CHECK, m_options.m_useListThumbs );
	ui::DDX_Bool( pDX, IDC_USE_LIST_DOUBLE_BUFFER_CHECK, m_options.m_useListDoubleBuffer );
	ui::DDX_Bool( pDX, IDC_HIGHLIGHT_TEXT_DIFFS_FRAME_CHECK, m_options.m_highlightTextDiffsFrame );
	ui::DDX_Bool( pDX, IDC_UNDO_LOG_PERSIST_CHECK, m_options.m_undoLogPersist );
	ui::DDX_RadioEnum( pDX, IDC_UNDO_LOG_TEXT_FMT_RADIO, m_options.m_undoLogFormat );
	ui::DDX_Bool( pDX, IDC_UNDO_EDITING_CMDS_CHECK, m_options.m_undoEditingCmds );

	UpdateStatus();
	CLayoutPropertyPage::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CGeneralOptionsPage, CLayoutPropertyPage )
	ON_CBN_SELCHANGE( IDC_SMALL_ICON_SIZE_COMBO, OnFieldModified )
	ON_CBN_SELCHANGE( IDC_LARGE_ICON_SIZE_COMBO, OnFieldModified )
	ON_BN_CLICKED( IDC_USE_LIST_THUMBS_CHECK, OnFieldModified )
	ON_BN_CLICKED( IDC_USE_LIST_DOUBLE_BUFFER_CHECK, OnFieldModified )
	ON_BN_CLICKED( IDC_HIGHLIGHT_TEXT_DIFFS_FRAME_CHECK, OnFieldModified )
	ON_BN_CLICKED( IDC_UNDO_LOG_PERSIST_CHECK, OnFieldModified )
	ON_BN_CLICKED( IDC_UNDO_LOG_TEXT_FMT_RADIO, OnFieldModified )
	ON_BN_CLICKED( IDC_UNDO_LOG_BINARY_FMT_RADIO, OnFieldModified )
	ON_BN_CLICKED( IDC_UNDO_EDITING_CMDS_CHECK, OnFieldModified )
	ON_BN_CLICKED( IDC_SET_DEFAULT_ALL, OnBnClicked_ResetDefaultAll )
END_MESSAGE_MAP()

void CGeneralOptionsPage::OnFieldModified( void )
{
	UpdateData( DialogSaveChanges );

	__super::OnFieldModified();
}

void CGeneralOptionsPage::OnBnClicked_ResetDefaultAll( void )
{
	m_options = s_defaultOptions;
	UpdateData( DialogOutput );
	OnFieldModified();
}


// CCapitalizeOptionsPage property page

namespace reg
{
	static const TCHAR section_capitalizePage[] = _T("CapitalizeOptionsPage");
}

namespace layout
{
	static CLayoutStyle stylesCap[] =
	{
		{ IDC_WORD_BREAK_CHARS_EDIT, SizeX },
		{ IDC_WORD_BREAK_PREFIXES_EDIT, SizeX },
		{ IDC_WORDS_PRESERVED_EDIT, SizeX },
		{ IDC_WORDS_UPPERCASE_EDIT, SizeX },
		{ IDC_WORDS_LOWERCASE_EDIT, SizeX },
		{ IDC_GROUP_BOX_1, SizeX },
		{ IDC_ARTICLES_EDIT, SizeX },
		{ IDC_ARTICLES_COMBO, MoveX },
		{ IDC_CONJUNCTIONS_EDIT, SizeX },
		{ IDC_CONJUNCTIONS_COMBO, MoveX },
		{ IDC_PREPOSITIONS_EDIT, SizeX },
		{ IDC_PREPOSITIONS_COMBO, MoveX },
		{ IDOK, Move },
		{ IDCANCEL, Move },
		{ IDC_SET_DEFAULT_ALL, MoveY }
	};
}

const CCapitalizeOptions CCapitalizeOptionsPage::s_defaultOptions;

CCapitalizeOptionsPage::CCapitalizeOptionsPage( void )
	: CLayoutPropertyPage( IDD_OPTIONS_CAPITALIZE_PAGE )
	, m_options( CCapitalizeOptions::Instance() )				// copy by value
	, m_wordBreakPrefixesEdit( cap::CWordList::s_listSep )
	, m_alwaysPreserveWordsEdit( cap::CWordList::s_listSep )
	, m_alwaysUppercaseWordsEdit( cap::CWordList::s_listSep )
	, m_alwaysLowercaseWordsEdit( cap::CWordList::s_listSep )
	, m_articlesEdit( cap::CWordList::s_listSep )
	, m_conjunctionsEdit( cap::CWordList::s_listSep )
	, m_prepositionsEdit( cap::CWordList::s_listSep )
{
	RegisterCtrlLayout( layout::stylesCap, COUNT_OF( layout::stylesCap ) );
}

CCapitalizeOptionsPage::~CCapitalizeOptionsPage()
{
}

void CCapitalizeOptionsPage::UpdateStatus( void )
{
	ui::EnableControl( m_hWnd, IDC_SET_DEFAULT_ALL, m_options != s_defaultOptions );
}

void CCapitalizeOptionsPage::ApplyPageChanges( void ) throws_( CRuntimeException )
{
	if ( m_options != CCapitalizeOptions::Instance() )
		GetApplyMacroCmd()->AddCmd( new CEditOptionsCmd< CCapitalizeOptions >( &CCapitalizeOptions::Instance(), m_options ) );
}

void CCapitalizeOptionsPage::DoDataExchange( CDataExchange* pDX )
{
	m_wordBreakCharsEdit.DDX_UiEscapeSeqs( pDX, m_options.m_wordBreakChars, IDC_WORD_BREAK_CHARS_EDIT );
	m_wordBreakPrefixesEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_wordBreakPrefixes.m_words, IDC_WORD_BREAK_PREFIXES_EDIT );
	m_alwaysPreserveWordsEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_alwaysPreserveWords.m_words, IDC_WORDS_PRESERVED_EDIT );
	m_alwaysUppercaseWordsEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_alwaysUppercaseWords.m_words, IDC_WORDS_UPPERCASE_EDIT );
	m_alwaysLowercaseWordsEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_alwaysLowercaseWords.m_words, IDC_WORDS_LOWERCASE_EDIT );

	m_articlesEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_articles.m_words, IDC_ARTICLES_EDIT );
	m_conjunctionsEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_conjunctions.m_words, IDC_CONJUNCTIONS_EDIT );
	m_prepositionsEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_prepositions.m_words, IDC_PREPOSITIONS_EDIT );

	ui::DDX_EnumCombo( pDX, IDC_ARTICLES_COMBO, m_articlesCombo, m_options.m_articles.m_caseModify, cap::GetTags_CaseModify() );
	ui::DDX_EnumCombo( pDX, IDC_CONJUNCTIONS_COMBO, m_conjunctionsCombo, m_options.m_conjunctions.m_caseModify, cap::GetTags_CaseModify() );
	ui::DDX_EnumCombo( pDX, IDC_PREPOSITIONS_COMBO, m_prepositionsCombo, m_options.m_prepositions.m_caseModify, cap::GetTags_CaseModify() );

	ui::DDX_Bool( pDX, IDC_SKIP_NUMERIC_PREFIX_CHECK, m_options.m_skipNumericPrefix );
	ui::DDX_ButtonIcon( pDX, IDC_SET_DEFAULT_ALL, ID_RESET_DEFAULT );

	UpdateStatus();
	CLayoutPropertyPage::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CCapitalizeOptionsPage, CLayoutPropertyPage )
	ON_EN_CHANGE( IDC_WORD_BREAK_CHARS_EDIT, OnFieldModified )
	ON_EN_CHANGE( IDC_WORD_BREAK_PREFIXES_EDIT, OnFieldModified )
	ON_EN_CHANGE( IDC_WORDS_PRESERVED_EDIT, OnFieldModified )
	ON_EN_CHANGE( IDC_WORDS_UPPERCASE_EDIT, OnFieldModified )
	ON_EN_CHANGE( IDC_WORDS_LOWERCASE_EDIT, OnFieldModified )
	ON_EN_CHANGE( IDC_ARTICLES_EDIT, OnFieldModified )
	ON_EN_CHANGE( IDC_CONJUNCTIONS_EDIT, OnFieldModified )
	ON_EN_CHANGE( IDC_PREPOSITIONS_EDIT, OnFieldModified )
	ON_CBN_SELCHANGE( IDC_ARTICLES_COMBO, OnFieldModified )
	ON_CBN_SELCHANGE( IDC_CONJUNCTIONS_COMBO, OnFieldModified )
	ON_CBN_SELCHANGE( IDC_PREPOSITIONS_COMBO, OnFieldModified )
	ON_BN_CLICKED( IDC_SKIP_NUMERIC_PREFIX_CHECK, OnFieldModified )
	ON_BN_CLICKED( IDC_SET_DEFAULT_ALL, OnBnClicked_ResetDefaultAll )
END_MESSAGE_MAP()

void CCapitalizeOptionsPage::OnFieldModified( void )
{
	UpdateData( DialogSaveChanges );

	__super::OnFieldModified();
}

void CCapitalizeOptionsPage::OnBnClicked_ResetDefaultAll( void )
{
	m_options = s_defaultOptions;
	UpdateData( DialogOutput );
	OnFieldModified();
}
