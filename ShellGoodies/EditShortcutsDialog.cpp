
#include "pch.h"
#include "EditShortcutsDialog.h"
#include "EditLinkItem.h"
#include "FileModel.h"
#include "FileService.h"
#include "EditingCommands.h"
#include "GeneralOptions.h"
#include "Application.h"
#include "AppCommands.h"
#include "utl/Command.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/LongestCommonSubsequence.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include "utl/TimeUtils.h"
#include "utl/TextClipboard.h"
#include "utl/UI/Color.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/WndUtilsEx.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/ReportListControl.hxx"
#include "utl/UI/SelectionData.hxx"


namespace reg
{
	static const TCHAR section_dialog[] = _T("EditShortcutsDialog");
}

namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_TARGET_PATH_EDIT, SizeX },
		{ IDC_ARGUMENTS_EDIT, SizeX },
		{ IDC_WORK_DIR_EDIT, SizeX },
		{ IDC_DESCRIPTION_EDIT, SizeX },

		{ IDC_FILES_STATIC, SizeX },
		{ IDC_EDIT_SHORTCUTS_LIST, Size },
		{ IDC_SHOW_SRC_COLUMNS_CHECK, MoveX },
		{ IDC_OUTCOME_INFO_STATUS, MoveY | SizeX },
		{ IDC_CURR_FOLDER_STATIC, MoveY },
		{ IDC_CURR_FOLDER_EDIT, MoveY | SizeX },

		{ IDC_COPY_SOURCE_PATHS_BUTTON, MoveY },
		{ IDC_PASTE_FILES_BUTTON, MoveY },
		{ IDC_RESET_FILES_BUTTON, MoveY },

		{ IDOK, MoveX },
		{ IDCANCEL, MoveX },
		{ IDC_TOOLBAR_PLACEHOLDER, MoveX }
	};
}

CEditShortcutsDialog::CEditShortcutsDialog( CFileModel* pFileModel, CWnd* pParent )
	: CFileEditorBaseDialog( pFileModel, cmd::EditShortcut, IDD_EDIT_SHORTCUTS_DIALOG, pParent )
	, m_rEditLinkItems( m_pFileModel->LazyInitEditLinkItems() )
	, m_fileListCtrl( IDC_EDIT_SHORTCUTS_LIST )
	, m_dirty( false )
	, m_filesLabelDivider( CLabelDivider::Instruction )
	, m_fileStatsStatic( ui::EditShrinkHost_MateOnRight )
	, m_showCmdCombo( &fmt::GetTags_ShowCmd() )
{
	m_nativeCmdTypes.push_back( cmd::ResetDestinations );
	m_mode = CommitFilesMode;
	REQUIRE( !m_rEditLinkItems.empty() );

	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );
	LoadDlgIcon( ID_TOUCH_FILES );

	//m_fileListCtrl.ModifyListStyleEx( 0, LVS_EX_GRIDLINES );
	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );
	m_fileListCtrl.SetUseAlternateRowColoring();
	m_fileListCtrl.SetTextEffectCallback( this );
