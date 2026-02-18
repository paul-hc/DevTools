
#include "pch.h"
#include "TouchFilesDialog.h"
#include "TouchItem.h"
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
	static const TCHAR section_dialog[] = _T("TouchDialog");
}

namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_FILES_STATIC, SizeX },
		{ IDC_FILE_TOUCH_LIST, Size },
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

CTouchFilesDialog::CTouchFilesDialog( CFileModel* pFileModel, CWnd* pParent )
	: CFileEditorBaseDialog( pFileModel, cmd::TouchFile, IDD_TOUCH_FILES_DIALOG, pParent )
	, m_rTouchItems( m_pFileModel->LazyInitTouchItems() )
	, m_fileListCtrl( IDC_FILE_TOUCH_LIST )
	, m_dirtyTouch( false )
	, m_filesLabelDivider( CLabelDivider::Instruction )
	, m_fileStatsStatic( ui::EditShrinkHost_MateOnRight )
{
	Construct();
	m_nativeCmdTypes.push_back( cmd::ResetDestinations );
	m_mode = CommitFilesMode;
	REQUIRE( !m_rTouchItems.empty() );

	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );
	LoadDlgIcon( ID_TOUCH_FILES );

	ui::LoadPopupMenu( &m_listPopupMenu_OnSelection, IDR_CONTEXT_MENU, popup::TouchList );

	//m_fileListCtrl.ModifyListStyleEx( 0, LVS_EX_GRIDLINES );
	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );
	m_fileListCtrl.SetUseAlternateRowColoring();
	m_fileListCtrl.SetTextEffectCallback( this );
	m_fileListCtrl.SetPopupMenu( CReportListControl::OnSelection, &m_listPopupMenu_OnSelection );	// let us plug in our custom menu
	m_fileListCtrl.SetTrackMenuTarget( this );		// firstly handle our custom commands in this dialog
	m_fileListCtrl.SetFormatTableFlags( lv::SelRowsDisplayVisibleColumns );		// copy table as selected rows, using visible columns in display order
	CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );

	m_fileListCtrl.AddRecordCompare( pred::NewComparator( pred::TCompareCode() ) );		// default row item comparator
	m_fileListCtrl.AddColumnCompare( PathName, pred::NewComparator( pred::TCompareDisplayCode() ) );
	m_fileListCtrl.AddColumnCompare( DestModifyTime, pred::NewPropertyComparator<CTouchItem>( func::AsDestModifyTime() ), false );		// order date-time descending by default
	m_fileListCtrl.AddColumnCompare( DestCreationTime, pred::NewPropertyComparator<CTouchItem>( func::AsDestCreationTime() ), false );
	m_fileListCtrl.AddColumnCompare( DestAccessTime, pred::NewPropertyComparator<CTouchItem>( func::AsDestAccessTime() ), false );
	m_fileListCtrl.AddColumnCompare( SrcModifyTime, pred::NewPropertyComparator<CTouchItem>( func::TAsSrcModifyTime() ), false );
	m_fileListCtrl.AddColumnCompare( SrcCreationTime, pred::NewPropertyComparator<CTouchItem>( func::TAsSrcCreationTime() ), false );
	m_fileListCtrl.AddColumnCompare( SrcAccessTime, pred::NewPropertyComparator<CTouchItem>( func::TAsSrcAccessTime() ), false );

	static const TCHAR s_mixedFormat[] = _T("'(multiple values)'");
	m_modifiedDateCtrl.SetNullFormat( s_mixedFormat );
	m_createdDateCtrl.SetNullFormat( s_mixedFormat );
	m_accessedDateCtrl.SetNullFormat( s_mixedFormat );

	m_fileStatsStatic.GetMateToolbar()->GetStrip()
		.AddButton( ID_EDIT_COPY )
		.AddButton( ID_CMD_RESET_DESTINATIONS );

	m_currFolderEdit.SetUseFixedFont( false );
	m_currFolderEdit.GetMateToolbar()->GetStrip()
		.AddButton( ID_EDIT_COPY )
		.AddButton( ID_BROWSE_FOLDER );

	m_targetSelItemsButton.SetFrameMargins( -3, -3 );	// draw the frame outside of the button, in the dialog area
}

CTouchFilesDialog::~CTouchFilesDialog()
{
}

