
#include "stdafx.h"
#include "ReplaceDialog.h"
#include "MainRenameDialog.h"
#include "FileWorkingSet.h"
#include "FileSetUi.h"
#include "PathFunctors.h"
#include "resource.h"
#include "utl/ImageStore.h"
#include "utl/LayoutEngine.h"
#include "utl/MenuUtilities.h"
#include "utl/TextEdit.h"
#include "utl/Utilities.h"
#include "utl/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static const TCHAR specialSep[] = _T("\n");

namespace reg
{
	const TCHAR section[] = _T("Replace Dialog");
	const TCHAR entry_findWhat[] = _T("Find What");
	const TCHAR entry_replaceWith[] = _T("Replace With");
	const TCHAR entry_matchCase[] = _T("Match Case");
	const TCHAR entry_findType[] = _T("Find Type");
}


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_FIND_WHAT_COMBO, StretchX },
		{ IDC_REPLACE_WITH_COMBO, StretchX },
		{ IDC_STRIP_BAR_1, OffsetX },
		{ IDC_STRIP_BAR_2, OffsetX },
		{ IDOK, OffsetX },
		{ IDCANCEL, OffsetX },
		{ IDC_CLEAR_FILES_BUTTON, Offset }
	};
}


CReplaceDialog::CReplaceDialog( CMainRenameDialog* pParent )
	: CLayoutDialog( IDD_REPLACE_DIALOG, pParent )
	, m_pParent( pParent )
	, m_findWhat( LoadFindWhat() )
	, m_replaceWith( LoadReplaceWith() )
	, m_matchCase( AfxGetApp()->GetProfileInt( reg::section, reg::entry_matchCase, TRUE ) != FALSE )
	, m_findType( static_cast< FindType >( AfxGetApp()->GetProfileInt( reg::section, reg::entry_findType, Find_Text ) ) )
	, m_findWhatCombo( ui::HistoryMaxSize, specialSep, m_matchCase ? str::Case : str::IgnoreCase )
	, m_replaceWithCombo( ui::HistoryMaxSize, specialSep, m_matchCase ? str::Case : str::IgnoreCase )
	, m_pFileSetUi( new CFileSetUi( m_pParent->GetWorkingSet() ) )
{
	ASSERT_PTR( m_pParent );

	m_initCentered = false;
	m_regSection = reg::section;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	GetLayoutEngine().MaxClientSize().cy = 0;
	LoadDlgIcon( ID_EDIT_REPLACE );

	m_findToolbar.GetStrip()
		.AddButton( ID_PICK_FILENAME )
		.AddButton( ID_COPY_FIND_TO_REPLACE );

	m_replaceToolbar.GetStrip()
		.AddButton( ID_PICK_DIR_PATH )
		.AddButton( ID_PICK_TEXT_TOOLS );
}

CReplaceDialog::~CReplaceDialog()
{
}

std::tstring CReplaceDialog::LoadFindWhat( void )
{
	return ui::LoadHistorySelItem( reg::section, reg::entry_findWhat, _T(""), specialSep );
}

std::tstring CReplaceDialog::LoadReplaceWith( void )
{
	return ui::LoadHistorySelItem( reg::section, reg::entry_replaceWith, _T(""), specialSep );
}

std::tstring CReplaceDialog::FormatTooltipMessage( void )
{
	std::tstring message;
	std::tstring findWhat = LoadFindWhat();
	if ( !findWhat.empty() )
		message = str::Format( IDS_REPLACE_FILES_TIP_FORMAT, findWhat.c_str(), LoadReplaceWith().c_str() );
	return message;
}

bool CReplaceDialog::Execute( void )
{
	if ( SkipDialog() )
		return ReplaceItems();
	return DoModal() != IDCANCEL;
}

bool CReplaceDialog::SkipDialog( void ) const
{
	if ( ui::IsKeyPressed( VK_CONTROL ) )
		if ( FindMatch() )
			return true;

	return false;
}

void CReplaceDialog::StoreFindWhatText( const std::tstring& text, const std::vector< std::tstring >* pDestFnames /*= NULL*/ )
{
	GotoDlgCtrl( &m_findWhatCombo );
	ui::SetComboEditText( m_findWhatCombo, text, m_matchCase ? str::Case : str::IgnoreCase );

	if ( pDestFnames != NULL )
	{
		std::tstring maxCommonPrefix = m_pFileSetUi->ExtractLongestCommonPrefix( *pDestFnames );
		if ( !maxCommonPrefix.empty() )
			m_findWhatCombo.SetEditSel( (int)maxCommonPrefix.length(), -1 );
	}

	OnChanged_FindWhat();
}

