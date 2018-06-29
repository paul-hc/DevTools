
#include "stdafx.h"
#include "MainRenameDialog.h"
#include "FileModel.h"
#include "FileService.h"
#include "FileCommands.h"
#include "GeneralOptions.h"
#include "RenameService.h"
#include "RenameItem.h"
#include "PathAlgorithms.h"
#include "ReplaceDialog.h"
#include "OptionsSheet.h"
#include "Application.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/CmdInfoStore.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/LongestCommonSubsequence.h"
#include "utl/MenuUtilities.h"
#include "utl/PathGenerator.h"
#include "utl/RuntimeException.h"
#include "utl/Thumbnailer.h"
#include "utl/UtilitiesEx.h"
#include "utl/VisualTheme.h"
#include "utl/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_mainDialog[] = _T("RenameDialog");
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
		{ IDCANCEL, MoveX },
		{ IDC_TOOLBAR_PLACEHOLDER, MoveX }
	};
}


CMainRenameDialog::CMainRenameDialog( CFileModel* pFileModel, CWnd* pParent )
	: CFileEditorBaseDialog( pFileModel, cmd::RenameFile, IDD_RENAME_FILES_DIALOG, pParent )
	, m_rRenameItems( m_pFileModel->LazyInitRenameItems() )
	, m_mode( MakeMode )
	, m_autoGenerate( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_autoGenerate, false ) != FALSE )
	, m_seqCountAutoAdvance( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_seqCountAutoAdvance, true ) != FALSE )
	, m_formatCombo( ui::HistoryMaxSize, specialSep )
	, m_fileListCtrl( IDC_FILE_RENAME_LIST )
	, m_changeCaseButton( &GetTags_ChangeCase() )
	, m_delimiterSetCombo( ui::HistoryMaxSize, specialSep )
	, m_delimStatic( CThemeItem( L"EXPLORERBAR", vt::EBP_IEBARMENU, vt::EBM_NORMAL ) )
	, m_pickRenameActionsStatic( ui::DropDown )
{
	REQUIRE( !m_rRenameItems.empty() );

	m_regSection = reg::section_mainDialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );

	m_fileListCtrl.ModifyListStyleEx( LVS_EX_DOUBLEBUFFER, LVS_EX_GRIDLINES );		// remove double buffering for better drawing accuracy on thumb scaling
	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );
	m_fileListCtrl.SetUseAlternateRowColoring();
	m_fileListCtrl.SetTextEffectCallback( this );
	CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );

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
}

bool CMainRenameDialog::RenameFiles( void )
{
	m_errorItems.clear();

	CFileService svc;
	if ( CMacroCommand* pRenameMacroCmd = svc.MakeRenameCmds( m_rRenameItems ) )
	{
		cmd::CScopedErrorObserver observe( this );
		return m_pFileModel->GetCommandModel()->Execute( pRenameMacroCmd );
	}

	return false;
}

void CMainRenameDialog::SetupFileListView( void )
{
	CScopedListTextSelection sel( &m_fileListCtrl );

	CScopedLockRedraw freeze( &m_fileListCtrl );
	CScopedInternalChange internalChange( &m_fileListCtrl );

	m_fileListCtrl.DeleteAllItems();

	for ( unsigned int pos = 0; pos != m_rRenameItems.size(); ++pos )
	{
		CRenameItem* pItem = m_rRenameItems[ pos ];

		m_fileListCtrl.InsertObjectItem( pos, pItem );		// Source
		m_fileListCtrl.SetSubItemText( pos, Destination, pItem->GetDestPath().GetNameExt() );
	}

	m_fileListCtrl.SetupDiffColumnPair( Source, Destination, path::GetMatch() );
	m_fileListCtrl.InitialSortList();
}

