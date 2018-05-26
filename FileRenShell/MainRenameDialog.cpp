
#include "stdafx.h"
#include "MainRenameDialog.h"
#include "Application.h"
#include "FileWorkingSet.h"
#include "FileSetUi.h"
#include "PathAlgorithms.h"
#include "ReplaceDialog.h"
#include "CapitalizeOptionsDialog.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/CmdInfoStore.h"
#include "utl/FmtUtils.h"
#include "utl/LongestCommonSubsequence.h"
#include "utl/MenuUtilities.h"
#include "utl/PathGenerator.h"
#include "utl/RuntimeException.h"
#include "utl/UtilitiesEx.h"
#include "utl/VisualTheme.h"
#include "utl/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static bool s_dbgGuides = false;
static bool s_useDefaultDraw = false;


namespace reg
{
	static const TCHAR section_mainDialog[] = _T("Main Dialog");
	static const TCHAR entry_formatHistory[] = _T("Format History");
	static const TCHAR entry_autoGenerate[] = _T("Auto Generate");
	static const TCHAR entry_seqCount[] = _T("Sequence Count");
	static const TCHAR entry_seqCountAutoAdvance[] = _T("Sequence Count Auto Advance");
	static const TCHAR entry_changeCase[] = _T("Change Case");
	static const TCHAR entry_delimiterSetHistory[] = _T("Delimiter Set History");
	static const TCHAR entry_newDelimiterHistory[] = _T("New Delimiter History");
}

static const TCHAR defaultDelimiterSet[] = _T(".;-_ \t");
static const TCHAR defaultNewDelimiter[] = _T(" ");
static const TCHAR specialSep[] = _T("\n");


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_FORMAT_COMBO, SizeX },
		{ IDC_STRIP_BAR_1, MoveX },

		{ IDC_FILE_RENAME_LIST, Size },

		{ IDC_SOURCE_FILES_GROUP, MoveY },
		{ IDC_COPY_SOURCE_PATHS_BUTTON, MoveY },

		{ IDC_DESTINATION_FILES_GROUP, Move },
		{ IDC_PASTE_FILES_BUTTON, Move },
		{ IDC_CLEAR_FILES_BUTTON, Move },
		{ IDC_CAPITALIZE_BUTTON, Move },
		{ IDC_CHANGE_CASE_BUTTON, Move },

		{ IDC_REPLACE_FILES_BUTTON, Move },
		{ IDC_REPLACE_DELIMS_BUTTON, Move },
		{ IDC_DELIMITER_SET_COMBO, Move },
		{ IDC_DELIMITER_STATIC, Move },
		{ IDC_NEW_DELIMITER_EDIT, Move },
		{ IDC_PICK_RENAME_ACTIONS, Move },

		{ IDOK, MoveX },
		{ IDC_UNDO_BUTTON, MoveX },
		{ IDCANCEL, MoveX }
	};
}


CMainRenameDialog::CMainRenameDialog( app::MenuCommand menuCmd, CFileWorkingSet* pFileData, CWnd* pParent )
	: CBaseMainDialog( IDD_RENAME_FILES_DIALOG, pParent )
	, m_pFileData( pFileData )
	, m_menuCmd( menuCmd )
	, m_autoGenerate( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_autoGenerate, false ) != FALSE )
	, m_seqCountAutoAdvance( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_seqCountAutoAdvance, true ) != FALSE )
	, m_mode( Uninit )
	, m_formatCombo( ui::HistoryMaxSize, specialSep )
	, m_fileListCtrl( IDC_FILE_RENAME_LIST, LVS_EX_GRIDLINES | CReportListControl::DefaultStyleEx )
	, m_changeCaseButton( &GetTags_ChangeCase() )
	, m_delimiterSetCombo( ui::HistoryMaxSize, specialSep )
	, m_delimStatic( CThemeItem( L"EXPLORERBAR", vt::EBP_IEBARMENU, vt::EBM_NORMAL ) )
	, m_pickRenameActionsStatic( ui::DropDown )
{
	ASSERT_PTR( m_pFileData );
	m_regSection = reg::section_mainDialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );

	m_fileListCtrl.SetUseAlternateRowColoring();
	m_fileListCtrl.SetTextEffectCallback( this );

	m_changeCaseButton.SetSelValue( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_changeCase, ExtLowerCase ) );

	m_formatToolbar.GetStrip()
		.AddButton( ID_PICK_FORMAT_TOKEN )
		.AddButton( ID_PICK_DIR_PATH )
		.AddButton( ID_PICK_TEXT_TOOLS )
		.AddButton( ID_TOGGLE_AUTO_GENERATE );

	m_seqCountToolbar.GetStrip()
		.AddButton( ID_SEQ_COUNT_RESET )
		.AddButton( ID_SEQ_COUNT_FIND_NEXT )
		.AddButton( ID_SEQ_COUNT_AUTO_ADVANCE );
}