void CTouchFilesDialog::Construct( void )
{
	m_dateTimeStates.reserve( fs::_TimeFieldCount );
	m_dateTimeStates.push_back( multi::CDateTimeState( IDC_CREATED_DATE, fs::CreatedDate ) );
	m_dateTimeStates.push_back( multi::CDateTimeState( IDC_MODIFIED_DATE, fs::ModifiedDate ) );
	m_dateTimeStates.push_back( multi::CDateTimeState( IDC_ACCESSED_DATE, fs::AccessedDate ) );

	m_attribCheckStates.push_back( multi::CAttribCheckState( IDC_ATTRIB_READONLY_CHECK, CFile::readOnly ) );
	m_attribCheckStates.push_back( multi::CAttribCheckState( IDC_ATTRIB_HIDDEN_CHECK, CFile::hidden ) );
	m_attribCheckStates.push_back( multi::CAttribCheckState( IDC_ATTRIB_SYSTEM_CHECK , CFile::system ) );
	m_attribCheckStates.push_back( multi::CAttribCheckState( IDC_ATTRIB_ARCHIVE_CHECK, CFile::archive ) );
	m_attribCheckStates.push_back( multi::CAttribCheckState( IDC_ATTRIB_DIRECTORY_CHECK, CFile::directory ) );
	m_attribCheckStates.push_back( multi::CAttribCheckState( IDC_ATTRIB_VOLUME_CHECK, CFile::volume ) );
}

const std::vector<CTouchItem*>* CTouchFilesDialog::GetCmdSelItems( void ) const
{
	return app::TargetSelectedItems == m_pFileModel->GetTargetScope() && !m_selData.GetSelItems().empty() ? &m_selData.GetSelItems() : nullptr;
}

const std::vector<CTouchItem*>& CTouchFilesDialog::GetTargetItems( void ) const
{
	if ( const std::vector<CTouchItem*>* pSelItems = GetCmdSelItems() )
		return *pSelItems;

	return m_rTouchItems;
}

bool CTouchFilesDialog::TouchFiles( void )
{
	CFileService svc;
	std::auto_ptr<CMacroCommand> pTouchMacroCmd = svc.MakeTouchCmds( m_rTouchItems );
	if ( pTouchMacroCmd.get() != nullptr )
		if ( !pTouchMacroCmd->IsEmpty() )
		{
			ClearFileErrors();

			cmd::CScopedErrorObserver observe( this );
			return SafeExecuteCmd( pTouchMacroCmd.release() );
		}
		else
			return PromptCloseDialog();

	return false;
}

const CEnumTags& CTouchFilesDialog::GetTags_Mode( void )
{
	static const CEnumTags s_modeTags( _T("&Store|&Touch|Roll &Back|Roll &Fwd") );
	return s_modeTags;
}

void CTouchFilesDialog::SwitchMode( Mode mode )
{
	m_mode = mode;
	if ( nullptr == m_hWnd )
		return;

	UpdateOkButton( GetTags_Mode().FormatUi( m_mode ) );

	static const UINT ctrlIds[] =
	{
		IDC_COPY_SOURCE_PATHS_BUTTON, IDC_PASTE_FILES_BUTTON, IDC_RESET_FILES_BUTTON,
		IDC_MODIFIED_DATE, IDC_CREATED_DATE, IDC_ACCESSED_DATE,
		IDC_ATTRIB_READONLY_CHECK, IDC_ATTRIB_HIDDEN_CHECK, IDC_ATTRIB_SYSTEM_CHECK, IDC_ATTRIB_ARCHIVE_CHECK
	};
	ui::EnableControls( *this, ctrlIds, COUNT_OF( ctrlIds ), !IsRollMode() );

	m_dirtyTouch = utl::Any( m_rTouchItems, std::mem_fn( &CTouchItem::IsModified ) );
	ui::EnableControl( *this, IDOK, m_mode != CommitFilesMode || m_dirtyTouch );

	m_fileListCtrl.Invalidate();			// do some custom draw magic
}

void CTouchFilesDialog::PostMakeDest( bool silent /*= false*/ ) override
{
	if ( !silent )
		GotoDlgCtrl( GetDlgItem( IDOK ) );

	EnsureVisibleFirstError();
	SwitchMode( CommitFilesMode );
}

void CTouchFilesDialog::PopStackTop( svc::StackType stackType ) override
{
	ASSERT( !IsRollMode() );

	if ( utl::ICommand* pTopCmd = PeekCmdForDialog( stackType ) )		// comand that is target for this dialog editor?
	{
		bool isTouchMacro = cmd::TouchFile == pTopCmd->GetTypeID();

		ClearFileErrors();

		if ( isTouchMacro )
			m_pFileModel->FetchFromStack( stackType );		// fetch dataset from the stack top macro command
		else
			m_pCmdSvc->UndoRedo( stackType );

		MarkInvalidSrcItems();

		if ( isTouchMacro )							// file command?
			SwitchMode( svc::Undo == stackType ? RollBackMode : RollForwardMode );
		else if ( IsNativeCmd( pTopCmd ) )			// file state editing command?
			SwitchMode( CommitFilesMode );
	}
	else
		PopStackRunCrossEditor( stackType );		// end this dialog and execute the target dialog editor
}