//	m_fileListCtrl.SetPopupMenu( CReportListControl::OnSelection, ... );	// let us track a custom menu
//	m_fileListCtrl.SetTrackMenuTarget( this );		// firstly handle our custom commands in this dialog
	m_fileListCtrl.SetFormatTableFlags( lv::SelRowsDisplayVisibleColumns );		// copy table as selected rows, using visible columns in display order
	CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );

	m_fileListCtrl.AddRecordCompare( pred::NewComparator( pred::TCompareCode() ) );		// default row item comparator
	m_fileListCtrl.AddColumnCompare( LinkName, pred::NewComparator( pred::TCompareDisplayCode() ) );
	//m_fileListCtrl.AddColumnCompare( D_Target, pred::NewPropertyComparator<CEditLinkItem>( func::Dest_AsTargetPath() ) );
	//m_fileListCtrl.AddColumnCompare( D_Folder, pred::NewPropertyComparator<CEditLinkItem>( func::Dest_AsWorkDirPath() ) );
	//m_fileListCtrl.AddColumnCompare( D_Arguments, pred::NewPropertyComparator<CEditLinkItem>( func::Dest_AsArguments() ) );
	//m_fileListCtrl.AddColumnCompare( D_IconLocation, pred::NewPropertyComparator<CEditLinkItem>( func::Dest_AsIconLocation() ) );
	//m_fileListCtrl.AddColumnCompare( HotKey, pred::NewPropertyComparator<CEditLinkItem>( func::Dest_AsHotKey() ) );
	//m_fileListCtrl.AddColumnCompare( ShowCmd, pred::NewPropertyComparator<CEditLinkItem>( func::Dest_AsShowCmd() ) );
	//m_fileListCtrl.AddColumnCompare( D_Description, pred::NewPropertyComparator<CEditLinkItem>( func::Dest_AsDescription() ) );

	static const TCHAR s_mixedFormat[] = _T("'(multiple values)'");

	m_fileStatsStatic.GetMateToolbar()->GetStrip()
		.AddButton( ID_EDIT_COPY )
		.AddButton( ID_CMD_RESET_DESTINATIONS );

	m_targetPathEdit.GetMateToolbar()->GetStrip()
		.AddButton( ID_EDIT_COPY )
		.AddButton( ID_BROWSE_FOLDER );

	m_workDirEdit.SetUseDirPath( true );
	m_workDirEdit.GetMateToolbar()->GetStrip()
		.AddButton( ID_EDIT_COPY )
		.AddButton( ID_BROWSE_FOLDER );

	m_currFolderEdit.SetUseFixedFont( false );
	m_currFolderEdit.SetUseDirPath( true );
	m_currFolderEdit.GetMateToolbar()->GetStrip()
		.AddButton( ID_EDIT_COPY )
		.AddButton( ID_BROWSE_FOLDER );

	m_targetSelItemsButton.SetFrameMargins( -3, -3 );	// draw the frame outside of the button, in the dialog area
}

CEditShortcutsDialog::~CEditShortcutsDialog()
{
}

const std::vector<CEditLinkItem*>* CEditShortcutsDialog::GetCmdSelItems( void ) const
{
	return app::TargetSelectedItems == m_pFileModel->GetTargetScope() && !m_selData.GetSelItems().empty() ? &m_selData.GetSelItems() : nullptr;
}

const std::vector<CEditLinkItem*>& CEditShortcutsDialog::GetTargetItems( void ) const
{
	if ( const std::vector<CEditLinkItem*>* pSelItems = GetCmdSelItems() )
		return *pSelItems;

	return m_rEditLinkItems;
}

bool CEditShortcutsDialog::ModifyShortcuts( void )
{
	CFileService svc;
	std::auto_ptr<CMacroCommand> pEditLinkMacroCmd = svc.MakeEditLinkCmds( m_rEditLinkItems );

	if ( pEditLinkMacroCmd.get() != nullptr )
		if ( !pEditLinkMacroCmd->IsEmpty() )
		{
			ClearFileErrors();

			cmd::CScopedErrorObserver observe( this );
			return SafeExecuteCmd( pEditLinkMacroCmd.release() );
		}
		else
			return PromptCloseDialog();

	return false;
}

const CEnumTags& CEditShortcutsDialog::GetTags_Mode( void )
{
	static const CEnumTags s_modeTags( _T("&Store|&Touch|Roll &Back|Roll &Fwd") );
	return s_modeTags;
}

void CEditShortcutsDialog::SwitchMode( Mode mode )
{
	m_mode = mode;
	if ( nullptr == m_hWnd )
		return;

	UpdateOkButton( GetTags_Mode().FormatUi( m_mode ) );

	static const UINT ctrlIds[] =
	{
		IDC_COPY_SOURCE_PATHS_BUTTON, IDC_PASTE_FILES_BUTTON, IDC_RESET_FILES_BUTTON/*,
		IDC_MODIFIED_DATE, IDC_CREATED_DATE, IDC_ACCESSED_DATE,
		IDC_ATTRIB_READONLY_CHECK, IDC_ATTRIB_HIDDEN_CHECK, IDC_ATTRIB_SYSTEM_CHECK, IDC_ATTRIB_ARCHIVE_CHECK*/
	};
	ui::EnableControls( *this, ctrlIds, COUNT_OF( ctrlIds ), !IsRollMode() );

	m_dirty = utl::Any( m_rEditLinkItems, std::mem_fn( &CEditLinkItem::IsModified ) );
	ui::EnableControl( *this, IDOK, m_mode != CommitFilesMode || m_dirty );

	m_fileListCtrl.Invalidate();			// do some custom draw magic
}

void CEditShortcutsDialog::PostMakeDest( bool silent /*= false*/ ) override
{
	if ( !silent )
		GotoDlgCtrl( GetDlgItem( IDOK ) );

	EnsureVisibleFirstError();
	SwitchMode( CommitFilesMode );
}

