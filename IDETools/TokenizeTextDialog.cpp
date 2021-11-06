
#include "stdafx.h"
#include "TokenizeTextDialog.h"
#include "resource.h"
#include "utl/StringUtilities.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_dialog[] = _T("TokenizeTextDialog");
	static const TCHAR entry_action[] = _T("Action");
	static const TCHAR* entry_actionSeparators[ 3 ][ 2 ] =
	{
		{ _T("SplitInputSeparator"), _T("SplitOutputSeparator") },
		{ _T("TokenizeInputTokens"), _T("TokenizeOutputSeparator") },
		{ _T("MergeInputSeparator"), _T("MergeOutputSeparator") }
	};
	static const TCHAR entry_caseSensitive[] = _T("CaseSensitive");
	static const TCHAR entry_removeEmptyLines[] = _T("RemoveEmptyLines");
	static const TCHAR entry_trimLines[] = _T("TrimLines");
	static const TCHAR entry_trimChars[] = _T("TrimChars");
	static const TCHAR entry_filterSource[] = _T("FilterSource");
	static const TCHAR entry_filterOutput[] = _T("FilterOutput");
	static const TCHAR entry_showWhiteSpace[] = _T("ShowWhiteSpace");
}

namespace win
{
	static const TCHAR lineEnd[] = _T("\r\n");
}


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_SOURCE_TEXT_LABEL, SizeX },
		{ IDC_SOURCE_TEXT_EDIT, SizeX },
		{ IDC_INPUT_TOKENS_EDIT, SizeX },
		{ IDC_INPUT_TOKENS_DEFAULT, MoveX },
		{ IDC_OUTPUT_SEPARATOR_EDIT, SizeX },
		{ IDC_OUTPUT_SEPARATOR_DEFAULT, MoveX },
		{ IDC_GROUP_BOX_2, SizeX },
		{ IDC_TRIM_CHARS_EDIT, SizeX },
		{ IDC_TRIM_CHARS_DEFAULT, MoveX },
		{ IDC_OUTPUT_TEXT_LABEL, SizeX },
		{ IDC_OUTPUT_TEXT_EDIT, Size },
		{ IDC_COPY_OUTPUT_TO_SOURCE, MoveX },
		{ IDC_SHOW_WHITESPACE_CHECK, MoveY },
		{ IDOK, Move },
		{ IDCANCEL, Move }
	};
}

const CTokenizeTextDialog::ActionSeparator CTokenizeTextDialog::m_defaultSeparators[ CTokenizeTextDialog::ActionCount ] =
{
	ActionSeparator( _T(";"), _T("\\n") ),
	ActionSeparator( _T(";,="), _T("\\n") ),
	ActionSeparator( _T("\\n"), _T(",") )
};

static const TCHAR defaultTrimChars[] = _T(" \t");


CTokenizeTextDialog::CTokenizeTextDialog( CWnd* pParent /*=NULL*/ )
	: CLayoutDialog( IDD_TOKENIZE_TEXT_DIALOG, pParent )
	, m_action( SplitAction )
	, m_tokenCount( 0 )
	, m_defaultInputTokensButton( ID_RESET_DEFAULT, false )
	, m_defaultOutputSepsButton( ID_RESET_DEFAULT, false )
	, m_defaultTrimCharsButton( ID_RESET_DEFAULT, false )
	, m_copyOutputToSourceButton( IDC_COPY_OUTPUT_TO_SOURCE )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );

	for ( unsigned int i = 0; i != ActionCount; ++i )
		m_separators[ i ] = m_defaultSeparators[ i ];

	RegistryLoad();
}

CTokenizeTextDialog::~CTokenizeTextDialog()
{
}

void CTokenizeTextDialog::RegistryLoad( void )
{
	CWinApp* pApp = AfxGetApp();
	m_action = static_cast<Action>( pApp->GetProfileInt( reg::section_dialog, reg::entry_action, m_action ) );

	for ( unsigned int i = 0; i != SepActionCount; ++i )
	{
		m_separators[ i ].m_input = (LPCTSTR)pApp->GetProfileString( reg::section_dialog, reg::entry_actionSeparators[ i ][ 0 ], m_separators[ i ].m_input.c_str() );
		m_separators[ i ].m_output = (LPCTSTR)pApp->GetProfileString( reg::section_dialog, reg::entry_actionSeparators[ i ][ 1 ], m_separators[ i ].m_output.c_str() );
	}
}