void CTouchFilesDialog::OnExecuteCmd( utl::ICommand* pCmd )
{
	if ( app::TargetSelectedItems == m_pFileModel->GetTargetScope() )
		if ( cmd::HasSelItemsTarget( pCmd ) )
			ui::FlashCtrlFrame( &m_targetSelItemsButton, app::ColorWarningText, 3 );
}


void CTouchFilesDialog::SetupDialog( void )
{
	AccumulateCommonStates();
	SetupFileListView();							// fill in and select the found files list
	UpdateFieldControls();
}

void CTouchFilesDialog::UpdateTargetScopeButton( void )
{
	m_targetSelItemsButton.SetFrameColor( app::TargetSelectedItems == m_pFileModel->GetTargetScope() ? app::ColorWarningText : CLR_NONE );
}

void CTouchFilesDialog::UpdateFileListStatus( void )
{
	std::tstring message = str::Format( _T("Total: %d file(s) in %d folder(s)."), m_pFileModel->GetSourcePaths().size(), m_pFileModel->GetSrcFolderPaths().size() );

	if ( size_t selCount = m_selData.GetSelItems().size() )
		message += str::Format( _T("  Selected %d items(s)."), selCount );

	m_fileStatsStatic.SetWindowText( message );

	fs::CPath currFolderPath;

	if ( CTouchItem* pCaretItem = m_selData.GetCaretItem() )
		currFolderPath = pCaretItem->GetSrcState().m_fullPath.GetParentPath();

	m_currFolderEdit.SetShellPath( currFolderPath );
}

void CTouchFilesDialog::SetupFileListView( void )
{
	CScopedInternalChange internalChange( &m_fileListCtrl );
	lv::TScopedStatus_ByObject sel( &m_fileListCtrl );

	CScopedLockRedraw freeze( &m_fileListCtrl );

	m_fileListCtrl.DeleteAllItems();

	for ( unsigned int pos = 0; pos != m_rTouchItems.size(); ++pos )
	{
		const CTouchItem* pTouchItem = m_rTouchItems[pos];

		m_fileListCtrl.InsertObjectItem( pos, pTouchItem );		// PathName

		m_fileListCtrl.SetSubItemText( pos, DestAttributes, fmt::FormatFileAttributes( pTouchItem->GetDestState().m_attributes ) );
		m_fileListCtrl.SetSubItemText( pos, DestModifyTime, time_utl::FormatTimestamp( pTouchItem->GetDestState().m_modifTime ) );
		m_fileListCtrl.SetSubItemText( pos, DestCreationTime, time_utl::FormatTimestamp( pTouchItem->GetDestState().m_creationTime ) );
		m_fileListCtrl.SetSubItemText( pos, DestAccessTime, time_utl::FormatTimestamp( pTouchItem->GetDestState().m_accessTime ) );

		m_fileListCtrl.SetSubItemText( pos, SrcAttributes, fmt::FormatFileAttributes( pTouchItem->GetSrcState().m_attributes ) );
		m_fileListCtrl.SetSubItemText( pos, SrcModifyTime, time_utl::FormatTimestamp( pTouchItem->GetSrcState().m_modifTime ) );
		m_fileListCtrl.SetSubItemText( pos, SrcCreationTime, time_utl::FormatTimestamp( pTouchItem->GetSrcState().m_creationTime ) );
		m_fileListCtrl.SetSubItemText( pos, SrcAccessTime, time_utl::FormatTimestamp( pTouchItem->GetSrcState().m_accessTime ) );
	}

	m_fileListCtrl.SetupDiffColumnPair( SrcAttributes, DestAttributes, str::TGetMatch() );
	m_fileListCtrl.SetupDiffColumnPair( SrcModifyTime, DestModifyTime, str::TGetMatch() );
	m_fileListCtrl.SetupDiffColumnPair( SrcCreationTime, DestCreationTime, str::TGetMatch() );
	m_fileListCtrl.SetupDiffColumnPair( SrcAccessTime, DestAccessTime, str::TGetMatch() );

	m_fileListCtrl.InitialSortList();		// store original order and sort by current criteria
}

