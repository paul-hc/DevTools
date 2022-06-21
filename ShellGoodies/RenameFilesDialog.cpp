
#include "stdafx.h"
#include "RenameFilesDialog.h"
#include "RenamePages.h"
#include "FileModel.h"
#include "FileService.h"
#include "EditingCommands.h"
#include "RenameService.h"
#include "RenameItem.h"
#include "PathItemSorting.h"
#include "TextAlgorithms.h"
#include "ReplaceDialog.h"
#include "OptionsSheet.h"
#include "Application.h"
#include "AppCommands.h"
#include "resource.h"
#include "utl/Algorithms.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/NumericProcessor.h"
#include "utl/PathGenerator.h"
#include "utl/RuntimeException.h"
#include "utl/UI/BalloonMessageTip.h"
#include "utl/UI/CmdInfoStore.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/VisualTheme.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/TandemControls.hxx"


namespace reg
{
	static const TCHAR section_mainDialog[] = _T("RenameDialog");

	static const TCHAR entry_formatHistory[] = _T("Format History");
	static const TCHAR entry_autoGenerate[] = _T("Auto Generate");
	static const TCHAR entry_seqCount[] = _T("Sequence Count");
	static const TCHAR entry_seqCountAutoAdvance[] = _T("Sequence Count Auto Advance");
	static const TCHAR entry_ignoreExtension[] = _T("Ignore Extension");
	static const TCHAR entry_changeCase[] = _T("Change Case");
	static const TCHAR entry_delimiterSetHistory[] = _T("Delimiter Set History");
	static const TCHAR entry_newDelimiterHistory[] = _T("New Delimiter History");
}

static const TCHAR s_defaultNewDelimiter[] = _T(" ");
static const TCHAR s_specialSep[] = _T("\n");


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_FORMAT_COMBO, SizeX },
		{ IDC_STRIP_BAR_1, MoveX },

		{ IDC_FILES_SHEET, Size },

		{ IDC_SOURCE_FILES_GROUP, MoveY },
		{ IDC_COPY_SOURCE_PATHS_BUTTON, MoveY },

		{ IDC_DESTINATION_FILES_GROUP, Move },
		{ IDC_PASTE_FILES_BUTTON, Move },
		{ IDC_RESET_FILES_BUTTON, Move },
		{ IDC_CAPITALIZE_BUTTON, Move },
		{ IDC_CHANGE_CASE_BUTTON, Move },

		{ IDC_REPLACE_FILES_BUTTON, Move },
		{ IDC_REPLACE_USER_DELIMS_BUTTON, Move },
		{ IDC_DELIMITER_SET_COMBO, Move },
		{ IDC_DELIMITER_STATIC, Move },
		{ IDC_NEW_DELIMITER_EDIT, Move },
		{ IDC_PICK_RENAME_ACTIONS, Move },

		{ IDOK, MoveX },
		{ IDCANCEL, MoveX },
		{ IDC_TOOLBAR_PLACEHOLDER, MoveX }
	};
}


CRenameFilesDialog::CRenameFilesDialog( CFileModel* pFileModel, CWnd* pParent )
	: CFileEditorBaseDialog( pFileModel, cmd::RenameFile, IDD_RENAME_FILES_DIALOG, pParent )
	, m_rRenameItems( m_pFileModel->LazyInitRenameItems() )
	, m_isInitialized( false )
	, m_autoGenerate( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_autoGenerate, false ) != FALSE )
	, m_seqCountAutoAdvance( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_seqCountAutoAdvance, true ) != FALSE )
	, m_ignoreExtension( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_ignoreExtension, true ) != FALSE )
	, m_prevGenSeqCount( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_seqCount, 1 ) )
	, m_pDisplayFilenameAdapter( new CDisplayFilenameAdapter( m_ignoreExtension ) )
	, m_formatCombo( ui::EditShinkHost_MateOnRight )
	, m_sortOrderCombo( &ren::ui::GetTags_UiSortBy() )
	, m_changeCaseButton( &GetTags_ChangeCase() )
	, m_delimiterSetCombo( ui::HistoryMaxSize, s_specialSep )
	, m_delimStatic( CThemeItem( L"EXPLORERBAR", vt::EBP_IEBARMENU, vt::EBM_NORMAL ) )
	, m_pickRenameActionsStatic( ui::DropDown )
{
	REQUIRE( !m_rRenameItems.empty() );
	m_nativeCmdTypes.push_back( cmd::ChangeDestPaths );
	m_nativeCmdTypes.push_back( cmd::ResetDestinations );

	m_regSection = reg::section_mainDialog;
	RegisterCtrlLayout( ARRAY_PAIR( layout::styles ) );

	m_filesSheet.m_regSection = CFileModel::section_filesSheet;
	m_filesSheet.AddPage( new CRenameSimpleListPage( this ) );
	m_filesSheet.AddPage( new CRenameDetailsListPage( this ) );
	m_filesSheet.AddPage( new CRenameEditPage( this ) );

	m_changeCaseButton.SetSelValue( AfxGetApp()->GetProfileInt( reg::section_mainDialog, reg::entry_changeCase, ExtLowerCase ) );
	ClearFlag( m_delimiterSetCombo.RefItemContent().m_itemsFlags, ui::CItemContent::RemoveEmpty | ui::CItemContent::Trim );

	m_accelPool.AddAccelTable( new CAccelTable( IDD_RENAME_FILES_DIALOG ) );

	m_formatCombo.SetMaxCount( ui::HistoryMaxSize );
	m_formatCombo.SetItemSep( s_specialSep );
	m_formatCombo.GetMateToolbar()->GetStrip()
		.AddButton( ID_PICK_FORMAT_TOKEN )
		.AddSeparator()
		.AddButton( ID_PICK_TEXT_TOOLS )
		.AddButton( ID_PICK_DIR_PATH )
		.AddSeparator()
		.AddButton( ID_GENERATE_NOW )
		.AddButton( ID_TOGGLE_AUTO_GENERATE );

	m_seqCountToolbar.GetStrip()
		.AddButton( ID_SEQ_COUNT_RESET )
		.AddButton( ID_SEQ_COUNT_FIND_NEXT )
		.AddSeparator()
		.AddButton( ID_SEQ_COUNT_AUTO_ADVANCE );

	m_showExtButton.SetFrameMargins( -2, -2 );		// draw the frame outside of the button, in the dialog area
}

