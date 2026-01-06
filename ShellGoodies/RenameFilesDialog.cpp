
#include "pch.h"
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
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/ShellDialogs.h"
#include "utl/UI/SystemTray_fwd.h"
#include "utl/UI/TaskDialog.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/VisualTheme.h"
#include "utl/UI/WndUtilsEx.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/SelectionData.hxx"
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

		{ IDC_FILES_STATIC, SizeX },
		{ IDC_FILES_SHEET, Size },
		{ IDC_OUTCOME_INFO_STATUS, MoveY | SizeX },
		{ IDC_CURR_FOLDER_STATIC, MoveY },
		{ IDC_CURR_FOLDER_EDIT, MoveY | SizeX },

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
	, m_formatCombo( ui::EditShrinkHost_MateOnRight )
	, m_filesLabelDivider( CLabelDivider::Instruction )
	, m_fileStatsStatic( ui::EditShrinkHost_MateOnRight )
	, m_changeCaseButton( &GetTags_ChangeCase() )
	, m_delimiterSetCombo( ui::HistoryMaxSize, s_specialSep )
	, m_delimStatic( CThemeItem( L"EXPLORERBAR", vt::EBP_IEBARMENU, vt::EBM_NORMAL ) )
	, m_pickRenameActionsStatic( ui::DropDown )
{
	REQUIRE( !m_rRenameItems.empty() );
	m_nativeCmdTypes.push_back( cmd::ChangeDestPaths );
	m_nativeCmdTypes.push_back( cmd::ResetDestinations );

	m_regSection = reg::section_mainDialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );

	m_filesSheet.m_regSection = CFileModel::section_filesSheet;
	m_filesSheet.AddPage( new CRenameSimpleListPage( this ) );
	m_filesSheet.AddPage( new CRenameDetailsListPage( this ) );
	m_filesSheet.AddPage( new CRenameEditPage( this ) );
	m_filesSheet.AddObserver( this );		// route cmd::OnRenameListSelChanged notifications to this dialog

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

	m_fileStatsStatic.GetMateToolbar()->GetStrip()
		.AddButton( ID_COPY_SEL_ITEMS, ID_EDIT_COPY )
		.AddButton( ID_CMD_RESET_DESTINATIONS );

	m_currFolderEdit.SetUseFixedFont( false );
	m_currFolderEdit.GetMateToolbar()->GetStrip()
		.AddButton( ID_EDIT_COPY )
		.AddButton( ID_BROWSE_FOLDER );

	m_sortOrderCombo.SetTags( &ren::ui::GetTags_UiSortBy() );
	m_showExtButton.SetFrameMargins( -2, -2 );			// draw the frame outside of the button, in the dialog area
	m_targetSelItemsButton.SetFrameMargins( -3, -3 );	// draw the frame outside of the button, in the dialog area
}

CRenameFilesDialog::~CRenameFilesDialog()
{
	m_filesSheet.RemoveObserver( this );
}

const std::vector<CRenameItem*>* CRenameFilesDialog::GetCmdSelItems( void ) const
{
	return app::TargetSelectedItems == m_pFileModel->GetTargetScope() && !m_selData.GetSelItems().empty() ? &m_selData.GetSelItems() : nullptr;
}

const std::vector<CRenameItem*>& CRenameFilesDialog::GetTargetItems( void ) const
{
	if ( const std::vector<CRenameItem*>* pSelItems = GetCmdSelItems() )
		return *pSelItems;

	return m_rRenameItems;
}

bool CRenameFilesDialog::RenameFiles( void )
{
	CFileService svc;
	std::auto_ptr<CMacroCommand> pRenameMacroCmd = svc.MakeRenameCmds( m_rRenameItems );
	if ( pRenameMacroCmd.get() != nullptr )
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
	return utl::Any( m_rRenameItems, std::mem_fn( &CRenameItem::HasDestPath ) );
}