void CTouchFilesDialog::UpdateFileListViewSelItems( void )
{
	path::TGetMatch getMatchFunc;

	for ( CTouchItem* pTouchItem : m_selData.GetSelItems() )
	{
		int pos = m_fileListCtrl.FindItemIndex( pTouchItem );
		ASSERT( pos != -1 );

		m_fileListCtrl.SetSubItemText( pos, DestAttributes, fmt::FormatFileAttributes( pTouchItem->GetDestState().m_attributes ) );
		m_fileListCtrl.SetSubItemText( pos, DestModifyTime, time_utl::FormatTimestamp( pTouchItem->GetDestState().m_modifTime ) );
		m_fileListCtrl.SetSubItemText( pos, DestCreationTime, time_utl::FormatTimestamp( pTouchItem->GetDestState().m_creationTime ) );
		m_fileListCtrl.SetSubItemText( pos, DestAccessTime, time_utl::FormatTimestamp( pTouchItem->GetDestState().m_accessTime ) );

		// IMP: update the Dest side of DiffColumn pairs!
		m_fileListCtrl.UpdateItemDiffColumnDest( pos, DestAttributes, getMatchFunc );
		m_fileListCtrl.UpdateItemDiffColumnDest( pos, DestModifyTime, getMatchFunc );
		m_fileListCtrl.UpdateItemDiffColumnDest( pos, DestCreationTime, getMatchFunc );
		m_fileListCtrl.UpdateItemDiffColumnDest( pos, DestAccessTime, getMatchFunc );
	}
}

void CTouchFilesDialog::AccumulateCommonStates( void )
{
	ASSERT( !m_rTouchItems.empty() );

	// accumulate common values among multiple items, starting with an invalid value, i.e. uninitialized
	multi::SetInvalidAll( m_dateTimeStates );
	multi::SetInvalidAll( m_attribCheckStates );

	for ( const CTouchItem* pTouchItem : m_rTouchItems )
		AccumulateItemStates( pTouchItem );
}

void CTouchFilesDialog::AccumulateItemStates( const CTouchItem* pTouchItem )
{
	ASSERT_PTR( pTouchItem );

	m_dateTimeStates[ fs::CreatedDate ].Accumulate( pTouchItem->GetDestState().m_creationTime );
	m_dateTimeStates[ fs::ModifiedDate ].Accumulate( pTouchItem->GetDestState().m_modifTime );
	m_dateTimeStates[ fs::AccessedDate ].Accumulate( pTouchItem->GetDestState().m_accessTime );

	for ( multi::CAttribCheckState& rAttribState : m_attribCheckStates )
		rAttribState.Accumulate( pTouchItem->GetDestState().m_attributes );
}

void CTouchFilesDialog::UpdateFieldControls( void )
{
	// commit common values to field controls
	for ( const multi::CDateTimeState& dateTimeState : m_dateTimeStates )
		dateTimeState.UpdateCtrl( this );

	for ( const multi::CAttribCheckState& attribState : m_attribCheckStates )
		attribState.UpdateCtrl( this );
}

void CTouchFilesDialog::UpdateFieldsFromCaretItem( void )
{
	if ( const CTouchItem* pCaretItem = m_selData.GetCaretItem() )
	{
		multi::SetInvalidAll( m_dateTimeStates );
		multi::SetInvalidAll( m_attribCheckStates );

		AccumulateItemStates( pCaretItem );
	}
	else
	{
		multi::ClearAll( m_dateTimeStates );
		multi::ClearAll( m_attribCheckStates );
	}

	UpdateFieldControls();
}

void CTouchFilesDialog::InputFields( void )
{
	for ( multi::CDateTimeState& rDateTimeState : m_dateTimeStates )
		rDateTimeState.InputCtrl( this );

	for ( multi::CAttribCheckState& rAttribState : m_attribCheckStates )
		rAttribState.InputCtrl( this );
}

void CTouchFilesDialog::ApplyFields( void )
{
	if ( utl::ICommand* pCmd = MakeChangeDestFileStatesCmd() )
	{
		ClearFileErrors();
		SafeExecuteCmd( pCmd );
	}
}

utl::ICommand* CTouchFilesDialog::MakeChangeDestFileStatesCmd( void )
{
	const std::vector<CTouchItem*>& targetItems = GetTargetItems();
	std::vector<fs::CFileState> destFileStates; destFileStates.reserve( m_rTouchItems.size() );
	bool anyChange = false;

	// apply valid edits, i.e. if not null
	for ( const CTouchItem* pTouchItem : targetItems )
	{
		fs::CFileState newFileState = pTouchItem->GetDestState();

		for ( const multi::CDateTimeState& dateTimeState : m_dateTimeStates )
			if ( dateTimeState.CanApply() )
			{
				dateTimeState.Apply( newFileState );
			}

		for ( const multi::CAttribCheckState& attribState : m_attribCheckStates )
			if ( attribState.CanApply() )
			{
				attribState.Apply( newFileState );
			}

		destFileStates.push_back( newFileState );

		anyChange |= newFileState != pTouchItem->GetDestState();
	}

	return anyChange ? new CChangeDestFileStatesCmd( m_pFileModel, &targetItems, destFileStates ) : nullptr;
}