CRenameFilesDialog::~CRenameFilesDialog()
{
}

bool CRenameFilesDialog::RenameFiles( void )
{
	CFileService svc;
	std::auto_ptr<CMacroCommand> pRenameMacroCmd = svc.MakeRenameCmds( m_rRenameItems );
	if ( pRenameMacroCmd.get() != NULL )
		if ( !pRenameMacroCmd->IsEmpty() )
		{
			m_errorItems.clear();

			cmd::CScopedErrorObserver observe( this );
			return SafeExecuteCmd( pRenameMacroCmd.release() );
		}
		else
			return PromptCloseDialog();

	return false;
}

bool CRenameFilesDialog::HasDestPaths( void ) const
{
	return utl::Any( m_rRenameItems, std::mem_fun( &CRenameItem::HasDestPath ) );
}

void CRenameFilesDialog::SwitchMode( Mode mode ) override
{
	m_mode = mode;
	if ( NULL == m_hWnd )
		return;

	static const CEnumTags modeTags( _T("&Make Names|Rena&me|Roll &Back|Roll &Fwd") );
	UpdateOkButton( modeTags.FormatUi( m_mode ) );

	static const UINT ctrlIds[] =
	{
		IDC_FORMAT_COMBO, IDC_STRIP_BAR_1, IDC_STRIP_BAR_2, IDC_COPY_SOURCE_PATHS_BUTTON,
		IDC_PASTE_FILES_BUTTON, IDC_RESET_FILES_BUTTON, IDC_CAPITALIZE_BUTTON, IDC_CHANGE_CASE_BUTTON,
		IDC_REPLACE_FILES_BUTTON, IDC_REPLACE_USER_DELIMS_BUTTON, IDC_DELIMITER_SET_COMBO, IDC_NEW_DELIMITER_EDIT
	};
	ui::EnableControls( *this, ctrlIds, COUNT_OF( ctrlIds ), !IsRollMode() );

	ui::EnableControl( *this, IDOK, m_mode != CommitFilesMode || HasDestPaths() );
}

void CRenameFilesDialog::PostMakeDest( bool silent /*= false*/ ) override
{
	// note: the list is set-up on OnUpdate()

	if ( !silent )
		GotoDlgCtrl( GetDlgItem( IDOK ) );

	ASSERT_PTR( m_pRenSvc.get() );

	if ( m_pRenSvc->CheckPathCollisions( this ) )		// stores erros in m_errorItems
		SwitchMode( CommitFilesMode );
	else
	{
		if ( const CRenameItem* pFirstErrorItem = GetFirstErrorItem< CRenameItem >() )
			for ( int i = 0; i != m_filesSheet.GetPageCount(); ++i )
				if ( IRenamePage* pPage = m_filesSheet.GetCreatedPageAs<IRenamePage>( i ) )
					pPage->EnsureVisibleItem( pFirstErrorItem );

		SwitchMode( EditMode );
		if ( !silent )
			ui::ReportError( str::Format( _T("Detected duplicate file name collisions in destination:\r\n%s"), JoinErrorDestPaths().c_str() ) );
	}
}

void CRenameFilesDialog::PopStackTop( svc::StackType stackType ) override
{
	ASSERT( !IsRollMode() );

	if ( utl::ICommand* pTopCmd = PeekCmdForDialog( stackType ) )		// comand that is target for this dialog editor?
	{
		bool isRenameMacro = cmd::RenameFile == pTopCmd->GetTypeID();

		ClearFileErrors();

		if ( isRenameMacro )
			m_pFileModel->FetchFromStack( stackType );		// fetch dataset from the stack top macro command
		else
			m_pCmdSvc->UndoRedo( stackType );

		MarkInvalidSrcItems();

		if ( isRenameMacro )								// file command?
			SwitchMode( svc::Undo == stackType ? RollBackMode : RollForwardMode );
		else
			SwitchMode( HasDestPaths() ? CommitFilesMode : EditMode );
	}
	else
		PopStackRunCrossEditor( stackType );				// end this dialog and execute the target dialog editor
}