void CEditShortcutsDialog::PopStackTop( svc::StackType stackType ) override
{
	ASSERT( !IsRollMode() );

	if ( utl::ICommand* pTopCmd = PeekCmdForDialog( stackType ) )		// comand that is target for this dialog editor?
	{
		bool isTouchMacro = cmd::EditShortcut == pTopCmd->GetTypeID();

		ClearFileErrors();

		if ( isTouchMacro )
			m_pFileModel->FetchFromStack( stackType );		// fetch dataset from the stack top macro command
		else
			m_pCmdSvc->UndoRedo( stackType );

		MarkInvalidSrcItems();

		if ( isTouchMacro )						// file command?
			SwitchMode( svc::Undo == stackType ? RollBackMode : RollForwardMode );
		else if ( IsNativeCmd( pTopCmd ) )		// file state editing command?
			SwitchMode( CommitFilesMode );
	}
	else
		PopStackRunCrossEditor( stackType );	// end this dialog and execute the target dialog editor
}

void CEditShortcutsDialog::OnExecuteCmd( utl::ICommand* pCmd )
{
	if ( app::TargetSelectedItems == m_pFileModel->GetTargetScope() )
		if ( cmd::HasSelItemsTarget( pCmd ) )
			ui::FlashCtrlFrame( &m_targetSelItemsButton, app::ColorWarningText, 3 );
}


void CEditShortcutsDialog::SetupDialog( void )
{
	SetupFileListView();							// fill in and select the found files list
	UpdateFieldControls();
}

void CEditShortcutsDialog::UpdateTargetScopeButton( void )
{
	m_targetSelItemsButton.SetFrameColor( app::TargetSelectedItems == m_pFileModel->GetTargetScope() ? app::ColorWarningText : CLR_NONE );
}

void CEditShortcutsDialog::UpdateFileListStatus( void )
{
	std::tstring message = str::Format( _T("Total: %d file(s) in %d folder(s)."), m_pFileModel->GetSourcePaths().size(), m_pFileModel->GetSrcFolderPaths().size() );

	if ( size_t selCount = m_selData.GetSelItems().size() )
		message += str::Format( _T("  Selected %d items(s)."), selCount );

	m_fileStatsStatic.SetWindowText( message );

	fs::CPath currFolderPath;

	if ( CEditLinkItem* pCaretItem = m_selData.GetCaretItem() )
		currFolderPath = pCaretItem->GetFilePath().GetParentPath();

	m_currFolderEdit.SetShellPath( currFolderPath );
}

void CEditShortcutsDialog::SetupFileListView( void )
{
	CScopedInternalChange internalChange( &m_fileListCtrl );
	lv::TScopedStatus_ByObject sel( &m_fileListCtrl );

	CScopedLockRedraw freeze( &m_fileListCtrl );

	m_fileListCtrl.DeleteAllItems();

	for ( unsigned int pos = 0; pos != m_rEditLinkItems.size(); ++pos )
	{
		const CEditLinkItem* pLinkItem = m_rEditLinkItems[pos];

		m_fileListCtrl.InsertObjectItem( pos, pLinkItem );		// LinkName

		{
			const shell::CShortcut& destShortcut = pLinkItem->GetDestShortcut();

			m_fileListCtrl.SetSubItemText( pos, D_Target, fmt::FormatTarget( destShortcut ) );
			m_fileListCtrl.SetSubItemText( pos, D_Folder, destShortcut.GetWorkDirPath().Get() );
			m_fileListCtrl.SetSubItemText( pos, D_Arguments, destShortcut.GetArguments() );
			m_fileListCtrl.SetSubItemText( pos, D_IconLocation, fmt::FormatIconLocation( destShortcut ) );
			m_fileListCtrl.SetSubItemText( pos, D_HotKey, fmt::FormatHotKey( destShortcut.GetHotKey() ) );
			m_fileListCtrl.SetSubItemText( pos, D_ShowCmd, fmt::FormatShowCmd( destShortcut ) );
			m_fileListCtrl.SetSubItemText( pos, D_Description, destShortcut.GetDescription() );
		}
		{
			const shell::CShortcut& srcShortcut = pLinkItem->GetDestShortcut();

			m_fileListCtrl.SetSubItemText( pos, S_Target, fmt::FormatTarget( srcShortcut ) );
			m_fileListCtrl.SetSubItemText( pos, S_Folder, srcShortcut.GetWorkDirPath().Get() );
			m_fileListCtrl.SetSubItemText( pos, S_Arguments, srcShortcut.GetArguments() );
			m_fileListCtrl.SetSubItemText( pos, S_IconLocation, fmt::FormatIconLocation( srcShortcut ) );
			m_fileListCtrl.SetSubItemText( pos, S_HotKey, fmt::FormatHotKey( srcShortcut.GetHotKey() ) );
			m_fileListCtrl.SetSubItemText( pos, S_ShowCmd, fmt::FormatShowCmd( srcShortcut ) );
			m_fileListCtrl.SetSubItemText( pos, S_Description, srcShortcut.GetDescription() );
		}
	}

	str::TGetMatch getMatchFunc;

	m_fileListCtrl.SetupDiffColumnPair( S_Target, D_Target, getMatchFunc );
	m_fileListCtrl.SetupDiffColumnPair( S_Folder, D_Folder, getMatchFunc );
	m_fileListCtrl.SetupDiffColumnPair( S_Arguments, D_Arguments, getMatchFunc );
	m_fileListCtrl.SetupDiffColumnPair( S_IconLocation, D_IconLocation, getMatchFunc );
	m_fileListCtrl.SetupDiffColumnPair( S_HotKey, D_HotKey, getMatchFunc );
	m_fileListCtrl.SetupDiffColumnPair( S_ShowCmd, D_ShowCmd, getMatchFunc );
	m_fileListCtrl.SetupDiffColumnPair( S_Description, D_Description, getMatchFunc );

	m_fileListCtrl.InitialSortList();		// store original order and sort by current criteria
}

