
#include "stdafx.h"
#include "ReplaceDialog.h"
#include "IFileEditor.h"
#include "RenameService.h"
#include "FileModel.h"
#include "EditingCommands.h"
#include "RenameItem.h"
#include "TextAlgorithms.h"
#include "Application_fwd.h"
#include "resource.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/LayoutEngine.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/TextEditor.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static const TCHAR specialSep[] = _T("\n");

namespace reg
{
	const TCHAR section[] = _T("ReplaceDialog");
	const TCHAR entry_findWhat[] = _T("Find What");
	const TCHAR entry_replaceWith[] = _T("Replace With");
	const TCHAR entry_matchCase[] = _T("Match Case");
	const TCHAR entry_findType[] = _T("Find Type");
	const TCHAR entry_autoFillCommonPrefix[] = _T("AutoFillCommonPrefix");
}


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_FIND_WHAT_COMBO, SizeX },
		{ IDC_REPLACE_WITH_COMBO, SizeX },
		{ IDC_STRIP_BAR_1, MoveX },
		{ IDC_STRIP_BAR_2, MoveX },
		{ IDOK, MoveX },
		{ IDCANCEL, MoveX },
		{ IDC_RESET_FILES_BUTTON, Move }
	};
}


CReplaceDialog::CReplaceDialog( IFileEditor* pParentEditor, const CRenameService* pRenSvc )
	: CLayoutDialog( IDD_REPLACE_DIALOG, pParentEditor->GetDialog() )
	, m_pParentEditor( pParentEditor )
	, m_pRenSvc( pRenSvc )
	, m_findWhat( LoadFindWhat() )
	, m_replaceWith( LoadReplaceWith() )
	, m_matchCase( AfxGetApp()->GetProfileInt( reg::section, reg::entry_matchCase, true ) != FALSE )
	, m_findType( static_cast< FindType >( AfxGetApp()->GetProfileInt( reg::section, reg::entry_findType, Find_Text ) ) )
	, m_autoFillCommonPrefix( AfxGetApp()->GetProfileInt( reg::section, reg::entry_autoFillCommonPrefix, true ) != FALSE )
	, m_findWhatCombo( ui::HistoryMaxSize, specialSep, m_matchCase ? str::Case : str::IgnoreCase )
	, m_replaceWithCombo( ui::HistoryMaxSize, specialSep, m_matchCase ? str::Case : str::IgnoreCase )
{
	ASSERT_PTR( m_pParentEditor );

	m_initCentered = false;
	m_regSection = reg::section;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	GetLayoutEngine().MaxClientSize().cy = 0;		// no vertical resize
	LoadDlgIcon( ID_EDIT_REPLACE );

	ClearFlag( m_findWhatCombo.RefItemContent().m_itemsFlags, ui::CItemContent::Trim );
	ClearFlag( m_replaceWithCombo.RefItemContent().m_itemsFlags, ui::CItemContent::RemoveEmpty | ui::CItemContent::Trim );

	m_findToolbar.GetStrip()
		.AddButton( ID_PICK_FILENAME )
		.AddButton( ID_COPY_FIND_TO_REPLACE )
		.AddSeparator()
		.AddButton( ID_AUTO_FILL_COMMON_PREFIX_CK );

	m_replaceToolbar.GetStrip()
		.AddButton( ID_PICK_TEXT_TOOLS )
		.AddButton( ID_PICK_DIR_PATH );
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

void CReplaceDialog::StoreFindWhatText( const std::tstring& text, const std::tstring& commonPrefix )
{
	GotoDlgCtrl( &m_findWhatCombo );
	ui::SetComboEditText( m_findWhatCombo, text, m_matchCase ? str::Case : str::IgnoreCase );

	if ( !commonPrefix.empty() )
		m_findWhatCombo.SetEditSel( static_cast< int >( commonPrefix.length() ), -1 );

	OnChanged_FindWhat();
}

void CReplaceDialog::StoreReplaceWithText( const std::tstring& text )
{
	if ( CEdit* pComboEdit = (CEdit*)m_replaceWithCombo.GetWindow( GW_CHILD ) )
	{
		pComboEdit->SetFocus();
		pComboEdit->ReplaceSel( text.c_str(), TRUE );
	}
}

bool CReplaceDialog::ReplaceItems( bool commit /*= true*/ ) const
{
	if ( m_findWhat.empty() )
		return false;				// no pattern to search for

	CFileModel* pFileModel = m_pParentEditor->GetFileModel();
	std::auto_ptr< utl::ICommand > pReplaceCmd;

	if ( Find_Text == m_findType )
	{
		func::ReplaceText functor( m_findWhat, m_replaceWith, m_matchCase, commit );
		std::tstring cmdTag = str::Format( _T("Replace \"%s\" with \"%s\""), m_findWhat.c_str(), m_replaceWith.c_str() );
		pReplaceCmd.reset( pFileModel->MakeChangeDestPathsCmd( functor, cmdTag ) );
		if ( 0 == functor.m_matchCount )
			return false;
	}
	else
	{
		ASSERT( Find_Characters == m_findType );

		func::ReplaceCharacters functor( m_findWhat, m_replaceWith, m_matchCase, commit );
		std::tstring cmdTag = str::Format( _T("Replace characters '%s' with \"%s\""), m_findWhat.c_str(), m_replaceWith.c_str() );
		pReplaceCmd.reset( pFileModel->MakeChangeDestPathsCmd( functor, cmdTag ) );
		if ( 0 == functor.m_matchCount )
			return false;
	}

	return
		!commit ||
		pFileModel->SafeExecuteCmd( m_pParentEditor, pReplaceCmd.release() );
}

bool CReplaceDialog::FillCommonPrefix( void )
{
	std::auto_ptr< CPickDataset > pPickDataset = m_pRenSvc->MakeFnamePickDataset();

	if ( !pPickDataset->HasCommonPrefix() )
	{
		ui::FlashCtrlFrame( &m_findWhatCombo, color::Error );
		return false;
	}

	StoreFindWhatText( pPickDataset->GetCommonPrefix(), pPickDataset->GetCommonPrefix() );
	ui::FlashCtrlFrame( &m_findWhatCombo, color::AzureBlue );
	return true;
}

void CReplaceDialog::DoDataExchange( CDataExchange* pDX )
{
	const bool firstInit = NULL == m_findWhatCombo.m_hWnd;

	DDX_Control( pDX, IDC_FIND_WHAT_COMBO, m_findWhatCombo );
	DDX_Control( pDX, IDC_REPLACE_WITH_COMBO, m_replaceWithCombo );
	m_findToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignLeft | V_AlignCenter );
	m_replaceToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_2, H_AlignLeft | V_AlignCenter );
	ui::DDX_ButtonIcon( pDX, IDC_RESET_FILES_BUTTON, ID_RESET_DEFAULT );

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

			if ( m_autoFillCommonPrefix )
				FillCommonPrefix();
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
	ON_WM_DESTROY()
	ON_CBN_EDITCHANGE( IDC_FIND_WHAT_COMBO, OnChanged_FindWhat )
	ON_CBN_SELCHANGE( IDC_FIND_WHAT_COMBO, OnChanged_FindWhat )
	ON_BN_CLICKED( IDC_MATCH_CASE_CHECK, OnBnClicked_MatchCase )
	ON_BN_CLICKED( IDC_RESET_FILES_BUTTON, OnBnClicked_ResetDestFiles )
	ON_COMMAND( ID_AUTO_FILL_COMMON_PREFIX_CK, OnToggle_AutoFillCommonPrefix )
	ON_UPDATE_COMMAND_UI( ID_AUTO_FILL_COMMON_PREFIX_CK, OnUpdate_AutoFillCommonPrefix )
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