bool CReplaceDialog::ReplaceItems( bool commit /*= true*/ ) const
{
	if ( m_findWhat.empty() )
		return false;				// no pattern to search for

	CFileWorkingSet* pFileData = m_pParent->GetWorkingSet();
	if ( Find_Text == m_findType )
	{
		func::ReplaceText functor( m_findWhat, m_replaceWith, m_matchCase, commit );
		pFileData->ForEachDestination( functor );
		if ( 0 == functor.m_matchCount )
			return false;
	}
	else
	{
		ASSERT( Find_Characters == m_findType );

		func::ReplaceCharacters functor( m_findWhat, m_replaceWith, m_matchCase, commit );
		pFileData->ForEachDestination( functor );
		if ( 0 == functor.m_matchCount )
			return false;
	}

	if ( commit )
		m_pParent->PostMakeDest();
	return true;
}

void CReplaceDialog::DoDataExchange( CDataExchange* pDX )
{
	const bool firstInit = NULL == m_findWhatCombo.m_hWnd;

	DDX_Control( pDX, IDC_FIND_WHAT_COMBO, m_findWhatCombo );
	DDX_Control( pDX, IDC_REPLACE_WITH_COMBO, m_replaceWithCombo );
	m_findToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignLeft | V_AlignCenter );
	m_replaceToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_2, H_AlignLeft | V_AlignCenter );

	ui::DDX_Bool( pDX, IDC_MATCH_CASE_CHECK, m_matchCase );
	DDX_Radio( pDX, IDC_TEXT_RADIO, (int&)m_findType );

	m_findWhatCombo.SetCaseType( m_matchCase ? str::Case : str::IgnoreCase );
	m_replaceWithCombo.SetCaseType( m_matchCase ? str::Case : str::IgnoreCase );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			enum { MaxChars = _MAX_PATH };

			CTextEdit::SetFixedFont( &m_findWhatCombo );
			CTextEdit::SetFixedFont( &m_replaceWithCombo );
			m_findWhatCombo.LimitText( MaxChars );
			m_replaceWithCombo.LimitText( MaxChars );

			m_findWhatCombo.LoadHistory( reg::section, reg::entry_findWhat );
			m_replaceWithCombo.LoadHistory( reg::section, reg::entry_replaceWith );
		}
		OnChanged_FindWhat();
	}
	else
	{
		m_findWhat = m_findWhatCombo.GetCurrentText();
		m_replaceWith = m_replaceWithCombo.GetCurrentText();

		m_findWhatCombo.SaveHistory( reg::section, reg::entry_findWhat );
		m_replaceWithCombo.SaveHistory( reg::section, reg::entry_replaceWith );
		AfxGetApp()->WriteProfileInt( reg::section, reg::entry_matchCase, m_matchCase );
		AfxGetApp()->WriteProfileInt( reg::section, reg::entry_findType, m_findType );
	}
	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CReplaceDialog, CLayoutDialog )
	ON_CBN_EDITCHANGE( IDC_FIND_WHAT_COMBO, OnChanged_FindWhat )
	ON_CBN_SELCHANGE( IDC_FIND_WHAT_COMBO, OnChanged_FindWhat )
	ON_BN_CLICKED( IDC_MATCH_CASE_CHECK, OnBnClicked_MatchCase )
	ON_BN_CLICKED( IDC_CLEAR_FILES_BUTTON, OnBnClicked_ClearDestFiles )
	ON_COMMAND( ID_PICK_FILENAME, OnPickFilename )
	ON_COMMAND( ID_COPY_FIND_TO_REPLACE, OnCopyFindToReplace )
	ON_COMMAND( ID_PICK_DIR_PATH, OnPickDirPath )
	ON_COMMAND( ID_PICK_TEXT_TOOLS, OnPickTextTools )
	ON_COMMAND( ID_EDIT_CLEAR, OnClearReplace )
	ON_COMMAND_RANGE( IDC_PICK_FILENAME_BASE, IDC_PICK_FILENAME_MAX, OnFilenamePicked )
	ON_COMMAND_RANGE( IDC_PICK_DIR_PATH_BASE, IDC_PICK_DIR_PATH_MAX, OnDirPathPicked )
	ON_COMMAND_RANGE( ID_TEXT_TITLE_CASE, ID_TEXT_SPACE_TO_UNDERBAR, OnFormatTextToolPicked )
END_MESSAGE_MAP()

void CReplaceDialog::OnOK( void )
{
	if ( UpdateData( DialogSaveChanges ) )
		if ( ReplaceItems() )
		{
			UpdateData( DialogOutput );

			if ( !ui::IsKeyPressed( VK_CONTROL ) )			// keep dialog open if CTRL key is down
				EndDialog( IDOK );
		}
}