void CRenameFilesDialog::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override
{
	if ( m_hWnd != NULL )
	{
		if ( m_pFileModel == pSubject )
		{
			if ( NULL == m_pRenSvc.get() )
				m_pRenSvc.reset( new CRenameService( m_rRenameItems ) );		// lazy init
			else
				m_pRenSvc->StoreRenameItems( m_rRenameItems );					// update the current object since it may be referenced in CReplaceDialog

			if ( CSortRenameItemsCmd* pSortCmd = dynamic_cast<CSortRenameItemsCmd*>( pMessage ) )
			{
				if ( pSortCmd->m_pPage != NULL )					// triggered by a list-ctrl, not internally
					m_sortOrderCombo.SetValue( ren::ui::FromSortingPair( pSortCmd->m_sorting ) );

				if ( CommitFilesMode == m_mode && HasDestPaths() )
					ChangeSeqCount( m_prevGenSeqCount );			// allow user to re-generate the dest sequence, by switching to EditMode
			}
		}

		for ( int i = 0; i != m_filesSheet.GetPageCount(); ++i )
			if ( utl::IObserver* pPage = m_filesSheet.GetCreatedPageAs<utl::IObserver>( i ) )
				pPage->OnUpdate( pSubject, pMessage );

		if ( m_pFileModel == pSubject )
			if ( cmd::ChangeDestPaths == utl::GetSafeTypeID( pMessage ) )
				PostMakeDest();
	}
}

void CRenameFilesDialog::ClearFileErrors( void ) override
{
	m_errorItems.clear();

	for ( int i = 0; i != m_filesSheet.GetPageCount(); ++i )
		if ( IRenamePage* pPage = m_filesSheet.GetCreatedPageAs<IRenamePage>( i ) )
			pPage->InvalidateFiles();
}

void CRenameFilesDialog::OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg ) override
{
	errMsg;

	if ( CRenameItem* pErrorItem = FindItemWithKey( srcPath ) )
	{
		utl::AddUnique( m_errorItems, pErrorItem );

		for ( int i = 0; i != m_filesSheet.GetPageCount(); ++i )
			if ( IRenamePage* pPage = m_filesSheet.GetCreatedPageAs<IRenamePage>( i ) )
				pPage->EnsureVisibleItem( pErrorItem );
	}
}