void CMainRenameDialog::SwitchMode( Mode mode )
{
	m_mode = mode;
	if ( NULL == m_hWnd )
		return;

	static const CEnumTags modeTags( _T("&Make Names|Rena&me|Roll &Back|Roll &Forward") );
	ui::SetWindowText( ::GetDlgItem( m_hWnd, IDOK ), modeTags.FormatUi( m_mode ) );

	static const UINT ctrlIds[] =
	{
		IDC_FORMAT_COMBO, IDC_STRIP_BAR_1, IDC_STRIP_BAR_2, IDC_COPY_SOURCE_PATHS_BUTTON,
		IDC_PASTE_FILES_BUTTON, IDC_CLEAR_FILES_BUTTON, IDC_CAPITALIZE_BUTTON, IDC_CHANGE_CASE_BUTTON,
		IDC_REPLACE_FILES_BUTTON, IDC_REPLACE_DELIMS_BUTTON, IDC_DELIMITER_SET_COMBO, IDC_NEW_DELIMITER_EDIT
	};
	ui::EnableControls( *this, ctrlIds, COUNT_OF( ctrlIds ), m_mode != RollBackMode );
}

void CMainRenameDialog::PostMakeDest( bool silent /*= false*/ )
{
	if ( !silent )
		GotoDlgCtrl( GetDlgItem( IDOK ) );

// list is setup on OnUpdate()
//	SetupFileListView();								// fill in and select the found files list

	ASSERT_PTR( m_pRenSvc.get() );
	if ( m_pRenSvc->CheckPathCollisions( this ) )		// stores erros in m_errorItems
		SwitchMode( RenameMode );
	else
	{
		ASSERT( !m_errorItems.empty() );

		m_fileListCtrl.EnsureVisible( static_cast< int >( FindItemPos( m_errorItems.front()->GetKeyPath() ) ), FALSE );
		m_fileListCtrl.Invalidate();

		SwitchMode( MakeMode );
		if ( !silent )
			ui::ReportError( str::Format( _T("Detected duplicate file name collisions in destination:\r\n%s"), JoinErrorDestPaths().c_str() ) );
	}
}

void CMainRenameDialog::PopStackTop( cmd::StackType stackType )
{
	ASSERT( m_mode != RollBackMode && m_mode != RollForwardMode );

	if ( m_pFileModel->CanUndoRedo( stackType, cmd::RenameFile ) )		// is this the proper dialog editor?
	{
		ClearFileErrors();
		m_pFileModel->FetchFromStack( stackType );		// fetch dataset from the stack top macro command
		MarkInvalidSrcItems();
		SwitchMode( cmd::Undo == stackType ? RollBackMode : RollForwardMode );
	}
	else
		PopStackRunCrossEditor( stackType );			// end this dialog and execute the target dialog editor
}

void CMainRenameDialog::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage )
{
	pMessage;

	if ( m_hWnd != NULL )
		if ( m_pFileModel == pSubject )
		{
			if ( NULL == m_pRenSvc.get() )
				m_pRenSvc.reset( new CRenameService( m_rRenameItems ) );		// lazy init
			else
				m_pRenSvc->StoreRenameItems( m_rRenameItems );					// update the current object since it may be referenced in CReplaceDialog

			SetupFileListView();
		}
		else if ( &CGeneralOptions::Instance() == pSubject )
			CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );
}

void CMainRenameDialog::ClearFileErrors( void )
{
	m_errorItems.clear();

	if ( m_hWnd != NULL )
		m_fileListCtrl.Invalidate();
}

void CMainRenameDialog::OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg )
{
	errMsg;

	size_t pos = FindItemPos( srcPath );
	if ( pos != utl::npos )
	{
		utl::AddUnique( m_errorItems, m_rRenameItems[ pos ] );
		m_fileListCtrl.EnsureVisible( static_cast< int >( pos ), FALSE );
	}
	m_fileListCtrl.Invalidate();
}

void CMainRenameDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const
{
	subItem;

	static const ui::CTextEffect s_errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );
	const CRenameItem* pItem = CReportListControl::AsPtr< CRenameItem >( rowKey );

	if ( utl::Contains( m_errorItems, pItem ) )
		rTextEffect.Combine( s_errorBk );
}

void CMainRenameDialog::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
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
		default:
			__super::QueryTooltipText( rText, cmdId, pTooltip );
	}
}