void CTokenizeTextDialog::RegistrySave( void )
{
	CWinApp* pApp = AfxGetApp();
	pApp->WriteProfileInt( reg::section_dialog, reg::entry_action, m_action );
	pApp->WriteProfileInt( reg::section_dialog, reg::entry_caseSensitive, IsDlgButtonChecked( IDC_CASE_SENSITIVE_CHECK ) );
	pApp->WriteProfileInt( reg::section_dialog, reg::entry_removeEmptyLines, IsDlgButtonChecked( IDC_REMOVE_EMPTY_LINES_CHECK ) );
	pApp->WriteProfileInt( reg::section_dialog, reg::entry_trimLines, IsDlgButtonChecked( IDC_TRIM_LINES_CHECK ) );
	pApp->WriteProfileString( reg::section_dialog, reg::entry_trimChars, ui::GetWindowText( m_trimCharsEdit ).c_str() );
	pApp->WriteProfileInt( reg::section_dialog, reg::entry_filterSource, IsDlgButtonChecked( IDC_FILTER_SOURCE_LINES_CHECK ) );
	pApp->WriteProfileInt( reg::section_dialog, reg::entry_filterOutput, IsDlgButtonChecked( IDC_FILTER_OUTPUT_LINES_CHECK ) );
	pApp->WriteProfileInt( reg::section_dialog, reg::entry_showWhiteSpace, IsDlgButtonChecked( IDC_SHOW_WHITESPACE_CHECK ) );

	for ( unsigned int i = 0; i != SepActionCount; ++i )
	{
		pApp->WriteProfileString( reg::section_dialog, reg::entry_actionSeparators[ i ][ 0 ], m_separators[ i ].m_input.c_str() );
		pApp->WriteProfileString( reg::section_dialog, reg::entry_actionSeparators[ i ][ 1 ], m_separators[ i ].m_output.c_str() );
	}
}

std::tstring CTokenizeTextDialog::GenerateOutputText( void )
{
	std::tstring sourceText = GetSourceText();
	m_tokenCount = 0;

	if ( !m_separators[ m_action ].m_input.empty() )
	{
		std::tstring inputSeparator = code::ParseEscapeSeqs( m_separators[ m_action ].m_input.c_str() );
		std::tstring outputSeparator = code::ParseEscapeSeqs( m_separators[ m_action ].m_output.c_str() );
		str::ToWindowsLineEnds( inputSeparator );
		str::ToWindowsLineEnds( outputSeparator );

		std::vector< std::tstring > parts;

		switch ( m_action )
		{
			default:
				ASSERT( false );
			case SplitAction:
			{
				str::Split( parts, sourceText.c_str(), inputSeparator.c_str() );
				m_tokenCount = parts.size();

				CLineSet lineSet( str::Join( parts, outputSeparator.c_str() ) );
				return FormatOutputText( lineSet );
			}
			case TokenizeAction:
			{
				str::Tokenize( parts, sourceText.c_str(), inputSeparator.c_str() );
				m_tokenCount = parts.size();

				CLineSet lineSet( str::Join( parts, outputSeparator.c_str() ) );
				return FormatOutputText( lineSet );
			}
			case MergeAction:
			{
				str::Split( parts, sourceText.c_str(), inputSeparator.c_str() );
				m_tokenCount = parts.size();

				// prevent last empty part from translating into an extra line-end (output separator)
				std::tstring lastLineEnd;
				if ( !parts.empty() && parts.back().empty() )
				{
					lastLineEnd = _T("\r\n");
					parts.pop_back();
					--m_tokenCount;
				}

				CLineSet lineSet( str::Join( parts, outputSeparator.c_str() ) + lastLineEnd );
				return FormatOutputText( lineSet );
			}
		}
	}
	else
	{
		CLineSet lineSet( sourceText );
		switch ( m_action )
		{
			case ReverseAction:
				lineSet.Reverse();
				return FormatOutputText( lineSet );
			case SortAscAction:
				lineSet.SortAscending();
				return FormatOutputText( lineSet );
			case SortDescAction:
				lineSet.SortDescending();
				return FormatOutputText( lineSet );
			case FilterUniqueAction:
				lineSet.RemoveDuplicateLines( IsDlgButtonChecked( IDC_CASE_SENSITIVE_CHECK ) ? str::Case : str::IgnoreCase );
				return FormatOutputText( lineSet );
		}
	}

	return sourceText;
}

