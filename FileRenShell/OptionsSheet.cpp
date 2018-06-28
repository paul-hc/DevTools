
#include "stdafx.h"
#include "OptionsSheet.h"
#include "GeneralOptions.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"
#include "utl/resource.h"
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// COptionsSheet implementation

COptionsSheet::COptionsSheet( CWnd* pParent, UINT initialPageIndex /*= UINT_MAX*/ )
	: CLayoutPropertySheet( _T("Options"), pParent )
{
	m_regSection = _T("OptionsSheet");
	m_alwaysModified = true;
	SetTopDlg();
	SetSingleTransaction();
	SetInitialPageIndex( initialPageIndex );
	LoadDlgIcon( ID_OPTIONS );

	AddPage( new CGeneralOptionsPage );
	AddPage( new CCapitalizeOptionsPage );
}


// CGeneralOptionsPage implementation

namespace layout
{
	static CLayoutStyle stylesGen[] =
	{
		{ IDC_SET_DEFAULT_ALL, MoveY }
	};
}

CGeneralOptionsPage::CGeneralOptionsPage( void )
	: CLayoutPropertyPage( IDD_OPTIONS_GENERAL_PAGE )
	, m_pOptions( new CGeneralOptions( CGeneralOptions::Instance() ) )
{
	RegisterCtrlLayout( layout::stylesGen, COUNT_OF( layout::stylesGen ) );
}

CGeneralOptionsPage::~CGeneralOptionsPage()
{
}

const CEnumTags& CGeneralOptionsPage::GetTags_IconStdSize( void )
{
	static const CEnumTags tags( _T("16 x 16|24 x 24|32 x 32|48 x 48|96 x 96|128 x 128|256 x 256"), NULL, -1, SmallIcon );
	return tags;
}

void CGeneralOptionsPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_smallIconSizeCombo.m_hWnd;

	DDX_Control( pDX, IDC_SMALL_ICON_SIZE_COMBO, m_smallIconSizeCombo );
	DDX_Control( pDX, IDC_LARGE_ICON_SIZE_COMBO, m_largeIconSizeCombo );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			ui::WriteComboItems( m_smallIconSizeCombo, GetTags_IconStdSize().GetUiTags() );
			ui::WriteComboItems( m_largeIconSizeCombo, GetTags_IconStdSize().GetUiTags() );
		}

		m_smallIconSizeCombo.SetCurSel( GetTags_IconStdSize().GetTagIndex( m_pOptions->m_smallIconStdSize ) );
		m_largeIconSizeCombo.SetCurSel( GetTags_IconStdSize().GetTagIndex( m_pOptions->m_largeIconStdSize ) );
	}
	else
	{
		m_pOptions->m_smallIconStdSize = GetTags_IconStdSize().GetSelValue< IconStdSize >( m_smallIconSizeCombo.GetCurSel() );
		m_pOptions->m_largeIconStdSize = GetTags_IconStdSize().GetSelValue< IconStdSize >( m_largeIconSizeCombo.GetCurSel() );

		if ( *m_pOptions != CGeneralOptions::Instance() )
		{
			CGeneralOptions::Instance() = *m_pOptions;

			CGeneralOptions::Instance().UpdateAllObservers( NULL );
			CGeneralOptions::Instance().SaveToRegistry();
		}
	}

	CLayoutPropertyPage::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CGeneralOptionsPage, CLayoutPropertyPage )
	ON_BN_CLICKED( IDC_SET_DEFAULT_ALL, OnBnClicked_ResetDefaultAll )
END_MESSAGE_MAP()

void CGeneralOptionsPage::OnBnClicked_ResetDefaultAll( void )
{
	*m_pOptions = CGeneralOptions();
	UpdateData( DialogOutput );
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

CCapitalizeOptionsPage::CCapitalizeOptionsPage( void )
	: CLayoutPropertyPage( IDD_CAPITALIZE_OPTIONS_PAGE )
	, m_options( CCapitalizeOptions::Instance() )				// copy by value
	, m_wordBreakPrefixesEdit( cap::CWordList::m_listSep )
	, m_alwaysPreserveWordsEdit( cap::CWordList::m_listSep )
	, m_alwaysUppercaseWordsEdit( cap::CWordList::m_listSep )
	, m_alwaysLowercaseWordsEdit( cap::CWordList::m_listSep )
	, m_articlesEdit( cap::CWordList::m_listSep )
	, m_conjunctionsEdit( cap::CWordList::m_listSep )
	, m_prepositionsEdit( cap::CWordList::m_listSep )
{
	RegisterCtrlLayout( layout::stylesCap, COUNT_OF( layout::stylesCap ) );
}

CCapitalizeOptionsPage::~CCapitalizeOptionsPage()
{
}

void CCapitalizeOptionsPage::DoDataExchange( CDataExchange* pDX )
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

		CCapitalizeOptions::Instance() = m_options;				// copy by value
		CCapitalizeOptions::Instance().SaveToRegistry();
	}

	CLayoutPropertyPage::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CCapitalizeOptionsPage, CLayoutPropertyPage )
	ON_BN_CLICKED( IDC_SET_DEFAULT_ALL, OnBnClicked_ResetDefaultAll )
END_MESSAGE_MAP()

void CCapitalizeOptionsPage::OnBnClicked_ResetDefaultAll( void )
{
	m_options = CCapitalizeOptions();
	UpdateData( DialogOutput );
}