void CEditShortcutsDialog::UpdateFileListViewSelItems( void )
{
	path::TGetMatch getMatchFunc;

	for ( CEditLinkItem* pLinkItem: m_selData.GetSelItems() )
	{
		const shell::CShortcut& destShortcut = pLinkItem->GetDestShortcut();
		int pos = m_fileListCtrl.FindItemIndex( pLinkItem );
		ASSERT( pos != -1 );

		m_fileListCtrl.SetSubItemText( pos, D_Target, fmt::FormatTarget( destShortcut ) );
		m_fileListCtrl.SetSubItemText( pos, D_Folder, destShortcut.GetWorkDirPath().Get() );
		m_fileListCtrl.SetSubItemText( pos, D_Arguments, destShortcut.GetWorkDirPath().Get() );
		m_fileListCtrl.SetSubItemText( pos, D_IconLocation, fmt::FormatIconLocation( destShortcut ) );
		m_fileListCtrl.SetSubItemText( pos, D_HotKey, fmt::FormatHotKey( destShortcut.GetHotKey() ) );
		m_fileListCtrl.SetSubItemText( pos, D_ShowCmd, fmt::FormatShowCmd( destShortcut ) );
		m_fileListCtrl.SetSubItemText( pos, D_Description, destShortcut.GetDescription() );

		// IMP: update the Dest side of DiffColumn pairs!
		m_fileListCtrl.UpdateItemDiffColumnDest( pos, D_Target, getMatchFunc );
		m_fileListCtrl.UpdateItemDiffColumnDest( pos, D_Folder, getMatchFunc );
		m_fileListCtrl.UpdateItemDiffColumnDest( pos, D_Arguments, getMatchFunc );
		m_fileListCtrl.UpdateItemDiffColumnDest( pos, D_IconLocation, getMatchFunc );
		m_fileListCtrl.UpdateItemDiffColumnDest( pos, D_HotKey, getMatchFunc );
		m_fileListCtrl.UpdateItemDiffColumnDest( pos, D_ShowCmd, getMatchFunc );
		m_fileListCtrl.UpdateItemDiffColumnDest( pos, D_Description, getMatchFunc );
	}
}

bool CEditShortcutsDialog::UpdateDetailFields( void )
{
	CEditLinkItem* pCaretItem = m_selData.GetCaretItem();
	if ( nullptr == pCaretItem )
		return false;

	const shell::CShortcut& destShortcut = pCaretItem->GetDestShortcut();

	if ( destShortcut.IsTargetNonFileSys() )
		m_targetPathEdit.SetPidl( destShortcut.GetTargetPidl() );
	else
		m_targetPathEdit.SetShellPath( destShortcut.GetTargetPath() );

	m_targetPathEdit.SetReadOnly( destShortcut.IsTargetNonFileSys() );

	m_argumentsEdit.SetText( destShortcut.GetArguments() );
	m_workDirEdit.SetShellPath( destShortcut.GetWorkDirPath() );
	m_descriptionEdit.SetText( destShortcut.GetDescription() );
	WORD hotKey = destShortcut.GetHotKey();
	m_hotKeyCtrl.SetHotKey( LOBYTE( hotKey ), HIBYTE( hotKey ) );
	m_showCmdCombo.SetValue( destShortcut.GetShowCmd() );

	return true;
}