CMainRenameDialog::~CMainRenameDialog()
{
	utl::ClearOwningContainer( m_displayItems );
}

void CMainRenameDialog::InitDisplayItems( void )
{
	utl::ClearOwningContainer( m_displayItems );

	const fs::TPathPairMap& rRenamePairs = m_pFileData->GetRenamePairs();

	m_displayItems.reserve( rRenamePairs.size() );
	for ( fs::TPathPairMap::const_iterator itPair = rRenamePairs.begin(); itPair != rRenamePairs.end(); ++itPair )
		m_displayItems.push_back( new CDisplayItem( &*itPair ) );
}

void CMainRenameDialog::SetupFileListView( void )
{
	int orgSel = m_fileListCtrl.GetCurSel();

	{
		CScopedLockRedraw freeze( &m_fileListCtrl );
		CScopedInternalChange internalChange( &m_fileListCtrl );

		m_fileListCtrl.DeleteAllItems();
		InitDisplayItems();

		for ( unsigned int pos = 0; pos != m_displayItems.size(); ++pos )
		{
			CDisplayItem* pItem = m_displayItems[ pos ];

			m_fileListCtrl.InsertObjectItem( pos, pItem );		// Source
			m_fileListCtrl.SetItemText( pos, Destination, pItem->GetDestPath().GetNameExt() );
		}

		m_fileListCtrl.SetupDiffColumnPair( Source, Destination, path::GetMatch() );
	}

	// restore the selection (if any)
	if ( orgSel != -1 )
	{
		m_fileListCtrl.EnsureVisible( orgSel, FALSE );
		m_fileListCtrl.SetCurSel( orgSel );
	}
}

int CMainRenameDialog::FindItemPos( const fs::CPath& sourcePath ) const
{
	for ( unsigned int pos = 0; pos != m_displayItems.size(); ++pos )
		if ( m_displayItems[ pos ]->GetSrcPath().Equal( sourcePath ) )
			return pos;

	return -1;
}

void CMainRenameDialog::SwitchMode( Mode mode )
{
	m_mode = mode;

	static const std::vector< std::tstring > labels = str::LoadStrings( IDS_OK_BUTTON_LABELS );	// &Make Names|Rena&me|R&ollback
	ui::SetWindowText( ::GetDlgItem( m_hWnd, IDOK ), labels[ m_mode ] );

	static const UINT ctrlIds[] =
	{
		IDC_FORMAT_COMBO, IDC_STRIP_BAR_1, IDC_STRIP_BAR_2, IDC_COPY_SOURCE_PATHS_BUTTON,
		IDC_PASTE_FILES_BUTTON, IDC_CLEAR_FILES_BUTTON, IDC_CAPITALIZE_BUTTON, IDC_CHANGE_CASE_BUTTON,
		IDC_REPLACE_FILES_BUTTON, IDC_REPLACE_DELIMS_BUTTON, IDC_DELIMITER_SET_COMBO, IDC_NEW_DELIMITER_EDIT
	};
	ui::EnableControls( *this, ctrlIds, COUNT_OF( ctrlIds ), m_mode != UndoRollbackMode );
	ui::EnableControl( *this, IDC_UNDO_BUTTON, m_mode != UndoRollbackMode && m_pFileData->CanUndo( app::RenameFiles ) );
}

void CMainRenameDialog::PostMakeDest( bool silent /*= false*/ )
{
	m_pBatchTransaction.reset();

	if ( !silent )
		GotoDlgCtrl( GetDlgItem( IDOK ) );

	SetupFileListView();							// fill in and select the found files list

	try
	{
		m_pFileData->CheckPathCollisions();			// throws if duplicates are found
		SwitchMode( RenameMode );
	}
	catch ( CRuntimeException& e )
	{
		if ( m_pFileData->HasErrors() )
		{
			m_fileListCtrl.EnsureVisible( (int)*m_pFileData->GetErrorIndexes().begin(), FALSE );
			m_fileListCtrl.Invalidate();
		}
		SwitchMode( MakeMode );

		if ( !silent )
			e.ReportError();
	}
}