void CRenameFilesDialog::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override
{
	switch ( cmdId )
	{
		case IDC_SORT_ORDER_COMBO:
			rText = m_sortOrderCombo.GetTags()->FormatKey( m_sortOrderCombo.GetValue() );
			break;
		case IDC_CHANGE_CASE_BUTTON:
		{
			static const std::vector< std::tstring > tooltips = str::LoadStrings( IDC_CHANGE_CASE_BUTTON );
			ChangeCase changeCase = m_changeCaseButton.GetSelEnum<ChangeCase>();
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

void CRenameFilesDialog::UpdateFormatLabel( void )
{
	ui::SetDlgItemText( this, IDC_FORMAT_LABEL, str::Format( _T("&Template \"%s\":"), m_ignoreExtension ? _T("name") : _T("name.ext") ) );
}

void CRenameFilesDialog::CommitLocalEdits( void )
{
	m_filesSheet.GetPageAs< CRenameEditPage >( EditPage )->CommitLocalEdits();			// allow detail page edit input
}

CRenameItem* CRenameFilesDialog::FindItemWithKey( const fs::CPath& srcPath ) const
{
	std::vector< CRenameItem* >::const_iterator itFoundItem = utl::BinaryFind( m_rRenameItems, srcPath, CPathItemBase::ToFilePath() );
	return itFoundItem != m_rRenameItems.end() ? *itFoundItem : NULL;
}

void CRenameFilesDialog::MarkInvalidSrcItems( void )
{
	for ( std::vector< CRenameItem* >::const_iterator itRenameItem = m_rRenameItems.begin(); itRenameItem != m_rRenameItems.end(); ++itRenameItem )
		if ( !( *itRenameItem )->GetSrcPath().FileExist() )
			utl::AddUnique( m_errorItems, *itRenameItem );
}

std::tstring CRenameFilesDialog::JoinErrorDestPaths( void ) const
{
	std::vector< fs::CPath > destPaths; destPaths.reserve( m_errorItems.size() );

	for ( std::vector< CPathItemBase* >::const_iterator itErrorItem = m_errorItems.begin(); itErrorItem != m_errorItems.end(); ++itErrorItem )
		destPaths.push_back( checked_static_cast<const CRenameItem*>( *itErrorItem )->GetDestPath() );

	return str::Join( destPaths, _T("\r\n") );
}

std::tstring CRenameFilesDialog::GetSelFindWhat( void ) const
{
	if ( CTextEdit* pFocusEdit = dynamic_cast<CTextEdit*>( GetFocus() ) )
	{
		std::tstring selText = pFocusEdit->GetSelText();
		if ( !selText.empty() && std::tstring::npos == selText.find( _T('\n') ) )
			return selText;		// user is attempting to replace selected text in an edit box
	}
	return str::GetEmpty();
}

CPathFormatter CRenameFilesDialog::InputRenameFormatter( bool checkConsistent ) const
{
	fs::CPath renameFormat = m_pDisplayFilenameAdapter->ParseFilename( m_formatCombo.GetCurrentText(), fs::CPath( _T("null") ) );
	CPathFormatter pathFormatter( renameFormat.Get(), m_ignoreExtension );

	if ( checkConsistent )
		if ( !pathFormatter.IsConsistent() )
		{
			ui::ShowBalloonTip( &m_formatCombo, _T("Ignore Extension mode"),
				str::Format( _T("Ignoring the extension part \"%s\" in the format!"), pathFormatter.GetFormat().GetExt() ),
				(HICON)TTI_WARNING );

			pathFormatter = pathFormatter.MakeConsistent();
		}

	return pathFormatter;
}

bool CRenameFilesDialog::IsFormatExtConsistent( void ) const
{
	return m_ignoreExtension == !fs::CPath( m_formatCombo.GetCurrentText() ).HasExt();
}

bool CRenameFilesDialog::GenerateDestPaths( const CPathFormatter& pathFormatter, UINT* pSeqCount )
{
	ASSERT_PTR( pSeqCount );
	ASSERT_PTR( m_pRenSvc.get() );

	ClearFileErrors();

	CPathRenamePairs renamePairs;
	ren::MakePairsFromItems( &renamePairs, m_rRenameItems );

	CPathGenerator generator( &renamePairs, pathFormatter, *pSeqCount );
	if ( !generator.GeneratePairs() )
		return false;

	ren::AssignPairsToItems( m_rRenameItems, renamePairs );
	*pSeqCount = generator.GetSeqCount();

	m_pFileModel->UpdateAllObservers( NULL );
	return true;
}

void CRenameFilesDialog::AutoGenerateFiles( void )
{
	UINT seqCount = m_seqCountEdit.GetNumericValue();
	bool succeeded = GenerateDestPaths( InputRenameFormatter( true ), &seqCount );

	PostMakeDest( true );							// silent mode, no modal messages
	m_formatCombo.SetFrameColor( succeeded ? CLR_NONE : color::Error );

	// note: it does not increment m_seqCountEdit
}

bool CRenameFilesDialog::ChangeSeqCount( UINT seqCount )
{
	if ( m_seqCountEdit.GetNumber<UINT>() == seqCount )
		return false;

	m_seqCountEdit.SetNumericValue( seqCount );
	OnEnChange_SeqCounter();
	return true;
}

void CRenameFilesDialog::ReplaceFormatEditText( const std::tstring& text )
{
	if ( CEdit* pComboEdit = m_formatCombo.GetEdit() )
	{
		pComboEdit->SetFocus();
		pComboEdit->ReplaceSel( text.c_str(), TRUE );
	}
}

void CRenameFilesDialog::DoDataExchange( CDataExchange* pDX ) override
{
	const bool firstInit = NULL == m_filesSheet.m_hWnd;

	m_filesSheet.DDX_DetailSheet( pDX, IDC_FILES_SHEET );

	DDX_Control( pDX, IDC_FORMAT_COMBO, m_formatCombo );
	DDX_Control( pDX, IDC_SEQ_COUNT_EDIT, m_seqCountEdit );
	m_seqCountToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_2, H_AlignLeft | V_AlignCenter );
	DDX_Control( pDX, IDC_SHOW_EXTENSION_CHECK, m_showExtButton );
	DDX_Control( pDX, IDC_SORT_ORDER_COMBO, m_sortOrderCombo );
	DDX_Control( pDX, IDC_CAPITALIZE_BUTTON, m_capitalizeButton );
	DDX_Control( pDX, IDC_CHANGE_CASE_BUTTON, m_changeCaseButton );
	DDX_Control( pDX, IDC_DELIMITER_SET_COMBO, m_delimiterSetCombo );
	DDX_Control( pDX, IDC_NEW_DELIMITER_EDIT, m_newDelimiterEdit );
	DDX_Control( pDX, IDC_DELIMITER_STATIC, m_delimStatic );
	DDX_Control( pDX, IDC_PICK_RENAME_ACTIONS, m_pickRenameActionsStatic );
	ui::DDX_ButtonIcon( pDX, IDC_COPY_SOURCE_PATHS_BUTTON, ID_EDIT_COPY );
	ui::DDX_ButtonIcon( pDX, IDC_PASTE_FILES_BUTTON, ID_EDIT_PASTE );
	ui::DDX_ButtonIcon( pDX, IDC_RESET_FILES_BUTTON, ID_RESET_DEFAULT );
	ui::DDX_ButtonIcon( pDX, IDC_REPLACE_FILES_BUTTON, ID_EDIT_REPLACE );

	if ( firstInit )
	{
		m_formatCombo.LimitText( _MAX_PATH );
		CTextEdit::SetFixedFont( &m_delimiterSetCombo );
		m_delimiterSetCombo.LimitText( 64 );
		m_newDelimiterEdit.LimitText( 64 );

		m_formatCombo.LoadHistory( m_regSection.c_str(), reg::entry_formatHistory, m_ignoreExtension ? _T("## - *") : _T("## - *.*") );
		m_delimiterSetCombo.LoadHistory( m_regSection.c_str(), reg::entry_delimiterSetHistory, delim::s_defaultDelimiterSet.c_str() );
		m_newDelimiterEdit.SetWindowText( AfxGetApp()->GetProfileString( m_regSection.c_str(), reg::entry_newDelimiterHistory, s_defaultNewDelimiter ) );

		m_seqCountEdit.SetNumericValue( m_prevGenSeqCount );
		m_showExtButton.SetCheck( !m_ignoreExtension );			// checkbox has inverted logic
		m_sortOrderCombo.SetValue( ren::ui::FromSortingPair( m_pFileModel->GetRenameSorting() ) );
		UpdateFormatLabel();

		m_isInitialized = true;

		if ( !HasDestPaths() && EditPage == m_filesSheet.GetActiveIndex() )
			CResetDestinationsCmd( m_pFileModel ).Execute();	// this will call OnUpdate() for all observers
		else
			OnUpdate( m_pFileModel, NULL );			// initialize the dialog
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CRenameFilesDialog, CFileEditorBaseDialog )
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
	ON_UPDATE_COMMAND_UI_RANGE( IDC_UNDO_BUTTON, IDC_REDO_BUTTON, OnUpdateUndoRedo )
	ON_CBN_EDITCHANGE( IDC_FORMAT_COMBO, OnChanged_Format )
	ON_CBN_SELCHANGE( IDC_FORMAT_COMBO, OnChanged_Format )
	ON_COMMAND( ID_PICK_FORMAT_TOKEN, OnPickFormatToken )
	ON_COMMAND( ID_PICK_DIR_PATH, OnPickDirPath )
	ON_COMMAND_RANGE( IDC_PICK_DIR_PATH_BASE, IDC_PICK_DIR_PATH_MAX, OnDirPathPicked )
	ON_COMMAND( ID_PICK_TEXT_TOOLS, OnPickTextTools )
	ON_COMMAND_RANGE( ID_TEXT_TITLE_CASE, ID_TEXT_SPACE_TO_UNDERBAR, OnFormatTextToolPicked )		// format combo pick text tool
	ON_COMMAND( ID_GENERATE_NOW, OnGenerateNow )
	ON_UPDATE_COMMAND_UI( ID_GENERATE_NOW, OnUpdateGenerateNow )
	ON_COMMAND( ID_TOGGLE_AUTO_GENERATE, OnToggle_AutoGenerate )
	ON_UPDATE_COMMAND_UI( ID_TOGGLE_AUTO_GENERATE, OnUpdate_AutoGenerate )
	ON_COMMAND_RANGE( ID_NUMERIC_SEQUENCE_2DIGITS, ID_EXTENSION_SEPARATOR, OnNumericSequence )
	ON_EN_CHANGE( IDC_SEQ_COUNT_EDIT, OnEnChange_SeqCounter )
	ON_COMMAND( ID_SEQ_COUNT_RESET, OnSeqCountReset )
	ON_UPDATE_COMMAND_UI( ID_SEQ_COUNT_RESET, OnUpdateSeqCountReset )
	ON_COMMAND( ID_SEQ_COUNT_FIND_NEXT, OnSeqCountFindNext )
	ON_UPDATE_COMMAND_UI( ID_SEQ_COUNT_FIND_NEXT, OnUpdateSeqCountFindNext )
	ON_COMMAND( ID_SEQ_COUNT_AUTO_ADVANCE, OnToggle_SeqCountAutoAdvance )
	ON_UPDATE_COMMAND_UI( ID_SEQ_COUNT_AUTO_ADVANCE, OnUpdate_SeqCountAutoAdvance )
	ON_BN_CLICKED( IDC_SHOW_EXTENSION_CHECK, OnToggle_ShowExtension )
	ON_CBN_SELCHANGE( IDC_SORT_ORDER_COMBO, OnCbnSelChange_SortOrder )
	ON_BN_CLICKED( IDC_COPY_SOURCE_PATHS_BUTTON, OnBnClicked_CopySourceFiles )
	ON_BN_CLICKED( IDC_PASTE_FILES_BUTTON, OnBnClicked_PasteDestFiles )
	ON_BN_CLICKED( IDC_RESET_FILES_BUTTON, OnBnClicked_ResetDestFiles )
	ON_BN_CLICKED( IDC_CAPITALIZE_BUTTON, OnBnClicked_CapitalizeDestFiles )
	ON_SBN_RIGHTCLICKED( IDC_CAPITALIZE_BUTTON, OnSbnRightClicked_CapitalizeOptions )
	ON_BN_CLICKED( IDC_CHANGE_CASE_BUTTON, OnBnClicked_ChangeCase )
	ON_BN_CLICKED( IDC_REPLACE_FILES_BUTTON, OnBnClicked_ReplaceDestFiles )
	ON_BN_CLICKED( IDC_REPLACE_USER_DELIMS_BUTTON, OnBnClicked_ReplaceAllDelimitersDestFiles )
	ON_CBN_EDITCHANGE( IDC_DELIMITER_SET_COMBO, OnFieldChanged )
	ON_CBN_SELCHANGE( IDC_DELIMITER_SET_COMBO, OnFieldChanged )
	ON_EN_CHANGE( IDC_NEW_DELIMITER_EDIT, OnFieldChanged )
	ON_BN_CLICKED( IDC_PICK_RENAME_ACTIONS, OnBnClicked_PickRenameActions )
	ON_BN_DOUBLECLICKED( IDC_PICK_RENAME_ACTIONS, OnBnClicked_PickRenameActions )
	ON_COMMAND_RANGE( ID_REPLACE_ALL_DELIMS, ID_SPACE_TO_UNDERBAR, OnChangeDestPathsTool )
	ON_COMMAND( ID_ENSURE_UNIFORM_ZERO_PADDING, OnEnsureUniformNumPadding )		// bottom-right pick DEST tool
	ON_COMMAND( ID_GENERATE_NUMERIC_SEQUENCE, OnGenerateNumericSequence )
END_MESSAGE_MAP()

BOOL CRenameFilesDialog::OnInitDialog( void ) override
{
	BOOL focusDefault = __super::OnInitDialog();

	if ( m_autoGenerate )
		AutoGenerateFiles();

	return focusDefault;
}

void CRenameFilesDialog::OnOK( void ) override
{
	CommitLocalEdits();

	switch ( m_mode )
	{
		case EditMode:
		{
			CPathFormatter pathFormatter = InputRenameFormatter( true );
			UINT oldSeqCount = m_seqCountEdit.GetNumericValue(), newSeqCount = oldSeqCount;

			if ( GenerateDestPaths( pathFormatter, &newSeqCount ) )
			{
				m_prevGenSeqCount = oldSeqCount;
				if ( !m_seqCountAutoAdvance )
					newSeqCount = oldSeqCount;

				if ( newSeqCount != oldSeqCount )
					m_seqCountEdit.SetNumericValue( newSeqCount );

				PostMakeDest();
			}
			else
			{
				ui::MessageBox( str::Format( IDS_INVALID_FORMAT, pathFormatter.GetFormat().GetPtr() ), MB_ICONERROR | MB_OK );
				ui::TakeFocus( m_formatCombo );
			}
			break;
		}
		case CommitFilesMode:
			if ( RenameFiles() )
			{
				m_formatCombo.SaveHistory( m_regSection.c_str(), reg::entry_formatHistory );
				m_delimiterSetCombo.SaveHistory( m_regSection.c_str(), reg::entry_delimiterSetHistory );
				AfxGetApp()->WriteProfileString( m_regSection.c_str(), reg::entry_newDelimiterHistory, ui::GetWindowText( &m_newDelimiterEdit ).c_str() );
				AfxGetApp()->WriteProfileInt( m_regSection.c_str(), reg::entry_seqCount, m_seqCountEdit.GetNumericValue() );

				__super::OnOK();
			}
			else
				SwitchMode( EditMode );
			break;
		case RollBackMode:
		case RollForwardMode:
		{
			cmd::CScopedErrorObserver observe( this );

			if ( m_pCmdSvc->UndoRedo( RollBackMode == m_mode ? svc::Undo : svc::Redo ) ||
				 PromptCloseDialog( PromptClose ) )
				__super::OnOK();
			else
				SwitchMode( EditMode );
			break;
		}
	}
}

void CRenameFilesDialog::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_autoGenerate, m_autoGenerate );
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_seqCountAutoAdvance, m_seqCountAutoAdvance );
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_changeCase, m_changeCaseButton.GetSelValue() );
	AfxGetApp()->WriteProfileInt( reg::section_mainDialog, reg::entry_ignoreExtension, m_ignoreExtension );

	__super::OnDestroy();
}