void CEditShortcutsDialog::UpdateFieldControls( void )
{
	// commit common values to field controls
/*	for ( const multi::CDateTimeState& dateTimeState: m_dateTimeStates )
		dateTimeState.UpdateCtrl( this );

	for ( const multi::CAttribCheckState& attribState: m_attribCheckStates )
		attribState.UpdateCtrl( this );*/
}

void CEditShortcutsDialog::UpdateFieldsFromCaretItem( void )
{
	if ( const CEditLinkItem* pCaretItem = m_selData.GetCaretItem() )
	{
	}
	else
	{
	}

	UpdateFieldControls();
}

void CEditShortcutsDialog::InputFields( void )
{
/*	for ( multi::CDateTimeState& rDateTimeState: m_dateTimeStates )
		rDateTimeState.InputCtrl( this );

	for ( multi::CAttribCheckState& rAttribState: m_attribCheckStates )
		rAttribState.InputCtrl( this );*/
}

void CEditShortcutsDialog::ApplyFields( void )
{
	if ( utl::ICommand* pCmd = MakeChangeDestShortcutsCmd() )
	{
		ClearFileErrors();
		SafeExecuteCmd( pCmd );
	}
}

utl::ICommand* CEditShortcutsDialog::MakeChangeDestShortcutsCmd( void )
{
	const std::vector<CEditLinkItem*>& targetItems = GetTargetItems();
	std::vector<shell::CShortcut> destShortcuts; destShortcuts.reserve( m_rEditLinkItems.size() );
	bool anyChange = false;

	// apply valid edits, i.e. if not null
	for ( const CEditLinkItem* pLinkItem: targetItems )
	{
/*		shell::CShortcut newFileState = pLinkItem->GetDestState();

		for ( const multi::CDateTimeState& dateTimeState: m_dateTimeStates )
			if ( dateTimeState.CanApply() )
			{
				dateTimeState.Apply( newFileState );
			}

		for ( const multi::CAttribCheckState& attribState: m_attribCheckStates )
			if ( attribState.CanApply() )
			{
				attribState.Apply( newFileState );
			}

		destShortcuts.push_back( newFileState );

		anyChange |= newFileState != pLinkItem->GetDestState();*/
	}

	return anyChange ? new CChangeDestShortcutsCmd( m_pFileModel, &targetItems, destShortcuts ) : nullptr;
}

bool CEditShortcutsDialog::VisibleAllSrcColumns( void ) const
{
	return m_fileListCtrl.IsColumnRangeVisible( S_Target, S_Description );
}

void CEditShortcutsDialog::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override
{
	const cmd::CommandType cmdType = static_cast<cmd::CommandType>( utl::GetSafeTypeID( pMessage ) ); cmdType;

	if ( nullptr == m_hWnd )
		return;

	if ( m_pFileModel == pSubject )
	{
		if ( nullptr == pMessage )
			SetupDialog();		// initial dlg setup
		else if ( const CChangeDestShortcutsCmd* pCmd = utl::GetSafeMatchCmd<CChangeDestShortcutsCmd>( pMessage, cmd::ChangeDestShortcuts ) )
		{
			if ( pCmd->HasSelItems() )
				UpdateFileListViewSelItems();
			else
				SetupDialog();

			PostMakeDest();
		}
		else
			ASSERT( false );		// any other command to handle?
	}
	else if ( &CGeneralOptions::Instance() == pSubject )
		CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );
}

void CEditShortcutsDialog::ClearFileErrors( void ) override
{
	m_errorItems.clear();

	if ( m_hWnd != nullptr )
		m_fileListCtrl.Invalidate();
}

void CEditShortcutsDialog::OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg ) override
{
	errMsg;

	if ( CEditLinkItem* pErrorItem = FindItemWithKey( srcPath ) )
		utl::AddUnique( m_errorItems, pErrorItem );

	EnsureVisibleFirstError();
}