const CEnumTags& CRenameFilesDialog::GetTags_Mode( void )
{
	static const CEnumTags s_modeTags( _T("&Make Names|Rena&me|Roll &Back|Roll &Fwd") );
	return s_modeTags;
}

void CRenameFilesDialog::SwitchMode( Mode mode ) override
{
	m_mode = mode;
	if ( nullptr == m_hWnd )
		return;

	UpdateOkButton( GetTags_Mode().FormatUi( m_mode ) );

	static const UINT ctrlIds[] =
	{
		IDC_FORMAT_COMBO, IDC_STRIP_BAR_1, IDC_COPY_SOURCE_PATHS_BUTTON,
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
		if ( const CRenameItem* pFirstErrorItem = GetFirstErrorItem<CRenameItem>() )
			SetCaretOnItem( pFirstErrorItem );

		SwitchMode( EditMode );

		if ( !silent )
			sys_tray::ShowBalloonMessage( str::Format( _T("Detected duplicate file name collisions in destination:\r\n%s"), JoinErrorDestPaths().c_str() ), nullptr, app::Warning );
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

void CRenameFilesDialog::OnExecuteCmd( utl::ICommand* pCmd ) override
{
	if ( app::TargetSelectedItems == m_pFileModel->GetTargetScope() )
		if ( cmd::HasSelItemsTarget( pCmd ) )
			ui::FlashCtrlFrame( &m_targetSelItemsButton, app::ColorWarningText, 3 );

	if ( m_pFileModel == pCmd->GetSubject() )
		if ( m_pFileModel->GetRenameSorting().first != ren::RecordDefault )
			ui::FlashCtrlFrame( &m_sortOrderCombo, app::ColorWarningText, 3 );
}


void CRenameFilesDialog::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override
{
	const cmd::CommandType cmdType = static_cast<cmd::CommandType>( utl::GetSafeTypeID( pMessage ) );

	if ( nullptr == m_hWnd )
		return;

	if ( &m_filesSheet == pSubject )
	{	// This is a "foreign" update from the child sheet for cross-pages commands.
		// Do minimal handling, then return.
		if ( cmd::OnRenameListSelChanged == cmdType )
			UpdateFileListStatus();

		return;		// skip rounting sheet/page command notifications, since the child sheet has it's own obeservers
	}

	if ( m_pFileModel == pSubject )
	{
		if ( nullptr == m_pRenSvc.get() )
			m_pRenSvc.reset( new CRenameService( m_rRenameItems ) );		// lazy init
		else
			m_pRenSvc->StoreRenameItems( m_rRenameItems );					// update the current object since it may be referenced in CReplaceDialog

		if ( const CSortRenameListCmd* pSortCmd = utl::GetSafeMatchCmd<CSortRenameListCmd>( pMessage, cmd::SortRenameList ) )
		{
			UpdateSortOrderCombo( pSortCmd->m_sorting );

			if ( CommitFilesMode == m_mode && HasDestPaths() )
				ChangeSeqCount( m_prevGenSeqCount );			// allow user to re-generate the dest sequence, by switching to EditMode
		}
	}

	for ( int i = 0; i != m_filesSheet.GetPageCount(); ++i )
		if ( utl::IObserver* pPage = m_filesSheet.GetCreatedPageAs<utl::IObserver>( i ) )
			pPage->OnUpdate( pSubject, pMessage );

	if ( m_pFileModel == pSubject )
		if ( cmd::ChangeDestPaths == cmdType )
			PostMakeDest();
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
		SetCaretOnItem( pErrorItem );
	}
}

void CRenameFilesDialog::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override
{
	switch ( cmdId )
	{
		case IDC_SORT_ORDER_COMBO:
			rText = m_sortOrderCombo.GetTags()->FormatKey( m_sortOrderCombo.GetValue() );
			break;
		case IDC_TARGET_SEL_ITEMS_CHECK:
			rText = app::GetTags_TargetScope().FormatKey( m_pFileModel->GetTargetScope() );
			break;
		case IDC_CHANGE_CASE_BUTTON:
		{
			static const std::vector<std::tstring> tooltips = str::LoadStrings( IDC_CHANGE_CASE_BUTTON );
			ChangeCase changeCase = m_changeCaseButton.GetSelEnum<ChangeCase>();
			rText = tooltips.at( changeCase );
			break;
		}
		case IDC_CAPITALIZE_BUTTON:
			if ( m_capitalizeButton.GetRhsPartRect().PtInRect( ui::GetCursorPos( m_capitalizeButton ) ) )
			{
				static const std::tstring s_details = str::Load( IDI_DETAILS );
				rText = s_details;
			}
			break;
		case IDC_REPLACE_FILES_BUTTON:
			rText = CReplaceDialog::FormatTooltipMessage();
			break;
		case IDC_CURR_FOLDER_STATIC:
			rText = _T("Folder of the current file");
			break;
		case IDC_CURR_FOLDER_EDIT:
			rText = utl::GetSafeCode( m_selData.GetCaretItem() );
			break;
		case ID_CMD_RESET_DESTINATIONS:
			rText = _T("Reset to initial destination name");
			if ( !m_selData.GetSelItems().empty() )
				rText += str::Format( _T(": %d selected item(s)"), m_selData.GetSelItems().size() );
			else
				rText += _T(" (no selected items)");
			break;
		case IDOK:
			if ( !ui::IsDisabled( *GetDlgItem( IDOK ) ) )
			{
				rText = EditMode == m_mode ? _T("Generate filenames") : _T("Rename files for");

				if ( EditMode == m_mode && app::TargetSelectedItems == m_pFileModel->GetTargetScope() && !m_selData.GetSelItems().empty() )
					rText += str::Format( _T(": %d selected item(s)"), m_selData.GetSelItems().size() );
				else
					rText += _T(" all items");
			}
			break;
		default:
			__super::QueryTooltipText( rText, cmdId, pTooltip );
	}
}

void CRenameFilesDialog::UpdateFormatLabel( void )
{
	ui::SetDlgItemText( this, IDC_FORMAT_LABEL, str::Format( _T("&Template \"%s\":"), m_ignoreExtension ? _T("name") : _T("name.ext") ) );
}

void CRenameFilesDialog::UpdateFileListStatus( void )
{
	std::tstring message = str::Format( _T("Total: %d file(s) in %d folder(s)."), m_pFileModel->GetSourcePaths().size(), m_pFileModel->GetSrcFolderPaths().size() );

	if ( size_t selCount = m_selData.GetSelItems().size() )
		message += str::Format( _T("  Selected %d items(s)."), selCount );

	m_fileStatsStatic.SetWindowText( message );

	fs::CPath currFolderPath;

	if ( CRenameItem* pCaretItem = m_selData.GetCaretItem() )
		currFolderPath = pCaretItem->GetSrcPath().GetParentPath();

	m_currFolderEdit.SetText( currFolderPath.Get() );
	m_currFolderEdit.SelectAll();		// scroll to end to show deepest subfolder
}

void CRenameFilesDialog::UpdateSortOrderCombo( const ren::TSortingPair& sorting )
{
	ren::ui::UiSortBy sortBy = ren::ui::FromSortingPair( sorting );

	if ( sortBy != m_sortOrderCombo.CComboBox::GetCurSel() )
		m_sortOrderCombo.SetValue( sortBy );

	m_sortOrderCombo.SetFrameColor( ren::ui::Default == sortBy ? CLR_NONE : app::ColorWarningText );
	GetDlgItem( IDC_SORT_ORDER_STATIC )->Invalidate();
}

void CRenameFilesDialog::UpdateTargetScopeButton( void )
{
	m_targetSelItemsButton.SetFrameColor( app::TargetSelectedItems == m_pFileModel->GetTargetScope() ? app::ColorWarningText : CLR_NONE );
}

bool CRenameFilesDialog::PromptRenameCustomSortOrder( void ) const
{
	if ( m_sortOrderCombo.GetEnum<ren::ui::UiSortBy>() != ren::ui::Default )		// non-default sorting order?
	{
		static bool s_promptUser = true;
		if ( s_promptUser )
		{
			ui::CTaskDialog dlg( _T("Rename Files"),									// title
				_T("The files to be renamed are sorted\n using a custom sort order."),	// main instruction
				_T("Are you sure you want to proceed with renaming the files?"),		// content
				TDCBF_YES_BUTTON | TDCBF_NO_BUTTON );

			dlg.SetMainIcon( TD_WARNING_ICON );
			dlg.SetFooterText( _T("Renaming files using a sort order other than default (by file path) could lead to potentially accidental results.") );
			dlg.SetFooterIcon( TD_INFORMATION_ICON );
			dlg.SetVerificationText( _T("Always prompt when renaming files") );
			dlg.SetVerificationChecked( s_promptUser );

			if ( IDYES == dlg.DoModal( (CWnd*)this ) )
				s_promptUser = dlg.IsVerificationChecked() != FALSE;		// store user's choice about further prompts
			else
				return false;		// user rejected renaming the files (perhaps it was unintended?)
		}
	}

	return true;
}


void CRenameFilesDialog::CommitLocalEdits( void )
{
	m_filesSheet.GetPageAs<CRenameEditPage>( EditPage )->CommitLocalEdits();			// allow detail page edit input
}

CRenameItem* CRenameFilesDialog::FindItemWithKey( const fs::CPath& srcPath ) const
{
	return func::FindItemWithPath( m_rRenameItems, srcPath );
}

void CRenameFilesDialog::MarkInvalidSrcItems( void )
{
	for ( CRenameItem* pRenameItem: m_rRenameItems )
		if ( !pRenameItem->GetSrcPath().FileExist() )
			utl::AddUnique( m_errorItems, pRenameItem );
}

std::tstring CRenameFilesDialog::JoinErrorDestPaths( void ) const
{
	std::vector<fs::CPath> destPaths; destPaths.reserve( m_errorItems.size() );

	for ( const CPathItemBase* pErrorItem: m_errorItems )
		destPaths.push_back( checked_static_cast<const CRenameItem*>( pErrorItem )->GetDestPath() );

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

bool CRenameFilesDialog::SetCaretOnItem( const CRenameItem* pRenameItem )
{
	if ( !m_selData.SetCaretItem( pRenameItem ) )
		return false;		// no caret change

	COnRenameListSelChangedCmd( &m_filesSheet, m_selData ).Execute();		// synchronize caret across pages
	return true;
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

bool CRenameFilesDialog::GenerateDestPaths( const CPathFormatter& pathFormatter, IN OUT UINT* pSeqCount )
{
	ASSERT_PTR( pSeqCount );
	ASSERT_PTR( m_pRenSvc.get() );

	const std::vector<CRenameItem*>& targetRenameItems = GetTargetItems();

	if ( targetRenameItems.empty() )
		return false;

	ClearFileErrors();

	CPathRenamePairs renamePairs;
	ren::MakePairsFromItems( &renamePairs, targetRenameItems );

	CPathGenerator generator( &renamePairs, pathFormatter, *pSeqCount );
	if ( !generator.GeneratePairs() )
		return false;

	*pSeqCount = generator.GetSeqCount();

	std::vector<fs::CPath> newDestPaths;
	renamePairs.QueryDestPaths( std::back_inserter( newDestPaths ) );

	return SafeExecuteCmd( new CChangeDestPathsCmd( m_pFileModel, GetCmdSelItems(), newDestPaths, _T("Generate destination file paths from template format") ) );
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
	const bool firstInit = nullptr == m_filesSheet.m_hWnd;

	m_filesSheet.DDX_DetailSheet( pDX, IDC_FILES_SHEET );

	DDX_Control( pDX, IDC_FORMAT_COMBO, m_formatCombo );
	DDX_Control( pDX, IDC_SEQ_COUNT_EDIT, m_seqCountEdit );
	m_seqCountToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignLeft | V_AlignCenter );
	DDX_Control( pDX, IDC_FILES_STATIC, m_filesLabelDivider );
	DDX_Control( pDX, IDC_OUTCOME_INFO_STATUS, m_fileStatsStatic );
	DDX_Control( pDX, IDC_CURR_FOLDER_EDIT, m_currFolderEdit );
	DDX_Control( pDX, IDC_SHOW_EXTENSION_CHECK, m_showExtButton );
	DDX_Control( pDX, IDC_SORT_ORDER_COMBO, m_sortOrderCombo );
	DDX_Control( pDX, IDC_TARGET_SEL_ITEMS_CHECK, m_targetSelItemsButton );
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
		UpdateSortOrderCombo( m_pFileModel->GetRenameSorting() );
		m_targetSelItemsButton.SetCheck( app::TargetSelectedItems == m_pFileModel->GetTargetScope() );
		UpdateTargetScopeButton();
		UpdateFormatLabel();
		UpdateFileListStatus();

		m_isInitialized = true;

		if ( !HasDestPaths() && EditPage == m_filesSheet.GetActiveIndex() )
			CResetDestinationsCmd( m_pFileModel ).Execute();	// this will call OnUpdate() for all observers
		else
			OnUpdate( m_pFileModel, nullptr );					// initialize the dialog
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
	ON_BN_CLICKED( IDC_TARGET_SEL_ITEMS_CHECK, OnToggle_TargetSelItems )
	ON_COMMAND_RANGE( ID_COPY_SEL_ITEMS, ID_COPY_SEL_ITEMS, On_SelItems_Copy )
	ON_UPDATE_COMMAND_UI( ID_COPY_SEL_ITEMS, OnUpdateListSelection )
	ON_COMMAND( ID_CMD_RESET_DESTINATIONS, On_SelItems_ResetDestFile )
	ON_UPDATE_COMMAND_UI( ID_CMD_RESET_DESTINATIONS, OnUpdateListSelection )
	ON_BN_CLICKED( IDC_COPY_SOURCE_PATHS_BUTTON, OnBnClicked_CopySourceFiles )
	ON_BN_CLICKED( IDC_PASTE_FILES_BUTTON, OnBnClicked_PasteDestFiles )
	ON_BN_CLICKED( IDC_RESET_FILES_BUTTON, OnBnClicked_ResetDestFiles )
	ON_UPDATE_COMMAND_UI( IDC_RESET_FILES_BUTTON, OnUpdate_ResetDestFiles )
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

	ON_COMMAND( ID_BROWSE_FOLDER, On_BrowseCurrFolder )
	ON_UPDATE_COMMAND_UI( ID_BROWSE_FOLDER, OnUpdate_BrowseCurrFolder )
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
				if ( GetTargetItems().empty() )
				{
					ui::MessageBox( _T("No items selected!\nPlease select the items to generate."), MB_ICONWARNING | MB_OK );
					ui::FlashCtrlFrame( &m_targetSelItemsButton, app::ColorWarningText, 3 );
				}
				else
				{
					ui::MessageBox( str::Format( IDS_INVALID_FORMAT, pathFormatter.GetFormat().GetPtr() ), MB_ICONERROR | MB_OK );
					ui::TakeFocus( m_formatCombo );
					ui::FlashCtrlFrame( &m_formatCombo, app::ColorWarningText, 3 );
				}
			}
			break;
		}
		case CommitFilesMode:
			if ( PromptRenameCustomSortOrder() )
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

	if ( pWnd != nullptr )
		switch ( pWnd->GetDlgCtrlID() )
		{
			case IDC_FORMAT_LABEL:
				if ( !IsFormatExtConsistent() )
					pDC->SetTextColor( app::ColorWarningText );
				break;
			case IDC_SORT_ORDER_STATIC:
				if ( m_sortOrderCombo.GetEnum<ren::ui::UiSortBy>() != ren::ui::Default )
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
	bool validFormat = ren::FormatHasValidEffect( pathFormatter, m_rRenameItems );

	m_formatCombo.SetFrameColor( validFormat ? CLR_NONE : color::Error );
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

	OnUpdate( m_pFileModel, nullptr );
}

void CRenameFilesDialog::OnCbnSelChange_SortOrder( void )
{
	ren::TSortingPair sorting = ren::ui::ToSortingPair( m_sortOrderCombo.GetEnum<ren::ui::UiSortBy>() );

	if ( sorting != m_pFileModel->GetRenameSorting() )
		CSortRenameListCmd( m_pFileModel, nullptr, sorting ).Execute();
}

void CRenameFilesDialog::OnToggle_TargetSelItems( void )
{
	if ( m_pFileModel->SetTargetScope( BST_CHECKED == m_targetSelItemsButton.GetCheck() ? app::TargetSelectedItems : app::TargetAllItems ) )
		UpdateTargetScopeButton();
}

void CRenameFilesDialog::On_SelItems_Copy( UINT cmdId )
{
	if ( const IRenamePage* pRenamePage = dynamic_cast<const IRenamePage*>( m_filesSheet.GetActivePage() ) )
		pRenamePage->OnParentCommand( cmdId );		// route the command in the active page
}

void CRenameFilesDialog::On_SelItems_ResetDestFile( void )
{
	CommitLocalEdits();

	if ( !m_selData.GetSelItems().empty() )
		SafeExecuteCmd( CChangeDestPathsCmd::MakeResetItemsCmd( m_pFileModel, m_selData.GetSelItems() ) );
}

void CRenameFilesDialog::OnUpdateListSelection( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_selData.GetSelItems().empty() );
}