size_t CMainRenameDialog::FindItemPos( const fs::CPath& srcPath ) const
{
	return utl::BinaryFindPos( m_rRenameItems, srcPath, CPathItemBase::ToKeyPath() );
}

void CMainRenameDialog::MarkInvalidSrcItems( void )
{
	for ( std::vector< CRenameItem* >::const_iterator itRenameItem = m_rRenameItems.begin(); itRenameItem != m_rRenameItems.end(); ++itRenameItem )
		if ( !( *itRenameItem )->GetKeyPath().FileExist() )
			utl::AddUnique( m_errorItems, *itRenameItem );
}

std::tstring CMainRenameDialog::JoinErrorDestPaths( void ) const
{
	std::vector< fs::CPath > destPaths; destPaths.reserve( m_errorItems.size() );

	for ( std::vector< CPathItemBase* >::const_iterator itErrorItem = m_errorItems.begin(); itErrorItem != m_errorItems.end(); ++itErrorItem )
		destPaths.push_back( checked_static_cast< const CRenameItem* >( *itErrorItem )->GetDestPath() );

	return str::Join( destPaths, _T("\r\n") );
}

void CMainRenameDialog::AutoGenerateFiles( void )
{
	std::tstring renameFormat = m_formatCombo.GetCurrentText();
	UINT seqCount = m_seqCountEdit.GetNumericValue();
	bool succeeded = GenerateDestPaths( renameFormat, &seqCount );

	PostMakeDest( true );							// silent mode, no modal messages
	m_formatCombo.SetFrameColor( succeeded ? CLR_NONE : color::Error );
}

bool CMainRenameDialog::GenerateDestPaths( const std::tstring& format, UINT* pSeqCount )
{
	ASSERT_PTR( pSeqCount );
	ASSERT_PTR( m_pRenSvc.get() );

	ClearFileErrors();

	fs::TPathPairMap renamePairs;
	ren::MakePairsFromItems( renamePairs, m_rRenameItems );

	CPathGenerator generator( &renamePairs, format, *pSeqCount );
	if ( !generator.GeneratePairs() )
		return false;

	ren::AssignPairsToItems( m_rRenameItems, renamePairs );
	*pSeqCount = generator.GetSeqCount();

	m_pFileModel->UpdateAllObservers( NULL );
	return true;
}

void CMainRenameDialog::EnsureUniformNumPadding( void )
{
	std::vector< std::tstring > fnames; fnames.reserve( m_rRenameItems.size() );

	for ( std::vector< CRenameItem* >::const_iterator itItem = m_rRenameItems.begin(); itItem != m_rRenameItems.end(); ++itItem )
		fnames.push_back( fs::CPathParts( ( *itItem )->GetSafeDestPath().Get() ).m_fname );

	num::EnsureUniformZeroPadding( fnames );

	m_pFileModel->ForEachRenameDestination( func::AssignFname( fnames.begin() ) );
}

bool CMainRenameDialog::ChangeSeqCount( UINT seqCount )
{
	if ( m_seqCountEdit.GetNumber< UINT >() == seqCount )
		return false;

	m_seqCountEdit.SetNumericValue( seqCount );
	OnEnChange_SeqCounter();
	return true;
}

void CMainRenameDialog::ReplaceFormatEditText( const std::tstring& text )
{
	if ( CEdit* pComboEdit = (CEdit*)m_formatCombo.GetWindow( GW_CHILD ) )
	{
		pComboEdit->SetFocus();
		pComboEdit->ReplaceSel( text.c_str(), TRUE );
	}
}