CWnd* CMainRenameDialog::GetWnd( void )
{
	return this;
}

CLogger* CMainRenameDialog::GetLogger( void )
{
	return &app::GetLogger();
}

fs::UserFeedback CMainRenameDialog::HandleFileError( const fs::CPath& sourcePath, const std::tstring& message )
{
	int pos = FindItemPos( sourcePath );
	if ( pos != -1 )
		m_fileListCtrl.EnsureVisible( pos, FALSE );
	m_fileListCtrl.Invalidate();

	switch ( AfxMessageBox( message.c_str(), MB_ICONWARNING | MB_ABORTRETRYIGNORE ) )
	{
		default: ASSERT( false );
		case IDIGNORE:	return fs::Ignore;
		case IDRETRY:	return fs::Retry;
		case IDABORT:	return fs::Abort;
	}
}

void CMainRenameDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const
{
	subItem;

	static const ui::CTextEffect s_errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );

	const CDisplayItem* pItem = CReportListControl::AsPtr< CDisplayItem >( rowKey );

	if ( m_pBatchTransaction.get() != NULL && m_pBatchTransaction->ContainsError( pItem->GetSrcPath() ) )
		rTextEffect.Combine( s_errorBk );
}

void CMainRenameDialog::AutoGenerateFiles( void )
{
	std::tstring renameFormat = m_formatCombo.GetCurrentText();
	UINT seqCount = m_seqCountEdit.GetNumericValue();
	bool succeeded = m_pFileData->GenerateDestPaths( renameFormat, &seqCount );

	if ( !succeeded )
		m_pFileData->ResetDestinations();

	PostMakeDest( true );							// silent mode, no modal messages
	m_formatCombo.SetFrameColor( succeeded ? CLR_NONE : color::Error );
}

bool CMainRenameDialog::RenameFiles( void )
{
	m_pBatchTransaction.reset( new fs::CBatchRename( m_pFileData->GetRenamePairs(), this ) );
	if ( !m_pBatchTransaction->RenameFiles() )
		return false;

	return true;
}

bool CMainRenameDialog::ChangeSeqCount( UINT seqCount )
{
	if ( m_seqCountEdit.GetNumber< UINT >() == seqCount )
		return false;

	m_seqCountEdit.SetNumericValue( seqCount );
	OnEnChange_SeqCounter();
	return true;
}

void CMainRenameDialog::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* /*pTooltip*/ ) const
{
	switch ( cmdId )
	{
		case IDC_CHANGE_CASE_BUTTON:
		{
			static const std::vector< std::tstring > tooltips = str::LoadStrings( IDC_CHANGE_CASE_BUTTON );
			ChangeCase changeCase = m_changeCaseButton.GetSelEnum< ChangeCase >();
			rText = tooltips.at( changeCase );
			break;
		}
		case IDC_CAPITALIZE_BUTTON:
			if ( m_capitalizeButton.GetRhsPartRect().PtInRect( ui::GetCursorPos( m_capitalizeButton ) ) )
			{
				static const std::tstring details = str::Load( IDI_DETAILS );
				rText = details;
			}
			break;
		case IDC_REPLACE_FILES_BUTTON:
			rText = CReplaceDialog::FormatTooltipMessage();
			break;
	}
}

void CMainRenameDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_FORMAT_COMBO, m_formatCombo );
	m_formatToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignLeft | V_AlignCenter );
	DDX_Control( pDX, IDC_SEQ_COUNT_EDIT, m_seqCountEdit );
	m_seqCountToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_2, H_AlignLeft | V_AlignCenter );
	DDX_Control( pDX, IDC_FILE_RENAME_LIST, m_fileListCtrl );
	DDX_Control( pDX, IDC_CAPITALIZE_BUTTON, m_capitalizeButton );
	DDX_Control( pDX, IDC_CHANGE_CASE_BUTTON, m_changeCaseButton );
	DDX_Control( pDX, IDC_DELIMITER_SET_COMBO, m_delimiterSetCombo );
	DDX_Control( pDX, IDC_NEW_DELIMITER_EDIT, m_newDelimiterEdit );
	DDX_Control( pDX, IDC_DELIMITER_STATIC, m_delimStatic );
	DDX_Control( pDX, IDC_PICK_RENAME_ACTIONS, m_pickRenameActionsStatic );
	ui::DDX_ButtonIcon( pDX, IDC_COPY_SOURCE_PATHS_BUTTON, ID_EDIT_COPY );
	ui::DDX_ButtonIcon( pDX, IDC_PASTE_FILES_BUTTON, ID_EDIT_PASTE );
	ui::DDX_ButtonIcon( pDX, IDC_CLEAR_FILES_BUTTON, ID_REMOVE_ALL_ITEMS );
	ui::DDX_ButtonIcon( pDX, IDC_REPLACE_FILES_BUTTON, ID_EDIT_REPLACE );
	ui::DDX_ButtonIcon( pDX, IDC_UNDO_BUTTON, ID_EDIT_UNDO );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( Uninit == m_mode )
		{
			m_formatCombo.LimitText( _MAX_PATH );
			CTextEdit::SetFixedFont( &m_delimiterSetCombo );
			m_delimiterSetCombo.LimitText( 64 );
			m_newDelimiterEdit.LimitText( 64 );

			m_formatCombo.LoadHistory( m_regSection.c_str(), reg::entry_formatHistory, _T("## - *.*") );
			m_delimiterSetCombo.LoadHistory( m_regSection.c_str(), reg::entry_delimiterSetHistory, defaultDelimiterSet );
			m_newDelimiterEdit.SetWindowText( AfxGetApp()->GetProfileString( m_regSection.c_str(), reg::entry_newDelimiterHistory, defaultNewDelimiter ) );

			m_seqCountEdit.SetNumericValue( AfxGetApp()->GetProfileInt( m_regSection.c_str(), reg::entry_seqCount, 1 ) );
			SetupFileListView();			// fill in and select the found files list
			SwitchMode( MakeMode );
		}
	}

	CBaseMainDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CMainRenameDialog, CBaseMainDialog )
	ON_WM_DESTROY()
	ON_CBN_EDITCHANGE( IDC_FORMAT_COMBO, OnChanged_Format )
	ON_CBN_SELCHANGE( IDC_FORMAT_COMBO, OnChanged_Format )
	ON_COMMAND( ID_PICK_FORMAT_TOKEN, OnPickFormatToken )
	ON_COMMAND( ID_PICK_DIR_PATH, OnPickDirPath )
	ON_COMMAND_RANGE( IDC_PICK_DIR_PATH_BASE, IDC_PICK_DIR_PATH_MAX, OnDirPathPicked )
	ON_COMMAND( ID_PICK_TEXT_TOOLS, OnPickTextTools )
	ON_COMMAND_RANGE( ID_TEXT_TITLE_CASE, ID_TEXT_SPACE_TO_UNDERBAR, OnFormatTextToolPicked )
	ON_COMMAND( ID_TOGGLE_AUTO_GENERATE, OnToggleAutoGenerate )
	ON_UPDATE_COMMAND_UI( ID_TOGGLE_AUTO_GENERATE, OnUpdateAutoGenerate )
	ON_COMMAND_RANGE( ID_NUMERIC_SEQUENCE_2DIGITS, ID_EXTENSION_SEPARATOR, OnNumericSequence )
	ON_BN_CLICKED( IDC_UNDO_BUTTON, OnBnClicked_Undo )
	ON_EN_CHANGE( IDC_SEQ_COUNT_EDIT, OnEnChange_SeqCounter )
	ON_COMMAND( ID_SEQ_COUNT_RESET, OnSeqCountReset )
	ON_UPDATE_COMMAND_UI( ID_SEQ_COUNT_RESET, OnUpdateSeqCountReset )
	ON_COMMAND( ID_SEQ_COUNT_FIND_NEXT, OnSeqCountFindNext )
	ON_UPDATE_COMMAND_UI( ID_SEQ_COUNT_FIND_NEXT, OnUpdateSeqCountFindNext )
	ON_COMMAND( ID_SEQ_COUNT_AUTO_ADVANCE, OnSeqCountAutoAdvance )
	ON_UPDATE_COMMAND_UI( ID_SEQ_COUNT_AUTO_ADVANCE, OnUpdateSeqCountAutoAdvance )
	ON_BN_CLICKED( IDC_COPY_SOURCE_PATHS_BUTTON, OnBnClicked_CopySourceFiles )
	ON_BN_CLICKED( IDC_PASTE_FILES_BUTTON, OnBnClicked_PasteDestFiles )
	ON_BN_CLICKED( IDC_CLEAR_FILES_BUTTON, OnBnClicked_ClearDestFiles )
	ON_BN_CLICKED( IDC_CAPITALIZE_BUTTON, OnBnClicked_CapitalizeDestFiles )
	ON_SBN_RIGHTCLICKED( IDC_CAPITALIZE_BUTTON, OnBnClicked_CapitalizeOptions )
	ON_BN_CLICKED( IDC_CHANGE_CASE_BUTTON, OnBnClicked_ChangeCase )
	ON_BN_CLICKED( IDC_REPLACE_FILES_BUTTON, OnBnClicked_ReplaceDestFiles )
	ON_BN_CLICKED( IDC_REPLACE_DELIMS_BUTTON, OnBnClicked_ReplaceAllDelimitersDestFiles )
	ON_CBN_EDITCHANGE( IDC_DELIMITER_SET_COMBO, OnFieldChanged )
	ON_CBN_SELCHANGE( IDC_DELIMITER_SET_COMBO, OnFieldChanged )
	ON_EN_CHANGE( IDC_NEW_DELIMITER_EDIT, OnFieldChanged )
	ON_BN_CLICKED( IDC_PICK_RENAME_ACTIONS, OnBnClicked_PickRenameActions )
	ON_BN_DOUBLECLICKED( IDC_PICK_RENAME_ACTIONS, OnBnClicked_PickRenameActions )
	ON_COMMAND( ID_SINGLE_WHITESPACE, OnSingleWhitespace )
	ON_COMMAND( ID_REMOVE_WHITESPACE, OnRemoveWhitespace )
	ON_COMMAND( ID_DASH_TO_SPACE, OnDashToSpace )
	ON_COMMAND( ID_SPACE_TO_DASH, OnSpaceToDash )
	ON_COMMAND( ID_UNDERBAR_TO_SPACE, OnUnderbarToSpace )
	ON_COMMAND( ID_SPACE_TO_UNDERBAR, OnSpaceToUnderbar )
	ON_COMMAND( ID_ENSURE_UNIFORM_NUM_PADDING, OnEnsureUniformNumPadding )