bool CTouchFilesDialog::VisibleAllSrcColumns( void ) const
{
	return m_fileListCtrl.IsColumnRangeVisible( SrcAttributes, SrcAccessTime );
}

void CTouchFilesDialog::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override
{
	const cmd::CommandType cmdType = static_cast<cmd::CommandType>( utl::GetSafeTypeID( pMessage ) ); cmdType;

	if ( nullptr == m_hWnd )
		return;

	if ( m_pFileModel == pSubject )
	{
		if ( nullptr == pMessage )
			SetupDialog();		// initial dlg setup
		else if ( const CChangeDestFileStatesCmd* pCmd = utl::GetSafeMatchCmd<CChangeDestFileStatesCmd>( pMessage, cmd::ChangeDestFileStates ) )
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

void CTouchFilesDialog::ClearFileErrors( void ) override
{
	m_errorItems.clear();

	if ( m_hWnd != nullptr )
		m_fileListCtrl.Invalidate();
}

void CTouchFilesDialog::OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg ) override
{
	errMsg;

	if ( CTouchItem* pErrorItem = FindItemWithKey( srcPath ) )
		utl::AddUnique( m_errorItems, pErrorItem );

	EnsureVisibleFirstError();
}

void CTouchFilesDialog::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override
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

void CTouchFilesDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const override
{
	pCtrl;
	static const ui::CTextEffect s_modPathName( ui::Bold );
	static const ui::CTextEffect s_modDest( ui::Regular, CReportListControl::s_mismatchDestTextColor );
	static const ui::CTextEffect s_modSrc( ui::Regular, CReportListControl::s_deleteSrcTextColor );
	static const ui::CTextEffect s_errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );

	const CTouchItem* pTouchItem = CReportListControl::AsPtr<CTouchItem>( rowKey );
	const ui::CTextEffect* pTextEffect = nullptr;
	bool isModified = false, isSrc = false;

	switch ( subItem )
	{
		case PathName:
			isModified = pTouchItem->IsModified();
			if ( isModified )
				pTextEffect = &s_modPathName;
			break;
		case SrcAttributes:
			isSrc = true;		// fall-through
		case DestAttributes:
			isModified = pTouchItem->GetDestState().m_attributes != pTouchItem->GetSrcState().m_attributes;
			break;
		case SrcModifyTime:
			isSrc = true;		// fall-through
		case DestModifyTime:
			isModified = pTouchItem->GetDestState().m_modifTime != pTouchItem->GetSrcState().m_modifTime;
			break;
		case SrcCreationTime:
			isSrc = true;		// fall-through
		case DestCreationTime:
			isModified = pTouchItem->GetDestState().m_creationTime != pTouchItem->GetSrcState().m_creationTime;
			break;
		case SrcAccessTime:
			isSrc = true;		// fall-through
		case DestAccessTime:
			isModified = pTouchItem->GetDestState().m_accessTime != pTouchItem->GetSrcState().m_accessTime;
			break;
	}

	if ( utl::Contains( m_errorItems, pTouchItem ) )
		rTextEffect |= s_errorBk;							// highlight error row background

	if ( pTextEffect != nullptr )
		rTextEffect |= *pTextEffect;
	else if ( isModified )
		rTextEffect |= isSrc ? s_modSrc : s_modDest;

	if ( EditMode == m_mode )
	{
		bool wouldModify = false;
		switch ( subItem )
		{
			case DestAttributes:
				wouldModify = pTouchItem->GetDestState().m_attributes != multi::EvalWouldBeAttributes( m_attribCheckStates, pTouchItem );
				break;
			case DestModifyTime:
				wouldModify = m_dateTimeStates[ fs::ModifiedDate ].WouldModify( pTouchItem );
				break;
			case DestCreationTime:
				wouldModify = m_dateTimeStates[ fs::CreatedDate ].WouldModify( pTouchItem );
				break;
			case DestAccessTime:
				wouldModify = m_dateTimeStates[ fs::AccessedDate ].WouldModify( pTouchItem );
				break;
		}

		if ( wouldModify )
			rTextEffect.m_textColor = ui::GetBlendedColor( rTextEffect.m_textColor != CLR_NONE ? rTextEffect.m_textColor : m_fileListCtrl.GetActualTextColor(), color::White );		// blend to gray
	}
}