HBRUSH CRenameFilesDialog::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColorType )
{
	HBRUSH hBrushFill = __super::OnCtlColor( pDC, pWnd, ctlColorType );

	if ( pWnd != NULL )
		switch ( pWnd->GetDlgCtrlID() )
		{
			case IDC_FORMAT_LABEL:
				if ( !IsFormatExtConsistent() )
					pDC->SetTextColor( app::ColorWarningText );
				break;
		}

	return hBrushFill;
}

void CRenameFilesDialog::OnUpdateUndoRedo( CCmdUI* pCmdUI )
{
	switch ( pCmdUI->m_nID )
	{
		case IDC_UNDO_BUTTON:
			pCmdUI->Enable( !IsRollMode() && m_pCmdSvc->CanUndoRedo( svc::Undo ) );
			break;
		case IDC_REDO_BUTTON:
			pCmdUI->Enable( !IsRollMode() && m_pCmdSvc->CanUndoRedo( svc::Redo ) );
			break;
	}
}

void CRenameFilesDialog::OnChanged_Format( void )
{
	OnFieldChanged();

	CPathFormatter pathFormatter = InputRenameFormatter( false );
	m_formatCombo.SetFrameColor( pathFormatter.IsValidFormat() ? CLR_NONE : color::Error );
	m_showExtButton.SetFrameColor( IsFormatExtConsistent() ? CLR_NONE : app::ColorWarningText );

	if ( m_autoGenerate )
		AutoGenerateFiles();
}