END_MESSAGE_MAP()

BOOL CMainRenameDialog::OnInitDialog( void )
{
	BOOL defaultFocus = CBaseMainDialog::OnInitDialog();
	if ( m_autoGenerate )
		AutoGenerateFiles();

	UINT cmdId = 0, flashId = 0;

	switch ( m_menuCmd )
	{
		case app::Cmd_RenameAndCopy:				cmdId = flashId = IDC_COPY_SOURCE_PATHS_BUTTON; break;
		case app::Cmd_RenameAndCapitalize:			cmdId = flashId = IDC_CAPITALIZE_BUTTON; break;
		case app::Cmd_RenameAndLowCaseExt:			cmdId = flashId = IDC_CHANGE_CASE_BUTTON; m_changeCaseButton.SetSelValue( ExtLowerCase ); break;
		case app::Cmd_RenameAndReplace:				cmdId = flashId = IDC_REPLACE_FILES_BUTTON; break;
		case app::Cmd_RenameAndReplaceDelims:		cmdId = flashId = IDC_REPLACE_DELIMS_BUTTON; break;
		case app::Cmd_RenameAndSingleWhitespace:	cmdId = ID_SINGLE_WHITESPACE; flashId = IDC_PICK_RENAME_ACTIONS; break;
		case app::Cmd_RenameAndRemoveWhitespace:	cmdId = ID_REMOVE_WHITESPACE; flashId = IDC_PICK_RENAME_ACTIONS; break;
		case app::Cmd_RenameAndDashToSpace:			cmdId = ID_DASH_TO_SPACE; flashId = IDC_PICK_RENAME_ACTIONS; break;
		case app::Cmd_RenameAndUnderbarToSpace:		cmdId = ID_UNDERBAR_TO_SPACE; flashId = IDC_PICK_RENAME_ACTIONS; break;
		case app::Cmd_RenameAndSpaceToUnderbar:		cmdId = ID_SPACE_TO_UNDERBAR; flashId = IDC_PICK_RENAME_ACTIONS; break;
		case app::Cmd_UndoRename:					cmdId = flashId = IDC_UNDO_BUTTON; break;
	}

	if ( cmdId != 0 )
	{
		PostMessage( WM_COMMAND, MAKEWPARAM( cmdId, BN_CLICKED ), 0 );
		if ( CWnd* pCtrl = GetDlgItem( cmdId ) )
			GotoDlgCtrl( pCtrl );
		defaultFocus = FALSE;
	}

	if ( flashId != 0 )
		if ( CWnd* pCtrl = GetDlgItem( flashId ) )
			ui::FlashCtrlFrame( pCtrl );

	return defaultFocus;
}