void CEditShortcutsDialog::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override
{
	switch ( cmdId )
	{
		case IDC_TARGET_SEL_ITEMS_CHECK:
			rText = app::GetTags_TargetScope().FormatKey( m_pFileModel->GetTargetScope() );
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
				rText = EditMode == m_mode ? _T("Store changes to") : _T("Touch file states for");

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

void CEditShortcutsDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const override
{
	pCtrl;
	static const ui::CTextEffect s_modLinkName( ui::Bold );
	static const ui::CTextEffect s_modDest( ui::Regular, CReportListControl::s_mismatchDestTextColor );
	static const ui::CTextEffect s_modSrc( ui::Regular, CReportListControl::s_deleteSrcTextColor );
	static const ui::CTextEffect s_errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );

	const CEditLinkItem* pLinkItem = CReportListControl::AsPtr<CEditLinkItem>( rowKey );
	shell::CShortcut::TFields dirtyFields = pLinkItem->GetDestShortcut().GetDiffFields( pLinkItem->GetSrcShortcut() );

	const ui::CTextEffect* pTextEffect = nullptr;
	bool isModified = false, isSrc = false;

	switch ( subItem )
	{
		case LinkName:
			isModified = pLinkItem->IsModified();
			if ( isModified )
				pTextEffect = &s_modLinkName;
			break;
		case S_Target:
			isSrc = true;		// fall-through
		case D_Target:
			isModified = HasFlag( dirtyFields, shell::CShortcut::TargetPath || shell::CShortcut::TargetPidl );
			break;
		case S_Folder:
			isSrc = true;		// fall-through
		case D_Folder:
			isModified = HasFlag( dirtyFields, shell::CShortcut::WorkDirPath );
			break;
		case S_Arguments:
			isSrc = true;		// fall-through
		case D_Arguments:
			isModified = HasFlag( dirtyFields, shell::CShortcut::Arguments );
			break;
		case S_IconLocation:
			isSrc = true;		// fall-through
		case D_IconLocation:
			isModified = HasFlag( dirtyFields, shell::CShortcut::IconLocation );
			break;
		case S_HotKey:
			isSrc = true;		// fall-through
		case D_HotKey:
			isModified = HasFlag( dirtyFields, shell::CShortcut::HotKey );
			break;
		case S_ShowCmd:
			isSrc = true;		// fall-through
		case D_ShowCmd:
			isModified = HasFlag( dirtyFields, shell::CShortcut::ShowCmd );
			break;
		case S_Description:
			isSrc = true;		// fall-through
		case D_Description:
			isModified = HasFlag( dirtyFields, shell::CShortcut::Description );
			break;
	}

	if ( utl::Contains( m_errorItems, pLinkItem ) )
		rTextEffect |= s_errorBk;		// highlight error row background

	if ( pTextEffect != nullptr )
		rTextEffect |= *pTextEffect;
	else if ( isModified )
		rTextEffect |= isSrc ? s_modSrc : s_modDest;

	if ( EditMode == m_mode )
	{
		bool wouldModify = false;
		/*
		switch ( subItem )
		{
			case DestAttributes:
				wouldModify = pLinkItem->GetDestShortcut().m_attributes != multi::EvalWouldBeAttributes( m_attribCheckShortcuts, pLinkItem );
				break;
			case DestModifyTime:
				wouldModify = m_dateTimeShortcuts[ fs::ModifiedDate ].WouldModify( pLinkItem );
				break;
			case DestCreationTime:
				wouldModify = m_dateTimeShortcuts[ fs::CreatedDate ].WouldModify( pLinkItem );
				break;
			case DestAccessTime:
				wouldModify = m_dateTimeShortcuts[ fs::AccessedDate ].WouldModify( pLinkItem );
				break;
		}*/

		if ( wouldModify )
			rTextEffect.m_textColor = ui::GetBlendedColor( rTextEffect.m_textColor != CLR_NONE ? rTextEffect.m_textColor : m_fileListCtrl.GetActualTextColor(), color::White );		// blend to gray
	}
}

void CEditShortcutsDialog::ModifyDiffTextEffectAt( lv::CMatchEffects& rEffects, LPARAM rowKey, int subItem, CReportListControl* pCtrl ) const override
{
	rowKey, pCtrl;
	if ( LinkName == subItem )
		return;

	ClearFlag( rEffects.m_rNotEqual.m_fontEffect, ui::Bold );		// line-up field columns nicely
	//rEffects.m_rNotEqual.m_frameFillTraits.Init( color::Green, 20, 10 );
}

CEditLinkItem* CEditShortcutsDialog::FindItemWithKey( const fs::CPath& keyPath ) const
{
	std::vector<CEditLinkItem*>::const_iterator itFoundItem = utl::BinaryFind( m_rEditLinkItems, keyPath, CPathItemBase::ToFilePath() );
	return itFoundItem != m_rEditLinkItems.end() ? *itFoundItem : nullptr;
}

void CEditShortcutsDialog::MarkInvalidSrcItems( void )
{
	for ( CEditLinkItem* pTouchItem: m_rEditLinkItems )
		if ( !pTouchItem->GetFilePath().FileExist() )
			utl::AddUnique( m_errorItems, pTouchItem );
}

void CEditShortcutsDialog::EnsureVisibleFirstError( void )
{
	if ( const CEditLinkItem* pFirstErrorItem = GetFirstErrorItem<CEditLinkItem>() )
		m_fileListCtrl.EnsureVisibleObject( pFirstErrorItem );

	m_fileListCtrl.Invalidate();				// trigger some highlighting
}

BOOL CEditShortcutsDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		m_fileListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CEditShortcutsDialog::DoDataExchange( CDataExchange* pDX )
{
	const bool firstInit = nullptr == m_fileListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_EDIT_SHORTCUTS_LIST, m_fileListCtrl );

	DDX_Control( pDX, IDC_TARGET_PATH_EDIT, m_targetPathEdit );
	DDX_Control( pDX, IDC_ARGUMENTS_EDIT, m_argumentsEdit );
	DDX_Control( pDX, IDC_WORK_DIR_EDIT, m_workDirEdit );
	DDX_Control( pDX, IDC_DESCRIPTION_EDIT, m_descriptionEdit );
	DDX_Control( pDX, IDC_HOT_KEY_CTRL, m_hotKeyCtrl );
	DDX_Control( pDX, IDC_SHOW_CMD_COMBO, m_showCmdCombo );
	// IDC_RUN_AS_ADMIN_CHECK, IDC_RUN_IN_SEPARATE_MEM_SPACE_CHECK

	DDX_Control( pDX, IDC_FILES_STATIC, m_filesLabelDivider );
	DDX_Control( pDX, IDC_OUTCOME_INFO_STATUS, m_fileStatsStatic );
	DDX_Control( pDX, IDC_CURR_FOLDER_EDIT, m_currFolderEdit );
	DDX_Control( pDX, IDC_TARGET_SEL_ITEMS_CHECK, m_targetSelItemsButton );

	ui::DDX_ButtonIcon( pDX, IDC_COPY_SOURCE_PATHS_BUTTON, ID_EDIT_COPY );
	ui::DDX_ButtonIcon( pDX, IDC_PASTE_FILES_BUTTON, ID_EDIT_PASTE );
	ui::DDX_ButtonIcon( pDX, IDC_RESET_FILES_BUTTON, ID_RESET_DEFAULT );

	if ( firstInit )
	{
		OnUpdate( m_pFileModel, nullptr );
		CheckDlgButton( IDC_SHOW_SRC_COLUMNS_CHECK, VisibleAllSrcColumns() );

		m_targetSelItemsButton.SetCheck( app::TargetSelectedItems == m_pFileModel->GetTargetScope() );
		UpdateTargetScopeButton();
		UpdateFileListStatus();
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CEditShortcutsDialog, CFileEditorBaseDialog )
	ON_UPDATE_COMMAND_UI_RANGE( IDC_UNDO_BUTTON, IDC_REDO_BUTTON, OnUpdateUndoRedo )
	ON_BN_CLICKED( IDC_TARGET_SEL_ITEMS_CHECK, OnToggle_TargetSelItems )
	ON_COMMAND( ID_CMD_RESET_DESTINATIONS, On_SelItems_ResetDestFile )
	ON_UPDATE_COMMAND_UI( ID_CMD_RESET_DESTINATIONS, OnUpdateListSelection )

	ON_BN_CLICKED( IDC_COPY_SOURCE_PATHS_BUTTON, OnBnClicked_CopySourceFiles )
	ON_BN_CLICKED( IDC_PASTE_FILES_BUTTON, OnBnClicked_PasteDestShortcuts )
	ON_BN_CLICKED( IDC_RESET_FILES_BUTTON, OnBnClicked_ResetDestFiles )
	ON_BN_CLICKED( IDC_SHOW_SRC_COLUMNS_CHECK, OnBnClicked_ShowSrcColumns )
	ON_UPDATE_COMMAND_UI( ID_PUSH_ATTRIBUTE_FIELDS, OnUpdateListSelection )
	ON_UPDATE_COMMAND_UI( ID_PUSH_ALL_FIELDS, OnUpdateListCaretItem )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_EDIT_SHORTCUTS_LIST, OnLvnItemChanged_LinkList )
	ON_NOTIFY( lv::LVN_CopyTableText, IDC_EDIT_SHORTCUTS_LIST, OnLvnCopyTableText_LinkList )