void CRenameFilesDialog::OnSeqCountReset( void )
{
	ChangeSeqCount( 1 );
	GotoDlgCtrl( &m_seqCountEdit );
}

void CRenameFilesDialog::OnUpdateSeqCountReset( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_seqCountEdit.GetNumericValue() != 1 );
}

void CRenameFilesDialog::OnSeqCountFindNext( void )
{
	ASSERT_PTR( m_pRenSvc.get() );

	UINT seqCount = m_pRenSvc->FindNextAvailSeqCount( InputRenameFormatter( false ) );
	ChangeSeqCount( seqCount );
	GotoDlgCtrl( &m_seqCountEdit );
}

void CRenameFilesDialog::OnUpdateSeqCountFindNext( CCmdUI* pCmdUI )
{
	CPathFormatter format = InputRenameFormatter( false );
	pCmdUI->Enable( format.IsNumericFormat() );
}

void CRenameFilesDialog::OnToggle_SeqCountAutoAdvance( void )
{
	m_seqCountAutoAdvance = !m_seqCountAutoAdvance;
}

void CRenameFilesDialog::OnUpdate_SeqCountAutoAdvance( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_seqCountAutoAdvance );
}

void CRenameFilesDialog::OnToggle_ShowExtension( void )
{
	m_ignoreExtension = BST_UNCHECKED == m_showExtButton.GetCheck();			// checkbox has inverted logic
	m_pDisplayFilenameAdapter->SetIgnoreExtension( m_ignoreExtension );

	UpdateFormatLabel();
	m_showExtButton.SetFrameColor( IsFormatExtConsistent() ? CLR_NONE : app::ColorWarningText );

	OnUpdate( m_pFileModel, NULL );
}