void CMainRenameDialog::DoDataExchange( CDataExchange* pDX )
{
	const bool firstInit = NULL == m_fileListCtrl.m_hWnd;

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

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_formatCombo.LimitText( _MAX_PATH );
			CTextEdit::SetFixedFont( &m_delimiterSetCombo );
			m_delimiterSetCombo.LimitText( 64 );
			m_newDelimiterEdit.LimitText( 64 );

			m_formatCombo.LoadHistory( m_regSection.c_str(), reg::entry_formatHistory, _T("## - *.*") );
			m_delimiterSetCombo.LoadHistory( m_regSection.c_str(), reg::entry_delimiterSetHistory, defaultDelimiterSet );
			m_newDelimiterEdit.SetWindowText( AfxGetApp()->GetProfileString( m_regSection.c_str(), reg::entry_newDelimiterHistory, defaultNewDelimiter ) );

			m_seqCountEdit.SetNumericValue( AfxGetApp()->GetProfileInt( m_regSection.c_str(), reg::entry_seqCount, 1 ) );

			OnUpdate( m_pFileModel, NULL );			// initialize the dialog
			SwitchMode( m_mode );					// normally MakeMode
		}
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CMainRenameDialog, CFileEditorBaseDialog )
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI_RANGE( IDC_UNDO_BUTTON, IDC_REDO_BUTTON, OnUpdateUndoRedo )
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
	ON_SBN_RIGHTCLICKED( IDC_CAPITALIZE_BUTTON, OnSbnRightClicked_CapitalizeOptions )
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
	BOOL defaultFocus = __super::OnInitDialog();
	if ( m_autoGenerate )
		AutoGenerateFiles();

	UINT cmdId = 0, flashId = 0;
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

			if ( GenerateDestPaths( renameFormat, &newSeqCount ) )
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
				m_formatCombo.SaveHistory( m_regSection.c_str(), reg::entry_formatHistory );
				m_delimiterSetCombo.SaveHistory( m_regSection.c_str(), reg::entry_delimiterSetHistory );
				AfxGetApp()->WriteProfileString( m_regSection.c_str(), reg::entry_newDelimiterHistory, ui::GetWindowText( &m_newDelimiterEdit ).c_str() );
				AfxGetApp()->WriteProfileInt( m_regSection.c_str(), reg::entry_seqCount, m_seqCountEdit.GetNumericValue() );

				__super::OnOK();
			}
			else
				SwitchMode( MakeMode );
			break;
		case RollBackMode:
		case RollForwardMode:
		{
			cmd::CScopedErrorObserver observe( this );

			if ( m_pFileModel->UndoRedo( RollBackMode == m_mode ? cmd::Undo : cmd::Redo ) )
				__super::OnOK();
			else
				SwitchMode( MakeMode );
			break;
		}
	}
}

void CMainRenameDialog::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_autoGenerate, m_autoGenerate );
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_seqCountAutoAdvance, m_seqCountAutoAdvance );
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_changeCase, m_changeCaseButton.GetSelValue() );

	__super::OnDestroy();
}

void CMainRenameDialog::OnUpdateUndoRedo( CCmdUI* pCmdUI )
{
	switch ( pCmdUI->m_nID )
	{
		case IDC_UNDO_BUTTON:
			pCmdUI->Enable( m_mode != RollBackMode && m_mode != RollForwardMode && m_pFileModel->CanUndoRedo( cmd::Undo ) );
			break;
		case IDC_REDO_BUTTON:
			pCmdUI->Enable( m_mode != RollForwardMode && m_mode != RollBackMode && m_pFileModel->CanUndoRedo( cmd::Redo ) );
			break;
	}
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
	ASSERT_PTR( m_pRenSvc.get() );

	UINT seqCount = m_pRenSvc->FindNextAvailSeqCount( m_formatCombo.GetCurrentText() );
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
	if ( !m_pFileModel->CopyClipSourcePaths( fmt::FilenameExt, this ) )
		AfxMessageBox( _T("Couldn't copy source files to clipboard!"), MB_ICONERROR | MB_OK );
}

void CMainRenameDialog::OnBnClicked_ClearDestFiles( void )
{
	ClearFileErrors();
	m_pFileModel->ResetDestinations();

	SetupFileListView();	// fill in and select the found files list
	SwitchMode( MakeMode );
}

void CMainRenameDialog::OnBnClicked_PasteDestFiles( void )
{
	try
	{
		ClearFileErrors();
		m_pFileModel->PasteClipDestinationPaths( this );
		PostMakeDest();
	}
	catch ( CRuntimeException& exc )
	{
		exc.ReportError();
	}
}