void CRenameFilesDialog::OnBnClicked_CopySourceFiles( void )
{
	if ( !CFileModel::CopyClipRenameSrcPaths( GetTargetItems(), fmt::FilenameExt, this, m_pDisplayFilenameAdapter.get() ) )
		ui::MessageBox( _T("Couldn't copy source files to clipboard!"), MB_ICONERROR | MB_OK );
}

void CRenameFilesDialog::OnBnClicked_PasteDestFiles( void )
{
	try
	{
		ClearFileErrors();
		SafeExecuteCmd( m_pFileModel->MakeClipPasteDestPathsCmd( GetTargetItems(), this, m_pDisplayFilenameAdapter.get()));
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

void CRenameFilesDialog::OnUpdate_ResetDestFiles( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( false );
}

void CRenameFilesDialog::OnBnClicked_CapitalizeDestFiles( void )
{
	CommitLocalEdits();

	CTitleCapitalizer capitalizer;
	SafeExecuteCmd( m_pFileModel->MakeChangeDestPathsCmd( func::CapitalizeWords( &capitalizer ), GetTargetItems(), _T("Title Case") ) );
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

	SafeExecuteCmd( m_pFileModel->MakeChangeDestPathsCmd( func::MakeCase( selCase ), GetTargetItems(), cmdTag ) );
}

void CRenameFilesDialog::OnBnClicked_ReplaceDestFiles( void )
{
	CReplaceDialog dlg( this, m_pRenSvc.get(), GetTargetItems(), GetSelFindWhat() );
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

	SafeExecuteCmd( m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceDelimiterSet( delimiterSet, ui::GetWindowText( &m_newDelimiterEdit ) ), GetTargetItems(),
														  str::Load( IDC_REPLACE_USER_DELIMS_BUTTON ) )
	);
}

void CRenameFilesDialog::OnFieldChanged( void )
{
	SwitchMode( EditMode );
}

void CRenameFilesDialog::OnPickFormatToken( void )
{
//DBG: OnUpdate(nullptr, nullptr);
	CMenu popupMenu;
	ui::LoadPopupMenu( &popupMenu, IDR_CONTEXT_MENU, popup::FormatPicker );

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
	ui::LoadPopupMenu( &popupMenu, IDR_CONTEXT_MENU, popup::TextTools );

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
		enable = ren::FormatHasValidEffect( pathFormatter, m_rRenameItems );
			//dbg: //enable = pathFormatter.IsValidFormat();
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
	const TCHAR* pInsertFmt = nullptr;

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

	utl::ICommand* pCmd = nullptr;
	std::tstring cmdTag = str::Load( menuId );
	const std::vector<CRenameItem*>& targetItems = GetTargetItems();

	switch ( menuId )
	{
		case ID_REPLACE_ALL_DELIMS:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceDelimiterSet( delim::GetAllDelimitersSet(), _T(" ") ), targetItems, cmdTag );
			break;
		case ID_REPLACE_UNICODE_SYMBOLS:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceMultiDelimiterSets( &text_tool::GetStdUnicodeToAnsiPairs() ), targetItems, cmdTag );
			break;
		case ID_SINGLE_WHITESPACE:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::SingleWhitespace(), targetItems, cmdTag );
			break;
		case ID_REMOVE_WHITESPACE:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::RemoveWhitespace(), targetItems, cmdTag );
			break;
		case ID_DASH_TO_SPACE:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceDelimiterSet( delim::s_dashes, _T(" ") ), targetItems, cmdTag );
			break;
		case ID_SPACE_TO_DASH:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceDelimiterSet( _T(" "), _T("-") ), targetItems, cmdTag );
			break;
		case ID_UNDERBAR_TO_SPACE:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceDelimiterSet( delim::s_underscores, _T(" ") ), targetItems, cmdTag );
			break;
		case ID_SPACE_TO_UNDERBAR:
			pCmd = m_pFileModel->MakeChangeDestPathsCmd( func::ReplaceDelimiterSet( _T(" "), _T("_") ), targetItems, cmdTag );
			break;
		default:
			ASSERT( false );
	}

	SafeExecuteCmd( pCmd );
}