void CReplaceDialog::OnChanged_FindWhat( void )
{
	m_findWhat = m_findWhatCombo.GetCurrentText();
	bool foundMatch = FindMatch();
	m_findWhatCombo.SetFrameColor( foundMatch ? CLR_NONE : color::Error );
	ui::EnableControl( m_hWnd, IDOK, foundMatch );
}

void CReplaceDialog::OnCopyFindToReplace( void )
{
	m_replaceWith = m_findWhat;
	GotoDlgCtrl( &m_replaceWithCombo );
	ui::SetComboEditText( m_replaceWithCombo, m_replaceWith, m_matchCase ? str::Case : str::IgnoreCase );
}

void CReplaceDialog::OnBnClicked_MatchCase( void )
{
	m_matchCase = IsDlgButtonChecked( IDC_MATCH_CASE_CHECK ) != FALSE;
	OnChanged_FindWhat();
}

void CReplaceDialog::OnBnClicked_ClearDestFiles( void )
{
	ui::SendCommand( GetParent()->GetSafeHwnd(), IDC_CLEAR_FILES_BUTTON );
	OnChanged_FindWhat();			// update combo frame
}

void CReplaceDialog::OnPickFilename( void )
{
	std::tstring singlePattern;
	CMenu popupMenu;
	m_pFileSetUi->MakePickFnamePatternMenu( &singlePattern, &popupMenu, m_findWhat.c_str() );		// single pattern or pick menu

	if ( !singlePattern.empty() )
		StoreFindWhatText( singlePattern );
	else
	{
		ASSERT_PTR( popupMenu.GetSafeHmenu() );
		m_findToolbar.TrackButtonMenu( ID_PICK_FILENAME, this, &popupMenu, ui::DropRight );
	}
}

void CReplaceDialog::OnFilenamePicked( UINT cmdId )
{
	std::vector< std::tstring > destFnames;
	std::tstring selFname = m_pFileSetUi->GetPickedFname( cmdId, &destFnames );
	StoreFindWhatText( selFname, &destFnames );
}

void CReplaceDialog::OnPickDirPath( void )
{
	// single command or pick menu
	UINT singleCmdId;
	CMenu popupMenu;

	if ( m_pFileSetUi->MakePickDirPathMenu( &singleCmdId, &popupMenu ) )
		if ( singleCmdId != 0 )
			OnDirPathPicked( singleCmdId );
		else
		{
			ASSERT_PTR( popupMenu.GetSafeHmenu() );
			m_replaceToolbar.TrackButtonMenu( ID_PICK_DIR_PATH, this, &popupMenu, ui::DropRight );
		}
}

void CReplaceDialog::OnDirPathPicked( UINT cmdId )
{
	std::tstring selDir = m_pFileSetUi->GetPickedDirectory( cmdId );
	if ( CEdit* pComboEdit = (CEdit*)m_replaceWithCombo.GetWindow( GW_CHILD ) )
	{
		pComboEdit->SetFocus();
		pComboEdit->ReplaceSel( selDir.c_str(), TRUE );
	}
}

void CReplaceDialog::OnPickTextTools( void )
{
	CMenu popupMenu;
	ui::LoadPopupMenu( popupMenu, IDR_CONTEXT_MENU_MENU, popup::TextTools );

	m_replaceToolbar.TrackButtonMenu( ID_PICK_TEXT_TOOLS, this, &popupMenu, ui::DropDown );
}

void CReplaceDialog::OnFormatTextToolPicked( UINT cmdId )
{
	// avoid since it selects all text
	//GotoDlgCtrl( &m_replaceWithCombo );

	CEdit* pComboEdit = (CEdit*)m_replaceWithCombo.GetWindow( GW_CHILD );
	ASSERT_PTR( pComboEdit );
	pComboEdit->SetFocus();

	std::tstring text = m_replaceWithCombo.GetCurrentText();

	DWORD sel = m_replaceWithCombo.GetEditSel();
	int startPos = LOWORD( sel ), endPos = HIWORD( sel );
	ASSERT( endPos != -1 );
	if ( startPos != endPos )						// has selected text
		text = text.substr( startPos, endPos - startPos );

	std::tstring newText = CFileSetUi::ApplyTextTool( cmdId, text );

	if ( newText != text )
		ui::ReplaceComboEditText( m_replaceWithCombo, newText, str::Case );
}

void CReplaceDialog::OnClearReplace( void )
{
	GotoDlgCtrl( &m_replaceWithCombo );
	ui::SetComboEditText( m_replaceWithCombo, std::tstring(), m_matchCase ? str::Case : str::IgnoreCase );
}