std::tstring CTokenizeTextDialog::GetSourceText( void ) const
{
	if ( IsDlgButtonChecked( IDC_FILTER_SOURCE_LINES_CHECK ) )
	{
		CLineSet lineSet( m_sourceText );

		if ( IsDlgButtonChecked( IDC_TRIM_LINES_CHECK ) )
			lineSet.TrimLines( code::ParseEscapeSeqs( ui::GetWindowText( m_trimCharsEdit ) ).c_str() );

		if ( IsDlgButtonChecked( IDC_REMOVE_EMPTY_LINES_CHECK ) )
			lineSet.RemoveEmptyLines();

		return lineSet.FormatText();
	}

	return m_sourceText;
}

std::tstring CTokenizeTextDialog::FormatOutputText( CLineSet& rLineSet ) const
{
	if ( IsDlgButtonChecked( IDC_FILTER_OUTPUT_LINES_CHECK ) )
	{
		if ( IsDlgButtonChecked( IDC_TRIM_LINES_CHECK ) )
			rLineSet.TrimLines( code::ParseEscapeSeqs( ui::GetWindowText( m_trimCharsEdit ) ).c_str() );

		if ( IsDlgButtonChecked( IDC_REMOVE_EMPTY_LINES_CHECK ) )
			rLineSet.RemoveEmptyLines();
	}

	return rLineSet.FormatText();
}

void CTokenizeTextDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_sourceEdit.m_hWnd;

	DDX_Control( pDX, IDC_SOURCE_TEXT_EDIT, m_sourceEdit );
	DDX_Control( pDX, IDC_OUTPUT_TEXT_EDIT, m_outputEdit );
	DDX_Control( pDX, IDC_INPUT_TOKENS_EDIT, m_inputTokensEdit );
	DDX_Control( pDX, IDC_OUTPUT_SEPARATOR_EDIT, m_outputSepsEdit );
	DDX_Control( pDX, IDC_INPUT_TOKENS_DEFAULT, m_defaultInputTokensButton );
	DDX_Control( pDX, IDC_OUTPUT_SEPARATOR_DEFAULT, m_defaultOutputSepsButton );
	DDX_Control( pDX, IDC_TRIM_CHARS_EDIT, m_trimCharsEdit );
	DDX_Control( pDX, IDC_TRIM_CHARS_DEFAULT, m_defaultTrimCharsButton );
	DDX_Control( pDX, IDC_COPY_OUTPUT_TO_SOURCE, m_copyOutputToSourceButton );

	if ( firstInit )
	{
		CWinApp* pApp = AfxGetApp();
		CheckDlgButton( IDC_CASE_SENSITIVE_CHECK, pApp->GetProfileInt( reg::section_dialog, reg::entry_caseSensitive, FALSE ) );
		CheckDlgButton( IDC_REMOVE_EMPTY_LINES_CHECK, pApp->GetProfileInt( reg::section_dialog, reg::entry_removeEmptyLines, FALSE ) );
		CheckDlgButton( IDC_TRIM_LINES_CHECK, pApp->GetProfileInt( reg::section_dialog, reg::entry_trimLines, FALSE ) );
		ui::SetWindowText( m_trimCharsEdit, (LPCTSTR)pApp->GetProfileString( reg::section_dialog, reg::entry_trimChars, defaultTrimChars ) );
		CheckDlgButton( IDC_FILTER_SOURCE_LINES_CHECK, pApp->GetProfileInt( reg::section_dialog, reg::entry_filterSource, FALSE ) );
		CheckDlgButton( IDC_FILTER_OUTPUT_LINES_CHECK, pApp->GetProfileInt( reg::section_dialog, reg::entry_filterOutput, TRUE ) );

		bool visibleWhiteSpace = pApp->GetProfileInt( reg::section_dialog, reg::entry_showWhiteSpace, FALSE ) != FALSE;
		CheckDlgButton( IDC_SHOW_WHITESPACE_CHECK, visibleWhiteSpace );
		m_sourceEdit.SetVisibleWhiteSpace( visibleWhiteSpace );
		m_outputEdit.SetVisibleWhiteSpace( visibleWhiteSpace );
	}

	m_sourceEdit.DDX_Text( pDX, m_sourceText );
	DDX_Radio( pDX, IDC_SPLIT_ACTION_RADIO, (int&)m_action );
	ui::DDX_Text( pDX, IDC_INPUT_TOKENS_EDIT, m_separators[ m_action ].m_input );
	ui::DDX_Text( pDX, IDC_OUTPUT_SEPARATOR_EDIT, m_separators[ m_action ].m_output );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		SetDlgItemText( IDC_INPUT_TOKENS_LABEL, TokenizeAction == m_action ? _T("&Input Tokens:") : _T("&Input Separator:") );

	m_outputText = GenerateOutputText();
	m_outputEdit.SetText( m_outputText );

	ui::SetDlgItemText( this, IDC_SOURCE_TEXT_LABEL, str::Format( _T("So&urce: %d lines"), m_sourceEdit.GetLineCount() ) );
	ui::SetDlgItemText( this, IDC_OUTPUT_TEXT_LABEL, str::Format( _T("&Output: %d lines, %d components found"), m_outputEdit.GetLineCount(), m_tokenCount ) );
	m_inputTokensEdit.Invalidate();

	static const UINT sepIds[] = { IDC_INPUT_TOKENS_LABEL, IDC_INPUT_TOKENS_EDIT, IDC_INPUT_TOKENS_DEFAULT,
		IDC_OUTPUT_SEPARATOR_LABEL, IDC_OUTPUT_SEPARATOR_EDIT, IDC_OUTPUT_SEPARATOR_DEFAULT };
	ui::ShowControls( m_hWnd, sepIds, COUNT_OF( sepIds ), ActionUseSeps() );
	ui::ShowControl( m_hWnd, IDC_CASE_SENSITIVE_CHECK, FilterUniqueAction == m_action );
	ui::EnableWindow( m_defaultInputTokensButton, m_separators[ m_action ].m_input != m_defaultSeparators[ m_action ].m_input );
	ui::EnableWindow( m_defaultOutputSepsButton, m_separators[ m_action ].m_output != m_defaultSeparators[ m_action ].m_output );

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CTokenizeTextDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED( IDC_INPUT_TOKENS_DEFAULT, OnDefaultInputTokens )
	ON_BN_CLICKED( IDC_OUTPUT_SEPARATOR_DEFAULT, OnDefaultOutputSeparator )
	ON_BN_CLICKED( IDC_TRIM_CHARS_DEFAULT, OnDefaultTrimChars )
	ON_BN_CLICKED( IDC_FILTER_SOURCE_LINES_CHECK, OnFieldChanged )
	ON_BN_CLICKED( IDC_FILTER_OUTPUT_LINES_CHECK, OnFieldChanged )
	ON_BN_CLICKED( IDC_COPY_OUTPUT_TO_SOURCE, OnCopyOutputToSource )
	ON_COMMAND_RANGE( IDC_SPLIT_ACTION_RADIO, IDC_FILTER_UNIQUE_ACTION_RADIO, OnActionChanged )
	ON_COMMAND_RANGE( IDC_CASE_SENSITIVE_CHECK, IDC_TRIM_LINES_CHECK, OnActionOptionChanged )
	ON_EN_CHANGE( IDC_SOURCE_TEXT_EDIT, OnEnChange_SourceText )
	ON_EN_CHANGE( IDC_INPUT_TOKENS_EDIT, OnEnChange_InputTokens )
	ON_EN_CHANGE( IDC_OUTPUT_SEPARATOR_EDIT, OnEnChange_OutputTokens )
	ON_EN_CHANGE( IDC_TRIM_CHARS_EDIT, OnFieldChanged )
	ON_BN_CLICKED( IDC_SHOW_WHITESPACE_CHECK, OnToggle_ShowWhiteSpace )
END_MESSAGE_MAP()