void CRenameFilesDialog::OnCbnSelChange_SortOrder( void )
{
	ren::TSortingPair sorting = ren::ui::ToSortingPair( m_sortOrderCombo.GetEnum<ren::ui::UiSortBy>() );

	if ( sorting != m_pFileModel->GetRenameSorting() )
		CSortRenameItemsCmd( m_pFileModel, NULL, sorting ).Execute();
}

void CRenameFilesDialog::OnBnClicked_CopySourceFiles( void )
{
	if ( !m_pFileModel->CopyClipSourcePaths( fmt::FilenameExt, this, m_pDisplayFilenameAdapter.get() ) )
		ui::MessageBox( _T("Couldn't copy source files to clipboard!"), MB_ICONERROR | MB_OK );
}

void CRenameFilesDialog::OnBnClicked_PasteDestFiles( void )
{
	try
	{
		ClearFileErrors();
		SafeExecuteCmd( m_pFileModel->MakeClipPasteDestPathsCmd( this, m_pDisplayFilenameAdapter.get() ) );
	}
	catch ( CRuntimeException& exc )
	{
		exc.ReportError();
	}
}

void CRenameFilesDialog::OnBnClicked_ResetDestFiles( void )
{
	ClearFileErrors();
	SafeExecuteCmd( new CResetDestinationsCmd( m_pFileModel ) );
}

void CRenameFilesDialog::OnBnClicked_CapitalizeDestFiles( void )
{
	CommitLocalEdits();

	CTitleCapitalizer capitalizer;
	SafeExecuteCmd( m_pFileModel->MakeChangeDestPathsCmd( func::CapitalizeWords( &capitalizer ), _T("Title Case") ) );
}

void CRenameFilesDialog::OnSbnRightClicked_CapitalizeOptions( void )
{
	COptionsSheet sheet( m_pFileModel, this, COptionsSheet::CapitalizePage );
	sheet.DoModal();
}

void CRenameFilesDialog::OnBnClicked_ChangeCase( void )
{
	CommitLocalEdits();

	ChangeCase selCase = m_changeCaseButton.GetSelEnum<ChangeCase>();
	std::tstring cmdTag = str::Format( _T("Change case to: %s"), GetTags_ChangeCase().FormatUi( selCase ).c_str() );

	if ( m_ignoreExtension )
	{
		static const ChangeCase s_altersTheExt[] = { LowerCase, UpperCase, ExtLowerCase, ExtUpperCase };

		if ( utl::Contains( s_altersTheExt, END_OF( s_altersTheExt ), selCase ) )
			if ( ui::MessageBox( _T("This operation may alter the hidden extension of certain files!\n\nDo you want to proceed?"), MB_ICONWARNING | MB_YESNO ) != IDYES )
				return;
	}

	SafeExecuteCmd( m_pFileModel->MakeChangeDestPathsCmd( func::MakeCase( selCase ), cmdTag ) );
}

void CRenameFilesDialog::OnBnClicked_ReplaceDestFiles( void )
{
	CReplaceDialog dlg( this, m_pRenSvc.get(), GetSelFindWhat() );
	dlg.Execute();
}

void CRenameFilesDialog::OnBnClicked_ReplaceAllDelimitersDestFiles( void )
{
	CommitLocalEdits();

	std::tstring delimiterSet = m_delimiterSetCombo.GetCurrentText();
	if ( delimiterSet.empty() )
	{
		ui::MessageBox( str::Format( IDS_NO_DELIMITER_SET ) );
		ui::TakeFocus( m_delimiterSetCombo );
		return;
	}

	SafeExecuteCmd( m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceDelimiterSet( delimiterSet, ui::GetWindowText( &m_newDelimiterEdit ) ), str::Load( IDC_REPLACE_USER_DELIMS_BUTTON ) ) );
}

void CRenameFilesDialog::OnFieldChanged( void )
{
	SwitchMode( EditMode );
}

void CRenameFilesDialog::OnPickFormatToken( void )
{
//DBG: OnUpdate(NULL, NULL);
	CMenu popupMenu;
	ui::LoadPopupMenu( popupMenu, IDR_CONTEXT_MENU, popup::FormatPicker );

	m_formatCombo.GetMateToolbar()->TrackButtonMenu( ID_PICK_FORMAT_TOKEN, this, &popupMenu, ui::DropDown );
}