void CTouchFilesDialog::ModifyDiffTextEffectAt( lv::CMatchEffects& rEffects, LPARAM rowKey, int subItem, CReportListControl* pCtrl ) const override
{
	rowKey, pCtrl;
	switch ( subItem )
	{
		case SrcModifyTime:
		case DestModifyTime:
		case SrcCreationTime:
		case DestCreationTime:
		case SrcAccessTime:
		case DestAccessTime:
			ClearFlag( rEffects.m_rNotEqual.m_fontEffect, ui::Bold );		// line-up date columns nicely
			//rEffects.m_rNotEqual.m_frameFillTraits.Init( color::Green, 20, 10 );
			break;
	}
}

CTouchItem* CTouchFilesDialog::FindItemWithKey( const fs::CPath& keyPath ) const
{
	std::vector<CTouchItem*>::const_iterator itFoundItem = utl::BinaryFind( m_rTouchItems, keyPath, CPathItemBase::ToFilePath() );
	return itFoundItem != m_rTouchItems.end() ? *itFoundItem : nullptr;
}

void CTouchFilesDialog::MarkInvalidSrcItems( void )
{
	for ( CTouchItem* pTouchItem : m_rTouchItems )
		if ( !pTouchItem->GetFilePath().FileExist() )
			utl::AddUnique( m_errorItems, pTouchItem );
}

void CTouchFilesDialog::EnsureVisibleFirstError( void )
{
	if ( const CTouchItem* pFirstErrorItem = GetFirstErrorItem<CTouchItem>() )
		m_fileListCtrl.EnsureVisibleObject( pFirstErrorItem );

	m_fileListCtrl.Invalidate();				// trigger some highlighting
}

fs::TimeField CTouchFilesDialog::GetTimeFieldFromId( UINT dtCtrlId )
{
	switch ( dtCtrlId )
	{
		default: ASSERT( false );
		case IDC_MODIFIED_DATE:	return fs::ModifiedDate;
		case IDC_CREATED_DATE:	return fs::CreatedDate;
		case IDC_ACCESSED_DATE:	return fs::AccessedDate;
	}
}

BOOL CTouchFilesDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		m_fileListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CTouchFilesDialog::DoDataExchange( CDataExchange* pDX )
{
	const bool firstInit = nullptr == m_fileListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_FILE_TOUCH_LIST, m_fileListCtrl );
	DDX_Control( pDX, IDC_MODIFIED_DATE, m_modifiedDateCtrl );
	DDX_Control( pDX, IDC_CREATED_DATE, m_createdDateCtrl );
	DDX_Control( pDX, IDC_ACCESSED_DATE, m_accessedDateCtrl );

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

BEGIN_MESSAGE_MAP( CTouchFilesDialog, CFileEditorBaseDialog )
	ON_UPDATE_COMMAND_UI_RANGE( IDC_UNDO_BUTTON, IDC_REDO_BUTTON, OnUpdateUndoRedo )
	ON_BN_CLICKED( IDC_TARGET_SEL_ITEMS_CHECK, OnToggle_TargetSelItems )
	ON_COMMAND( ID_CMD_RESET_DESTINATIONS, On_SelItems_ResetDestFile )
	ON_UPDATE_COMMAND_UI( ID_CMD_RESET_DESTINATIONS, OnUpdateListSelection )

	ON_BN_CLICKED( IDC_COPY_SOURCE_PATHS_BUTTON, OnBnClicked_CopySourceFiles )
	ON_BN_CLICKED( IDC_PASTE_FILES_BUTTON, OnBnClicked_PasteDestStates )
	ON_BN_CLICKED( IDC_RESET_FILES_BUTTON, OnBnClicked_ResetDestFiles )
	ON_BN_CLICKED( IDC_SHOW_SRC_COLUMNS_CHECK, OnBnClicked_ShowSrcColumns )
	ON_COMMAND_RANGE( ID_COPY_MODIFIED_DATE, ID_COPY_ACCESSED_DATE, OnCopyDateField )
	ON_UPDATE_COMMAND_UI_RANGE( ID_COPY_MODIFIED_DATE, ID_COPY_ACCESSED_DATE, OnUpdateListCaretItem )
	ON_COMMAND_RANGE( ID_PUSH_MODIFIED_DATE, ID_PUSH_ACCESSED_DATE, OnPushDateField )
	ON_UPDATE_COMMAND_UI_RANGE( ID_PUSH_MODIFIED_DATE, ID_PUSH_ACCESSED_DATE, OnUpdateListSelection )
	ON_COMMAND( ID_PUSH_ATTRIBUTE_FIELDS, OnPushAttributeFields )
	ON_UPDATE_COMMAND_UI( ID_PUSH_ATTRIBUTE_FIELDS, OnUpdateListSelection )
	ON_COMMAND( ID_PUSH_ALL_FIELDS, OnPushAllFields )
	ON_UPDATE_COMMAND_UI( ID_PUSH_ALL_FIELDS, OnUpdateListCaretItem )
	ON_COMMAND_RANGE( IDC_ATTRIB_READONLY_CHECK, IDC_ATTRIB_VOLUME_CHECK, OnToggle_Attribute )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_FILE_TOUCH_LIST, OnLvnItemChanged_TouchList )
	ON_NOTIFY( lv::LVN_CopyTableText, IDC_FILE_TOUCH_LIST, OnLvnCopyTableText_TouchList )
	ON_NOTIFY( DTN_DATETIMECHANGE, IDC_MODIFIED_DATE, OnDtnDateTimeChange )
	ON_NOTIFY( DTN_DATETIMECHANGE, IDC_CREATED_DATE, OnDtnDateTimeChange )
	ON_NOTIFY( DTN_DATETIMECHANGE, IDC_ACCESSED_DATE, OnDtnDateTimeChange )