void CMainRenameDialog::OnOK( void )
{
	switch ( m_mode )
	{
		case MakeMode:
		{
			std::tstring renameFormat = m_formatCombo.GetCurrentText();
			UINT oldSeqCount = m_seqCountEdit.GetNumericValue(), newSeqCount = oldSeqCount;

			if ( m_pFileData->GenerateDestPaths( renameFormat, &newSeqCount ) )
			{
				if ( !m_seqCountAutoAdvance )
					newSeqCount = oldSeqCount;

				if ( newSeqCount != oldSeqCount )
					m_seqCountEdit.SetNumericValue( newSeqCount );

				PostMakeDest();
			}
			else
			{
				AfxMessageBox( str::Format( IDS_INVALID_FORMAT, renameFormat.c_str() ).c_str(), MB_ICONERROR | MB_OK );
				ui::TakeFocus( m_formatCombo );
			}
			break;
		}
		case RenameMode:
			if ( RenameFiles() )
			{
				m_pFileData->SaveUndoInfo( app::RenameFiles, m_pBatchTransaction->GetCommittedKeys() );

				m_formatCombo.SaveHistory( m_regSection.c_str(), reg::entry_formatHistory );
				m_delimiterSetCombo.SaveHistory( m_regSection.c_str(), reg::entry_delimiterSetHistory );
				AfxGetApp()->WriteProfileString( m_regSection.c_str(), reg::entry_newDelimiterHistory, ui::GetWindowText( &m_newDelimiterEdit ).c_str() );
				AfxGetApp()->WriteProfileInt( m_regSection.c_str(), reg::entry_seqCount, m_seqCountEdit.GetNumericValue() );

				CBaseMainDialog::OnOK();
			}
			else
			{
				m_pFileData->SaveUndoInfo( app::RenameFiles, m_pBatchTransaction->GetCommittedKeys(), false );		// keep the rename pairs to allow inspecting for errors, or Undo
				SwitchMode( MakeMode );
			}
			break;
		case UndoRollbackMode:
			if ( RenameFiles() )
			{
				m_pFileData->CommitUndoInfo( app::RenameFiles );
				CBaseMainDialog::OnOK();
			}
			else
				SwitchMode( MakeMode );
			break;
	}
}

void CMainRenameDialog::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_autoGenerate, m_autoGenerate );
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_seqCountAutoAdvance, m_seqCountAutoAdvance );
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_changeCase, m_changeCaseButton.GetSelValue() );

	CBaseMainDialog::OnDestroy();
}

void CMainRenameDialog::OnChanged_Format( void )
{
	OnFieldChanged();

	CPathFormatter formatter( m_formatCombo.GetCurrentText() );
	m_formatCombo.SetFrameColor( formatter.IsValidFormat() ? CLR_NONE : color::Error );

	if ( m_autoGenerate )
		AutoGenerateFiles();
}

void CMainRenameDialog::OnSeqCountReset( void )
{
	ChangeSeqCount( 1 );
	GotoDlgCtrl( &m_seqCountEdit );
}

void CMainRenameDialog::OnUpdateSeqCountReset( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_seqCountEdit.GetNumericValue() != 1 );
}

void CMainRenameDialog::OnSeqCountFindNext( void )
{
	UINT seqCount = m_pFileData->FindNextAvailSeqCount( m_formatCombo.GetCurrentText() );
	ChangeSeqCount( seqCount );
	GotoDlgCtrl( &m_seqCountEdit );
}

void CMainRenameDialog::OnUpdateSeqCountFindNext( CCmdUI* pCmdUI )
{
	CPathFormatter format( m_formatCombo.GetCurrentText() );
	pCmdUI->Enable( format.m_isNumericFormat );
}

void CMainRenameDialog::OnSeqCountAutoAdvance( void )
{
	m_seqCountAutoAdvance = !m_seqCountAutoAdvance;
}

void CMainRenameDialog::OnUpdateSeqCountAutoAdvance( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_seqCountAutoAdvance );
}