void CReplaceDialog::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section, reg::entry_autoFillCommonPrefix, m_autoFillCommonPrefix );

	__super::OnDestroy();
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

void CReplaceDialog::OnBnClicked_ResetDestFiles( void )
{
	ui::SendCommand( GetParent()->GetSafeHwnd(), IDC_RESET_FILES_BUTTON );

	if ( m_autoFillCommonPrefix )
		FillCommonPrefix();
	else
		OnChanged_FindWhat();			// update combo frame
}

void CReplaceDialog::OnToggle_AutoFillCommonPrefix( void )
{
	m_autoFillCommonPrefix = !m_autoFillCommonPrefix;

	if ( m_autoFillCommonPrefix )
		FillCommonPrefix();
}

void CReplaceDialog::OnUpdate_AutoFillCommonPrefix( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_autoFillCommonPrefix );
}

void CReplaceDialog::OnPickFilename( void )
{
	m_pPickDataset = m_pRenSvc->MakeFnamePickDataset();

	if ( m_pPickDataset->HasCommonPrefix() && ui::IsKeyPressed( VK_CONTROL ) )
		StoreFindWhatText( m_pPickDataset->GetCommonPrefix(), m_pPickDataset->GetCommonPrefix() );		// write the common prefix directly as find pattern
	else
	{
		CMenu popupMenu;
		m_pPickDataset->MakePickFnameMenu( &popupMenu, m_findWhat.c_str() );
		m_findToolbar.TrackButtonMenu( ID_PICK_FILENAME, this, &popupMenu, ui::DropRight );
	}
}

void CReplaceDialog::OnFilenamePicked( UINT cmdId )
{
	ASSERT_PTR( m_pPickDataset.get() );

	StoreFindWhatText( m_pPickDataset->GetPickedFname( cmdId ), m_pPickDataset->GetCommonPrefix() );
}

void CReplaceDialog::OnPickDirPath( void )
{
	m_pPickDataset = m_pRenSvc->MakeDirPickDataset();

	if ( m_pPickDataset->HasSubDirs() )
		if ( ui::IsKeyPressed( VK_CONTROL ) )
			StoreReplaceWithText( m_pPickDataset->GetSubDirs().back() );		// pick the parent directory
		else
		{
			CMenu popupMenu;
			m_pPickDataset->MakePickDirMenu( &popupMenu );
			m_replaceToolbar.TrackButtonMenu( ID_PICK_DIR_PATH, this, &popupMenu, ui::DropRight );
		}
}

void CReplaceDialog::OnDirPathPicked( UINT cmdId )
{
	ASSERT_PTR( m_pPickDataset.get() );
	StoreReplaceWithText( m_pPickDataset->GetPickedDirectory( cmdId ) );
	m_pPickDataset.reset();
}

void CReplaceDialog::OnPickTextTools( void )
{
	CMenu popupMenu;
	ui::LoadPopupMenu( popupMenu, IDR_CONTEXT_MENU, popup::TextTools );

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

	std::tstring newText = CRenameService::ApplyTextTool( cmdId, text );

	if ( newText != text )
		ui::ReplaceComboEditText( m_replaceWithCombo, newText, str::Case );
}

void CReplaceDialog::OnClearReplace( void )
{
	GotoDlgCtrl( &m_replaceWithCombo );
	ui::SetComboEditText( m_replaceWithCombo, std::tstring(), m_matchCase ? str::Case : str::IgnoreCase );
}