END_MESSAGE_MAP()

void CEditShortcutsDialog::OnOK( void )
{
	switch ( m_mode )
	{
		case EditMode:
			InputFields();
			ApplyFields();
			PostMakeDest();
			break;
		case CommitFilesMode:
			if ( ModifyShortcuts() )
				__super::OnOK();
			else
				SwitchMode( CommitFilesMode );
			break;
		case RollBackMode:
		case RollForwardMode:
		{
			cmd::CScopedErrorObserver observe( this );

			if ( m_pCmdSvc->UndoRedo( RollBackMode == m_mode ? svc::Undo : svc::Redo )
				 || PromptCloseDialog( PromptClose ) )
				__super::OnOK();
			else
				SwitchMode( EditMode );
			break;
		}
	}
}

void CEditShortcutsDialog::OnUpdateUndoRedo( CCmdUI* pCmdUI )
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

void CEditShortcutsDialog::OnFieldChanged( void )
{
	SwitchMode( EditMode );
}

void CEditShortcutsDialog::OnToggle_TargetSelItems( void )
{
	if ( m_pFileModel->SetTargetScope( BST_CHECKED == m_targetSelItemsButton.GetCheck() ? app::TargetSelectedItems : app::TargetAllItems ) )
		UpdateTargetScopeButton();
}