void CMainRenameDialog::OnBnClicked_CopySourceFiles( void )
{
	if ( !m_pFileData->CopyClipSourcePaths( fmt::FilenameExt, this ) )
		AfxMessageBox( _T("Couldn't copy source files to clipboard!"), MB_ICONERROR | MB_OK );
}

void CMainRenameDialog::OnBnClicked_ClearDestFiles( void )
{
	m_pFileData->ResetDestinations();

	SetupFileListView();	// fill in and select the found files list
	SwitchMode( MakeMode );
}

void CMainRenameDialog::OnBnClicked_PasteDestFiles( void )
{
	try
	{
		m_pFileData->PasteClipDestinationPaths( this );
		PostMakeDest();
	}
	catch ( CRuntimeException& e )
	{
		e.ReportError();
	}
}

void CMainRenameDialog::OnBnClicked_CapitalizeDestFiles( void )
{
	CTitleCapitalizer capitalizer;
	m_pFileData->ForEachRenameDestination( func::CapitalizeWords( &capitalizer ) );
	PostMakeDest();
}

void CMainRenameDialog::OnBnClicked_CapitalizeOptions( void )
{
	CCapitalizeOptionsDialog dialog( this );
	dialog.DoModal();
	GotoDlgCtrl( GetDlgItem( IDC_CAPITALIZE_BUTTON ) );
}

void CMainRenameDialog::OnBnClicked_ChangeCase( void )
{
	m_pFileData->ForEachRenameDestination( func::MakeCase( m_changeCaseButton.GetSelEnum< ChangeCase >() ) );
	PostMakeDest();
}

void CMainRenameDialog::OnBnClicked_ReplaceDestFiles( void )
{
	CReplaceDialog dlg( this );
	dlg.Execute();
}

void CMainRenameDialog::OnBnClicked_ReplaceAllDelimitersDestFiles( void )
{
	std::tstring delimiterSet = m_delimiterSetCombo.GetCurrentText();
	if ( delimiterSet.empty() )
	{
		AfxMessageBox( str::Format( IDS_NO_DELIMITER_SET ).c_str() );
		ui::TakeFocus( m_delimiterSetCombo );
		return;
	}

	m_pFileData->ForEachRenameDestination( func::ReplaceDelimiterSet( delimiterSet, ui::GetWindowText( &m_newDelimiterEdit ) ) );
	PostMakeDest();
}

void CMainRenameDialog::OnBnClicked_Undo( void )
{
	ASSERT( m_mode != UndoRollbackMode && m_pFileData->CanUndo( app::RenameFiles ) );

	GotoDlgCtrl( GetDlgItem( IDOK ) );
	m_pFileData->RetrieveUndoInfo( app::RenameFiles );
	SwitchMode( UndoRollbackMode );
	SetupFileListView();
}

void CMainRenameDialog::OnFieldChanged( void )
{
	if ( m_mode != Uninit )
		SwitchMode( MakeMode );
}

void CMainRenameDialog::OnPickFormatToken( void )
{
	CMenu popupMenu;
	ui::LoadPopupMenu( popupMenu, IDR_CONTEXT_MENU, popup::FormatPicker );

	m_formatToolbar.TrackButtonMenu( ID_PICK_FORMAT_TOKEN, this, &popupMenu, ui::DropDown );
}

void CMainRenameDialog::OnPickDirPath( void )
{
	// single command or pick menu
	UINT singleCmdId;
	CMenu popupMenu;
	CFileSetUi fileSetUi( m_pFileData );

	if ( fileSetUi.MakePickDirPathMenu( &singleCmdId, &popupMenu ) )
		if ( singleCmdId != 0 )
			OnDirPathPicked( singleCmdId );
		else
		{
			ASSERT_PTR( popupMenu.GetSafeHmenu() );
			m_formatToolbar.TrackButtonMenu( ID_PICK_DIR_PATH, this, &popupMenu, ui::DropRight );
		}
}

void CMainRenameDialog::OnDirPathPicked( UINT cmdId )
{
	CFileSetUi fileSetUi( m_pFileData );
	std::tstring selDir = fileSetUi.GetPickedDirectory( cmdId );
	if ( CEdit* pComboEdit = (CEdit*)m_formatCombo.GetWindow( GW_CHILD ) )
	{
		pComboEdit->SetFocus();
		pComboEdit->ReplaceSel( selDir.c_str(), TRUE );
	}
}