BOOL CTokenizeTextDialog::OnInitDialog( void )
{
	CLayoutDialog::OnInitDialog();
	if ( m_sourceText.empty() )
	{
		GotoDlgCtrl( &m_sourceEdit );
		return FALSE;
	}
	return TRUE;
}

void CTokenizeTextDialog::OnDestroy( void )
{
	RegistrySave();
	CLayoutDialog::OnDestroy();
}

HBRUSH CTokenizeTextDialog::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColor )
{
	enum { ErrorBkColor = RGB( 255, 222, 206 ), ErrorTextColor = RGB( 0x80, 0, 0 ) };

	HBRUSH hBrushBk = CLayoutDialog::OnCtlColor( pDC, pWnd, ctlColor );

	if ( pWnd != NULL )
		switch ( pWnd->GetDlgCtrlID() )
		{
			case IDC_INPUT_TOKENS_EDIT:
				if ( m_tokenCount <= 1 )
				{
					static CBrush errorBkBrush( ErrorBkColor );
					pDC->SetBkColor( ErrorBkColor );
					hBrushBk = errorBkBrush;
				}
				break;
			case IDC_OUTPUT_TEXT_EDIT:
				if ( ActionUseSeps() && m_tokenCount <= 1 )
					pDC->SetTextColor( ErrorTextColor );
				break;
		}

	return hBrushBk;
}

void CTokenizeTextDialog::OnActionChanged( UINT radioId )
{
	if ( ActionUseSeps() )			// old action
	{
		m_separators[ m_action ].m_input = ui::GetWindowText( m_inputTokensEdit );
		m_separators[ m_action ].m_output = ui::GetWindowText( m_outputSepsEdit );
	}

	m_action = static_cast<Action>( radioId - IDC_SPLIT_ACTION_RADIO );
	OnFieldChanged();
}

void CTokenizeTextDialog::OnActionOptionChanged( UINT radioId )
{
	radioId;
	OnFieldChanged();
}

void CTokenizeTextDialog::OnDefaultInputTokens( void )
{
	m_separators[ m_action ].m_input = m_defaultSeparators[ m_action ].m_input;
	OnFieldChanged();
	GotoDlgCtrl( &m_inputTokensEdit );
}

void CTokenizeTextDialog::OnDefaultOutputSeparator( void )
{
	m_separators[ m_action ].m_output = m_defaultSeparators[ m_action ].m_output;
	OnFieldChanged();
	GotoDlgCtrl( &m_outputSepsEdit );
}

void CTokenizeTextDialog::OnDefaultTrimChars( void )
{
	ui::SetWindowText( m_trimCharsEdit, code::FormatEscapeSeq( defaultTrimChars ) );
	OnFieldChanged();
	GotoDlgCtrl( &m_trimCharsEdit );
}

void CTokenizeTextDialog::OnCopyOutputToSource( void )
{
	m_sourceText = m_outputEdit.GetText();
	m_sourceEdit.SetText( m_sourceText );
	GotoDlgCtrl( &m_sourceEdit );
	m_sourceEdit.SelectAll();

	OnFieldChanged();
}

void CTokenizeTextDialog::OnEnChange_SourceText( void )
{
	m_sourceText = m_sourceEdit.GetText();
	OnFieldChanged();
}

void CTokenizeTextDialog::OnEnChange_InputTokens( void )
{
	m_separators[ m_action ].m_input = ui::GetWindowText( m_inputTokensEdit );
	OnFieldChanged();
}

void CTokenizeTextDialog::OnEnChange_OutputTokens( void )
{
	m_separators[ m_action ].m_output = ui::GetWindowText( m_outputSepsEdit );
	OnFieldChanged();
}

void CTokenizeTextDialog::OnToggle_ShowWhiteSpace( void )
{
	bool visibleWhiteSpace = IsDlgButtonChecked( IDC_SHOW_WHITESPACE_CHECK ) != FALSE;
	m_sourceEdit.SetVisibleWhiteSpace( visibleWhiteSpace );
	m_outputEdit.SetVisibleWhiteSpace( visibleWhiteSpace );
}

void CTokenizeTextDialog::OnFieldChanged( void )
{
	UpdateData( DialogOutput );
}