void CRenameFilesDialog::OnPickDirPath( void )
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
			m_formatCombo.GetMateToolbar()->TrackButtonMenu( ID_PICK_DIR_PATH, this, &popupMenu, ui::DropRight );
		}
}

void CRenameFilesDialog::OnDirPathPicked( UINT cmdId )
{
	ASSERT_PTR( m_pPickDataset.get() );
	ReplaceFormatEditText( m_pPickDataset->GetPickedDirectory( cmdId ) );
	m_pPickDataset.reset();
}

void CRenameFilesDialog::OnPickTextTools( void )
{
	CMenu popupMenu;
	ui::LoadPopupMenu( popupMenu, IDR_CONTEXT_MENU, popup::TextTools );

	m_formatCombo.GetMateToolbar()->TrackButtonMenu( ID_PICK_TEXT_TOOLS, this, &popupMenu, ui::DropDown );
}

void CRenameFilesDialog::OnFormatTextToolPicked( UINT menuId )
{
	GotoDlgCtrl( &m_formatCombo );

	std::tstring oldFormat = m_formatCombo.GetCurrentText();
	std::tstring format = CRenameService::ApplyTextTool( menuId, oldFormat );

	if ( format != oldFormat )
		ui::SetComboEditText( m_formatCombo, format, str::Case );
}

void CRenameFilesDialog::OnGenerateNow( void )
{
	AutoGenerateFiles();
}

void CRenameFilesDialog::OnUpdateGenerateNow( CCmdUI* pCmdUI )
{
	bool enable = false;

	if ( !IsRollMode() )
	{
		CPathFormatter pathFormatter = InputRenameFormatter( false );
		enable = pathFormatter.IsValidFormat();
	}
	pCmdUI->Enable( enable );
}

void CRenameFilesDialog::OnToggle_AutoGenerate( void )
{
	m_autoGenerate = !m_autoGenerate;
	if ( m_autoGenerate )
		AutoGenerateFiles();
}

void CRenameFilesDialog::OnUpdate_AutoGenerate( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !IsRollMode() );
	pCmdUI->SetCheck( m_autoGenerate );
}

void CRenameFilesDialog::OnNumericSequence( UINT cmdId )
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

void CRenameFilesDialog::OnEnChange_SeqCounter( void )
{
	if ( m_autoGenerate )
		AutoGenerateFiles();
	else
		SwitchMode( EditMode );
}

void CRenameFilesDialog::OnBnClicked_PickRenameActions( void )
{
	m_pickRenameActionsStatic.TrackMenu( this, IDR_CONTEXT_MENU, popup::MoreRenameActions );
}

void CRenameFilesDialog::OnChangeDestPathsTool( UINT menuId )
{
	CommitLocalEdits();

	utl::ICommand* pCmd = NULL;
	std::tstring cmdTag = str::Load( menuId );

	switch ( menuId )
	{
		case ID_REPLACE_ALL_DELIMS:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceDelimiterSet( delim::GetAllDelimitersSet(), _T(" ") ), cmdTag );
			break;
		case ID_REPLACE_UNICODE_SYMBOLS:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceMultiDelimiterSets( &text_tool::GetStdUnicodeToAnsiPairs() ), cmdTag );
			break;
		case ID_SINGLE_WHITESPACE:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::SingleWhitespace(), cmdTag );
			break;
		case ID_REMOVE_WHITESPACE:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::RemoveWhitespace(), cmdTag );
			break;
		case ID_DASH_TO_SPACE:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceDelimiterSet( delim::s_dashes, _T(" ") ), cmdTag );
			break;
		case ID_SPACE_TO_DASH:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceDelimiterSet( _T(" "), _T("-") ), cmdTag );
			break;
		case ID_UNDERBAR_TO_SPACE:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceDelimiterSet( delim::s_underscores, _T(" ") ), cmdTag );
			break;
		case ID_SPACE_TO_UNDERBAR:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceDelimiterSet( _T(" "), _T("_") ), cmdTag );
			break;
		default:
			ASSERT( false );
	}

	SafeExecuteCmd( pCmd );
}

void CRenameFilesDialog::OnEnsureUniformNumPadding( void )
{
	CommitLocalEdits();

	std::vector< std::tstring > destFnames;
	ren::QueryDestFnames( destFnames, m_rRenameItems );

	num::EnsureUniformZeroPadding( destFnames );

	SafeExecuteCmd( m_pFileModel->MakeChangeDestPathsCmd( func::AssignFname( destFnames.begin() ), str::Load( ID_ENSURE_UNIFORM_ZERO_PADDING ) ) );
}

void CRenameFilesDialog::OnGenerateNumericSequence( void )
{
	CommitLocalEdits();

	std::vector< std::tstring > destFnames;
	ren::QueryDestFnames( destFnames, m_rRenameItems );

	try
	{
		num::GenerateNumbersInSequence( destFnames );

		SafeExecuteCmd( m_pFileModel->MakeChangeDestPathsCmd( func::AssignFname( destFnames.begin() ), str::Load( ID_GENERATE_NUMERIC_SEQUENCE ) ) );
	}
	catch ( CRuntimeException& exc )
	{
		exc.ReportError();
	}
}