void CRenameFilesDialog::OnEnsureUniformNumPadding( void )
{
	CommitLocalEdits();

	std::vector<std::tstring> destFnames;
	ren::QueryDestFnames( destFnames, GetTargetItems() );

	num::EnsureUniformZeroPadding( destFnames );

	SafeExecuteCmd( m_pFileModel->MakeChangeDestPathsCmd( func::AssignFname( destFnames.begin() ), GetTargetItems(), str::Load( ID_ENSURE_UNIFORM_ZERO_PADDING ) ) );
}

void CRenameFilesDialog::OnGenerateNumericSequence( void )
{
	CommitLocalEdits();

	std::vector<std::tstring> destFnames;
	ren::QueryDestFnames( destFnames, GetTargetItems() );

	try
	{
		num::GenerateNumbersInSequence( destFnames );

		SafeExecuteCmd( m_pFileModel->MakeChangeDestPathsCmd( func::AssignFname( destFnames.begin() ), GetTargetItems(), str::Load( ID_GENERATE_NUMERIC_SEQUENCE ) ) );
	}
	catch ( CRuntimeException& exc )
	{
		exc.ReportError();
	}
}

void CRenameFilesDialog::On_BrowseCurrFolder( void )
{
	fs::CPath caretFilePath;

	if ( CRenameItem* pCaretItem = m_selData.GetCaretItem() )
		caretFilePath = pCaretItem->GetSrcPath();

	shell::BrowseForFile( caretFilePath, this );
}

void CRenameFilesDialog::OnUpdate_BrowseCurrFolder( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_selData.GetCaretItem() != nullptr );
}