void CMainRenameDialog::OnBnClicked_CapitalizeDestFiles( void )
{
	CTitleCapitalizer capitalizer;
	m_pFileModel->ForEachRenameDestination( func::CapitalizeWords( &capitalizer ) );
	PostMakeDest();
}

void CMainRenameDialog::OnSbnRightClicked_CapitalizeOptions( void )
{
	COptionsSheet sheet( this, COptionsSheet::CapitalizePage );
	sheet.DoModal();
}

void CMainRenameDialog::OnBnClicked_ChangeCase( void )
{
	m_pFileModel->ForEachRenameDestination( func::MakeCase( m_changeCaseButton.GetSelEnum< ChangeCase >() ) );
	PostMakeDest();
}

void CMainRenameDialog::OnBnClicked_ReplaceDestFiles( void )
{
	CReplaceDialog dlg( this, m_pRenSvc.get() );
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

	m_pFileModel->ForEachRenameDestination( func::ReplaceDelimiterSet( delimiterSet, ui::GetWindowText( &m_newDelimiterEdit ) ) );
	PostMakeDest();
}

void CMainRenameDialog::OnFieldChanged( void )
{
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
	ASSERT_PTR( m_pRenSvc.get() );

	m_pPickDataset = m_pRenSvc->MakeDirPickDataset();

	if ( m_pPickDataset->HasSubDirs() )
		if ( ui::IsKeyPressed( VK_CONTROL ) )
			ReplaceFormatEditText( m_pPickDataset->GetSubDirs().back() );		// pick the parent directory
		else
		{
			CMenu popupMenu;
			m_pPickDataset->MakePickDirMenu( &popupMenu );
			m_formatToolbar.TrackButtonMenu( ID_PICK_DIR_PATH, this, &popupMenu, ui::DropRight );
		}
}

void CMainRenameDialog::OnDirPathPicked( UINT cmdId )
{
	ASSERT_PTR( m_pPickDataset.get() );
	ReplaceFormatEditText( m_pPickDataset->GetPickedDirectory( cmdId ) );
	m_pPickDataset.reset();
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
	std::tstring format = CRenameService::ApplyTextTool( cmdId, oldFormat );

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
	pCmdUI->Enable( m_mode != RollBackMode );
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
	else
		SwitchMode( MakeMode );
}

void CMainRenameDialog::OnBnClicked_PickRenameActions( void )
{
	m_pickRenameActionsStatic.TrackMenu( this, IDR_CONTEXT_MENU, popup::MoreRenameActions );
}

void CMainRenameDialog::OnSingleWhitespace( void )
{
	m_pFileModel->ForEachRenameDestination( func::SingleWhitespace() );
	PostMakeDest();
}

void CMainRenameDialog::OnRemoveWhitespace( void )
{
	m_pFileModel->ForEachRenameDestination( func::RemoveWhitespace() );
	PostMakeDest();
}

void CMainRenameDialog::OnDashToSpace( void )
{
	m_pFileModel->ForEachRenameDestination( func::ReplaceDelimiterSet( _T("-"), _T(" ") ) );
	PostMakeDest();
}

void CMainRenameDialog::OnSpaceToDash( void )
{
	m_pFileModel->ForEachRenameDestination( func::ReplaceDelimiterSet( _T(" "), _T("-") ) );
	PostMakeDest();
}

void CMainRenameDialog::OnUnderbarToSpace( void )
{
	m_pFileModel->ForEachRenameDestination( func::ReplaceDelimiterSet( _T("_"), _T(" ") ) );
	PostMakeDest();
}

void CMainRenameDialog::OnSpaceToUnderbar( void )
{
	m_pFileModel->ForEachRenameDestination( func::ReplaceDelimiterSet( _T(" "), _T("_") ) );
	PostMakeDest();
}

void CMainRenameDialog::OnEnsureUniformNumPadding( void )
{
	EnsureUniformNumPadding();
	PostMakeDest();
}