END_MESSAGE_MAP()

void CTouchFilesDialog::OnOK( void )
{
	switch ( m_mode )
	{
		case EditMode:
			InputFields();
			ApplyFields();
			PostMakeDest();
			break;
		case CommitFilesMode:
			if ( TouchFiles() )
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

void CTouchFilesDialog::OnUpdateUndoRedo( CCmdUI* pCmdUI )
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

void CTouchFilesDialog::OnFieldChanged( void )
{
	SwitchMode( EditMode );
}

void CTouchFilesDialog::OnToggle_TargetSelItems( void )
{
	if ( m_pFileModel->SetTargetScope( BST_CHECKED == m_targetSelItemsButton.GetCheck() ? app::TargetSelectedItems : app::TargetAllItems ) )
		UpdateTargetScopeButton();
}

void CTouchFilesDialog::On_SelItems_ResetDestFile( void )
{
	if ( !m_selData.GetSelItems().empty() )
		SafeExecuteCmd( CChangeDestFileStatesCmd::MakeResetItemsCmd( m_pFileModel, m_selData.GetSelItems() ) );
}


void CTouchFilesDialog::OnBnClicked_CopySourceFiles( void )
{
	if ( !CFileModel::CopyClipSourceFileStates( this, GetTargetItems() ) )
		AfxMessageBox( _T("Cannot copy source file states to clipboard!"), MB_ICONERROR | MB_OK );
}

void CTouchFilesDialog::OnBnClicked_PasteDestStates( void )
{
	try
	{
		ClearFileErrors();
		SafeExecuteCmd( m_pFileModel->MakeClipPasteDestFileStatesCmd( GetTargetItems(), this ) );
	}
	catch ( CRuntimeException& exc )
	{
		exc.ReportError();
	}
}

void CTouchFilesDialog::OnBnClicked_ResetDestFiles( void )
{
	ClearFileErrors();
	SafeExecuteCmd( new CResetDestinationsMacroCmd( m_pFileModel ) );
}

void CTouchFilesDialog::OnBnClicked_ShowSrcColumns( void )
{
	if ( VisibleAllSrcColumns() )
	{
		for ( CListTraits::TColumn column = SrcAttributes; column <= SrcAccessTime; ++column )
			m_fileListCtrl.ShowColumn( column, false );		// hide column
	}
	else
		m_fileListCtrl.ResetColumnLayout();					// show all columns with default layout
}

void CTouchFilesDialog::OnCopyDateField( UINT cmdId )
{
	fs::TimeField dateField;

	switch ( cmdId )
	{
		case ID_COPY_MODIFIED_DATE:	dateField = fs::ModifiedDate; break;
		case ID_COPY_CREATED_DATE:	dateField = fs::CreatedDate; break;
		case ID_COPY_ACCESSED_DATE:	dateField = fs::AccessedDate; break;
		default:
			ASSERT( false );
			return;
	}

	const CTouchItem* pCaretItem = m_selData.GetCaretItem();
	ASSERT_PTR( pCaretItem );

	CTextClipboard::CopyText( time_utl::FormatTimestamp( pCaretItem->GetSrcState().GetTimeField( dateField ) ), m_hWnd );
}

void CTouchFilesDialog::OnPushDateField( UINT cmdId )
{
	fs::TimeField dateField;

	switch ( cmdId )
	{
		case ID_PUSH_MODIFIED_DATE:	dateField = fs::ModifiedDate; break;
		case ID_PUSH_CREATED_DATE:	dateField = fs::CreatedDate; break;
		case ID_PUSH_ACCESSED_DATE:	dateField = fs::AccessedDate; break;
		default:
			ASSERT( false );
			return;
	}

	const CTouchItem* pCaretItem = m_selData.GetCaretItem();
	ASSERT_PTR( pCaretItem );

	multi::CDateTimeState& rDateTimeState = m_dateTimeStates[ dateField ];
	rDateTimeState.Reset( pCaretItem->GetDestState().GetTimeField( dateField ) );

	if ( rDateTimeState.UpdateCtrl( this ) )
		app::FlashCtrlFrame( GetDlgItem( rDateTimeState.m_ctrlId ) );

	OnFieldChanged();
}

void CTouchFilesDialog::OnPushAttributeFields( void )
{
	multi::SetInvalidAll( m_attribCheckStates );

	for ( const CTouchItem* pSelItem : m_selData.GetSelItems() )
	{
		BYTE attributes = pSelItem->GetSrcState().m_attributes;

		for ( multi::CAttribCheckState& rAttribState: m_attribCheckStates )
			rAttribState.Accumulate( attributes );
	}

	for ( multi::CAttribCheckState& rAttribState : m_attribCheckStates )
	{
		if ( rAttribState.UpdateCtrl( this ) )
			app::FlashCtrlFrame( GetDlgItem( rAttribState.m_ctrlId ) );
	}

	OnFieldChanged();
}

void CTouchFilesDialog::OnPushAllFields( void )
{
	UpdateFieldsFromCaretItem();
	OnFieldChanged();
}

void CTouchFilesDialog::OnUpdateListCaretItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_selData.GetCaretItem() != nullptr );
}