void CEditShortcutsDialog::On_SelItems_ResetDestFile( void )
{
	if ( !m_selData.GetSelItems().empty() )
		SafeExecuteCmd( CChangeDestShortcutsCmd::MakeResetItemsCmd( m_pFileModel, m_selData.GetSelItems() ) );
}


void CEditShortcutsDialog::OnBnClicked_CopySourceFiles( void )
{
	if ( !CFileModel::CopyClipSourceShortcuts( this, GetTargetItems(), m_fileListCtrl.GetSubjectAdapter() ) )
		AfxMessageBox( _T("Cannot copy source file states to clipboard!"), MB_ICONERROR | MB_OK );
}

void CEditShortcutsDialog::OnBnClicked_PasteDestShortcuts( void )
{
	try
	{
		ClearFileErrors();
		SafeExecuteCmd( m_pFileModel->MakeClipPasteDestShortcutsCmd( GetTargetItems(), this ) );
	}
	catch ( CRuntimeException& exc )
	{
		exc.ReportError();
	}
}

void CEditShortcutsDialog::OnBnClicked_ResetDestFiles( void )
{
	ClearFileErrors();
	SafeExecuteCmd( new CResetDestinationsMacroCmd( m_pFileModel ) );
}

void CEditShortcutsDialog::OnBnClicked_ShowSrcColumns( void )
{
	if ( VisibleAllSrcColumns() )
	{
		for ( CListTraits::TColumn column = S_Target; column <= S_Description; ++column )
			m_fileListCtrl.ShowColumn( column, false );		// hide column
	}
	else
		m_fileListCtrl.ResetColumnLayout();					// show all columns with default layout
}

void CEditShortcutsDialog::OnUpdateListCaretItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_selData.GetCaretItem() != nullptr );
}

void CEditShortcutsDialog::OnUpdateListSelection( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_selData.GetSelItems().empty() );
}

void CEditShortcutsDialog::OnLvnItemChanged_LinkList( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;

	ASSERT( !m_fileListCtrl.IsInternalChange() );		// filtered internally by CReportListControl
	if ( m_fileListCtrl.IsSelectionCaretChangeNotify( pNmList ) )
	{
		CReportListControl::TraceNotify( pNmList );

		if ( m_selData.ReadList( &m_fileListCtrl ) )
		{
			UpdateFileListStatus();
			UpdateDetailFields();
		}
	}

	*pResult = 0;
}

void CEditShortcutsDialog::OnLvnCopyTableText_LinkList( NMHDR* pNmHdr, LRESULT* pResult )
{
	lv::CNmCopyTableText* pNmInfo = (lv::CNmCopyTableText*)pNmHdr;
	*pResult = 0L;		// continue default handling

	if ( ui::IsKeyPressed( VK_SHIFT ) )
		pNmInfo->m_textRows.push_back( pNmInfo->m_pColumnSet->FormatHeaderRow() );		// copy header row if SHIFT is pressed
}