void CMainRenameDialog::OnPickTextTools( void )
{
	CMenu popupMenu;
	ui::LoadPopupMenu( popupMenu, IDR_CONTEXT_MENU, popup::TextTools );

	m_formatToolbar.TrackButtonMenu( ID_PICK_TEXT_TOOLS, this, &popupMenu, ui::DropDown );
}

void CMainRenameDialog::OnFormatTextToolPicked( UINT cmdId )
{
	GotoDlgCtrl( &m_formatCombo );
	std::tstring oldFormat = m_formatCombo.GetCurrentText();
	std::tstring format = CFileSetUi::ApplyTextTool( cmdId, oldFormat );

	if ( format != oldFormat )
		ui::SetComboEditText( m_formatCombo, format, str::Case );
}

void CMainRenameDialog::OnToggleAutoGenerate( void )
{
	m_autoGenerate = !m_autoGenerate;
	if ( m_autoGenerate )
		AutoGenerateFiles();
}

void CMainRenameDialog::OnUpdateAutoGenerate( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_mode != UndoRollbackMode );
	pCmdUI->SetCheck( m_autoGenerate );
}

void CMainRenameDialog::OnNumericSequence( UINT cmdId )
{
	const TCHAR* pInsertFmt = NULL;

	switch ( cmdId )
	{
		case ID_NUMERIC_SEQUENCE_2DIGITS:	pInsertFmt = _T("##"); break;
		case ID_NUMERIC_SEQUENCE_3DIGITS:	pInsertFmt = _T("###"); break;
		case ID_NUMERIC_SEQUENCE_4DIGITS:	pInsertFmt = _T("####"); break;
		case ID_NUMERIC_SEQUENCE_5DIGITS:	pInsertFmt = _T("#####"); break;
		case ID_NUMERIC_SEQUENCE:			pInsertFmt = _T("#"); break;
		case ID_WILDCARD:					pInsertFmt = _T("*"); break;
		case ID_EXTENSION_SEPARATOR:		pInsertFmt = _T("."); break;
		default: ASSERT( false ); return;
	}

	CEdit* pComboEdit = (CEdit*)m_formatCombo.GetWindow( GW_CHILD );
	pComboEdit->ReplaceSel( pInsertFmt, TRUE );
}

void CMainRenameDialog::OnEnChange_SeqCounter( void )
{
	if ( m_autoGenerate )
		AutoGenerateFiles();
	else if ( m_mode != Uninit )
		SwitchMode( MakeMode );
}

void CMainRenameDialog::OnBnClicked_PickRenameActions( void )
{
	m_pickRenameActionsStatic.TrackMenu( this, IDR_CONTEXT_MENU, popup::MoreRenameActions );
}

void CMainRenameDialog::OnSingleWhitespace( void )
{
	m_pFileData->ForEachRenameDestination( func::SingleWhitespace() );
	PostMakeDest();
}

void CMainRenameDialog::OnRemoveWhitespace( void )
{
	m_pFileData->ForEachRenameDestination( func::RemoveWhitespace() );
	PostMakeDest();
}

void CMainRenameDialog::OnDashToSpace( void )
{
	m_pFileData->ForEachRenameDestination( func::ReplaceDelimiterSet( _T("-"), _T(" ") ) );
	PostMakeDest();
}

void CMainRenameDialog::OnSpaceToDash( void )
{
	m_pFileData->ForEachRenameDestination( func::ReplaceDelimiterSet( _T(" "), _T("-") ) );
	PostMakeDest();
}

void CMainRenameDialog::OnUnderbarToSpace( void )
{
	m_pFileData->ForEachRenameDestination( func::ReplaceDelimiterSet( _T("_"), _T(" ") ) );
	PostMakeDest();
}

void CMainRenameDialog::OnSpaceToUnderbar( void )
{
	m_pFileData->ForEachRenameDestination( func::ReplaceDelimiterSet( _T(" "), _T("_") ) );
	PostMakeDest();
}

void CMainRenameDialog::OnEnsureUniformNumPadding( void )
{
	m_pFileData->EnsureUniformNumPadding();
	PostMakeDest();
}


// CMainRenameDialog::CDisplayItem implementation

std::tstring CMainRenameDialog::CDisplayItem::GetCode( void ) const
{
	return m_pPathPair->first.Get();
}

std::tstring CMainRenameDialog::CDisplayItem::GetDisplayCode( void ) const
{
	return m_pPathPair->first.GetNameExt();
}