void CTouchFilesDialog::OnUpdateListSelection( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_selData.GetSelItems().empty() );
}

void CTouchFilesDialog::OnToggle_Attribute( UINT checkId )
{
	if ( multi::CAttribCheckState* pAttribState = multi::FindWithCtrlId( m_attribCheckStates, checkId ) )
		if ( pAttribState->InputCtrl( this ) )				// input the checked state so that custom draw can evaluate would-modify
			OnFieldChanged();
		else
			m_fileListCtrl.Invalidate();
}

void CTouchFilesDialog::OnLvnItemChanged_TouchList( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;

	ASSERT( !m_fileListCtrl.IsInternalChange() );		// filtered internally by CReportListControl
	if ( m_fileListCtrl.IsSelectionCaretChangeNotify( pNmList ) )
	{
		CReportListControl::TraceNotify( pNmList );

		if ( m_selData.ReadList( &m_fileListCtrl ) )
			UpdateFileListStatus();
	}

	*pResult = 0;
}

void CTouchFilesDialog::OnLvnCopyTableText_TouchList( NMHDR* pNmHdr, LRESULT* pResult )
{
	lv::CNmCopyTableText* pNmInfo = (lv::CNmCopyTableText*)pNmHdr;
	*pResult = 0L;		// continue default handling

	if ( ui::IsKeyPressed( VK_SHIFT ) )
		pNmInfo->m_textRows.push_back( pNmInfo->m_pColumnSet->FormatHeaderRow() );		// copy header row if SHIFT is pressed
}

void CTouchFilesDialog::OnDtnDateTimeChange( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMDATETIMECHANGE* pChange = (NMDATETIMECHANGE*)pNmHdr; pChange;
	*pResult = 0L;

#ifdef _DEBUG
	// Cannot break into the debugger due to a mouse hook set in CDateTimeCtrl implementation (Windows).
	//	https://stackoverflow.com/questions/18621575/are-there-issues-with-dtn-datetimechange-breakpoints-and-the-date-time-picker-co

	fs::TimeField field = GetTimeFieldFromId( static_cast<UINT>( pNmHdr->idFrom ) );
	CDateTimeControl* pCtrl = checked_static_cast<CDateTimeControl*>( FromHandle( pNmHdr->hwndFrom ) );
	TRACE( _T(" - CTouchFilesDialog::OnDtnDateTimeChange for: %s = <%s>\n"), fs::GetTags_TimeField().FormatUi( field ).c_str(), time_utl::FormatTimestamp( pCtrl->GetDateTime() ).c_str() );
#endif

	if ( multi::CDateTimeState* pDateTimeState = multi::FindWithCtrlId( m_dateTimeStates, static_cast<UINT>( pNmHdr->idFrom ) ) )
		if ( pDateTimeState->InputCtrl( this ) )			// input the checked state so that custom draw can evaluate would-modify
			OnFieldChanged();
		else
			m_fileListCtrl.Invalidate();
}
