
#include "stdafx.h"
#include "CapitalizeOptionsDialog.h"
#include "utl/EnumTags.h"
#include "utl/LayoutEngine.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"
#include "utl/resource.h"
#include "resource.h"


namespace reg
{
	static const TCHAR section_dialog[] = _T("CapitalizeOptionsDialog");
}


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_WORD_BREAK_CHARS_EDIT, StretchX },
		{ IDC_WORD_BREAK_PREFIXES_EDIT, StretchX },
		{ IDC_WORDS_PRESERVED_EDIT, StretchX },
		{ IDC_WORDS_UPPERCASE_EDIT, StretchX },
		{ IDC_WORDS_LOWERCASE_EDIT, StretchX },
		{ IDC_GROUP_BOX_1, StretchX },
		{ IDC_ARTICLES_EDIT, StretchX },
		{ IDC_ARTICLES_COMBO, OffsetX },
		{ IDC_CONJUNCTIONS_EDIT, StretchX },
		{ IDC_CONJUNCTIONS_COMBO, OffsetX },
		{ IDC_PREPOSITIONS_EDIT, StretchX },
		{ IDC_PREPOSITIONS_COMBO, OffsetX },
		{ IDOK, Offset },
		{ IDCANCEL, Offset },
		{ IDC_SET_DEFAULT_ALL, Offset }
	};
}


CCapitalizeOptionsDialog::CCapitalizeOptionsDialog( CWnd* pParent /*= NULL*/ )
	: CLayoutDialog( IDD_CAPITALIZE_OPTIONS, pParent )
	, m_wordBreakPrefixesEdit( cap::CWordList::m_listSep )
	, m_alwaysPreserveWordsEdit( cap::CWordList::m_listSep )
	, m_alwaysUppercaseWordsEdit( cap::CWordList::m_listSep )
	, m_alwaysLowercaseWordsEdit( cap::CWordList::m_listSep )
	, m_articlesEdit( cap::CWordList::m_listSep )
	, m_conjunctionsEdit( cap::CWordList::m_listSep )
	, m_prepositionsEdit( cap::CWordList::m_listSep )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	GetLayoutEngine().MaxClientSize().cy = -1;
	LoadDlgIcon( IDD_CAPITALIZE_OPTIONS );
	m_options.LoadFromRegistry();
}

CCapitalizeOptionsDialog::~CCapitalizeOptionsDialog()
{
}

void CCapitalizeOptionsDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_wordBreakCharsEdit.GetSafeHwnd();

	m_wordBreakCharsEdit.DDX_UiEscapeSeqs( pDX, m_options.m_wordBreakChars, IDC_WORD_BREAK_CHARS_EDIT );
	m_wordBreakPrefixesEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_wordBreakPrefixes.m_words, IDC_WORD_BREAK_PREFIXES_EDIT );
	m_alwaysPreserveWordsEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_alwaysPreserveWords.m_words, IDC_WORDS_PRESERVED_EDIT );
	m_alwaysUppercaseWordsEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_alwaysUppercaseWords.m_words, IDC_WORDS_UPPERCASE_EDIT );
	m_alwaysLowercaseWordsEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_alwaysLowercaseWords.m_words, IDC_WORDS_LOWERCASE_EDIT );
	m_articlesEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_articles.m_words, IDC_ARTICLES_EDIT );
	m_conjunctionsEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_conjunctions.m_words, IDC_CONJUNCTIONS_EDIT );
	m_prepositionsEdit.DDX_ItemsUiEscapeSeqs( pDX, m_options.m_prepositions.m_words, IDC_PREPOSITIONS_EDIT );
	DDX_Control( pDX, IDC_ARTICLES_COMBO, m_articlesCombo );
	DDX_Control( pDX, IDC_CONJUNCTIONS_COMBO, m_conjunctionsCombo );
	DDX_Control( pDX, IDC_PREPOSITIONS_COMBO, m_prepositionsCombo );
	ui::DDX_Bool( pDX, IDC_SKIP_NUMERIC_PREFIX_CHECK, m_options.m_skipNumericPrefix );
	ui::DDX_ButtonIcon( pDX, IDC_SET_DEFAULT_ALL, ID_RESET_DEFAULT );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			ui::WriteComboItems( m_articlesCombo, cap::GetTags_CaseModify().GetUiTags() );
			ui::WriteComboItems( m_conjunctionsCombo, cap::GetTags_CaseModify().GetUiTags() );
			ui::WriteComboItems( m_prepositionsCombo, cap::GetTags_CaseModify().GetUiTags() );
		}

		m_articlesCombo.SetCurSel( m_options.m_articles.m_caseModify );
		m_conjunctionsCombo.SetCurSel( m_options.m_conjunctions.m_caseModify );
		m_prepositionsCombo.SetCurSel( m_options.m_prepositions.m_caseModify );
	}
	else
	{
		m_options.m_articles.m_caseModify = static_cast< cap::CaseModify >( m_articlesCombo.GetCurSel() );
		m_options.m_conjunctions.m_caseModify = static_cast< cap::CaseModify >( m_conjunctionsCombo.GetCurSel() );
		m_options.m_prepositions.m_caseModify = static_cast< cap::CaseModify >( m_prepositionsCombo.GetCurSel() );

		m_options.SaveToRegistry();
	}
	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CCapitalizeOptionsDialog, CLayoutDialog )
	ON_BN_CLICKED( IDC_SET_DEFAULT_ALL, OnBnClicked_ResetDefaultAll )
END_MESSAGE_MAP()

void CCapitalizeOptionsDialog::OnBnClicked_ResetDefaultAll( void )
{
	m_options = CCapitalizeOptions();
	UpdateData( DialogOutput );
}
