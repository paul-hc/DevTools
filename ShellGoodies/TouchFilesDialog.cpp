
#include "pch.h"
#include "TouchFilesDialog.h"
#include "TouchItem.h"
#include "FileModel.h"
#include "FileService.h"
#include "EditingCommands.h"
#include "GeneralOptions.h"
#include "Application.h"
#include "AppCommands.h"
#include "resource.h"
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


namespace reg
{
	static const TCHAR section_dialog[] = _T("TouchDialog");
}

namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_FILE_TOUCH_LIST, Size },
		{ IDC_SHOW_SOURCE_INFO_CHECK, MoveX },

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
	, m_anyChanges( false )
{
	Construct();
	m_nativeCmdTypes.push_back( cmd::ResetDestinations );
	m_mode = CommitFilesMode;
	REQUIRE( !m_rTouchItems.empty() );

	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );
	LoadDlgIcon( ID_TOUCH_FILES );

	m_fileListCtrl.ModifyListStyleEx( 0, LVS_EX_GRIDLINES );
	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );
	m_fileListCtrl.SetUseAlternateRowColoring();
	m_fileListCtrl.SetTextEffectCallback( this );
	m_fileListCtrl.SetPopupMenu( CReportListControl::OnSelection, NULL );				// let us track a custom menu
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

bool CTouchFilesDialog::TouchFiles( void )
{
	CFileService svc;
	std::auto_ptr<CMacroCommand> pTouchMacroCmd = svc.MakeTouchCmds( m_rTouchItems );
	if ( pTouchMacroCmd.get() != NULL )
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

void CTouchFilesDialog::SwitchMode( Mode mode )
{
	m_mode = mode;
	if ( NULL == m_hWnd )
		return;

	static const CEnumTags modeTags( _T("&Store|&Touch|Roll &Back|Roll &Fwd") );
	UpdateOkButton( modeTags.FormatUi( m_mode ) );

	static const UINT ctrlIds[] =
	{
		IDC_COPY_SOURCE_PATHS_BUTTON, IDC_PASTE_FILES_BUTTON, IDC_RESET_FILES_BUTTON,
		IDC_MODIFIED_DATE, IDC_CREATED_DATE, IDC_ACCESSED_DATE,
		IDC_ATTRIB_READONLY_CHECK, IDC_ATTRIB_HIDDEN_CHECK, IDC_ATTRIB_SYSTEM_CHECK, IDC_ATTRIB_ARCHIVE_CHECK
	};
	ui::EnableControls( *this, ctrlIds, COUNT_OF( ctrlIds ), !IsRollMode() );

	m_anyChanges = utl::Any( m_rTouchItems, std::mem_fun( &CTouchItem::IsModified ) );
	ui::EnableControl( *this, IDOK, m_mode != CommitFilesMode || m_anyChanges );

	m_fileListCtrl.Invalidate();			// do some custom draw magic
}

void CTouchFilesDialog::PostMakeDest( bool silent /*= false*/ )
{
	if ( !silent )
		GotoDlgCtrl( GetDlgItem( IDOK ) );

	EnsureVisibleFirstError();
	SwitchMode( CommitFilesMode );
}

void CTouchFilesDialog::PopStackTop( svc::StackType stackType )
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

void CTouchFilesDialog::SetupDialog( void )
{
	AccumulateCommonStates();
	SetupFileListView();							// fill in and select the found files list
	UpdateFieldControls();
}

void CTouchFilesDialog::SetupFileListView( void )
{
	CScopedInternalChange internalChange( &m_fileListCtrl );
	lv::TScopedStatus_ByObject sel( &m_fileListCtrl );

	CScopedLockRedraw freeze( &m_fileListCtrl );

	m_fileListCtrl.DeleteAllItems();

	for ( unsigned int pos = 0; pos != m_rTouchItems.size(); ++pos )
	{
		const CTouchItem* pTouchItem = m_rTouchItems[ pos ];

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

void CTouchFilesDialog::AccumulateCommonStates( void )
{
	ASSERT( !m_rTouchItems.empty() );

	// accumulate common values among multiple items, starting with an invalid value, i.e. uninitialized
	multi::SetInvalidAll( m_dateTimeStates );
	multi::SetInvalidAll( m_attribCheckStates );

	for ( size_t i = 0; i != m_rTouchItems.size(); ++i )
		AccumulateItemStates( m_rTouchItems[ i ] );
}

void CTouchFilesDialog::AccumulateItemStates( const CTouchItem* pTouchItem )
{
	ASSERT_PTR( pTouchItem );

	m_dateTimeStates[ fs::CreatedDate ].Accumulate( pTouchItem->GetDestState().m_creationTime );
	m_dateTimeStates[ fs::ModifiedDate ].Accumulate( pTouchItem->GetDestState().m_modifTime );
	m_dateTimeStates[ fs::AccessedDate ].Accumulate( pTouchItem->GetDestState().m_accessTime );

	for ( std::vector< multi::CAttribCheckState >::iterator itAttribState = m_attribCheckStates.begin(); itAttribState != m_attribCheckStates.end(); ++itAttribState )
		itAttribState->Accumulate( pTouchItem->GetDestState().m_attributes );
}

void CTouchFilesDialog::UpdateFieldControls( void )
{
	// commit common values to field controls
	for ( std::vector< multi::CDateTimeState >::const_iterator itDateTimeState = m_dateTimeStates.begin(); itDateTimeState != m_dateTimeStates.end(); ++itDateTimeState )
		itDateTimeState->UpdateCtrl( this );

	for ( std::vector< multi::CAttribCheckState >::const_iterator itAttribState = m_attribCheckStates.begin(); itAttribState != m_attribCheckStates.end(); ++itAttribState )
		itAttribState->UpdateCtrl( this );
}

void CTouchFilesDialog::UpdateFieldsFromSel( int selIndex )
{
	if ( selIndex != -1 )
	{
		multi::SetInvalidAll( m_dateTimeStates );
		multi::SetInvalidAll( m_attribCheckStates );

		AccumulateItemStates( m_fileListCtrl.GetPtrAt<CTouchItem>( selIndex ) );
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
	for ( std::vector< multi::CDateTimeState >::iterator itDateTimeState = m_dateTimeStates.begin(); itDateTimeState != m_dateTimeStates.end(); ++itDateTimeState )
		itDateTimeState->InputCtrl( this );

	for ( std::vector< multi::CAttribCheckState >::iterator itAttribState = m_attribCheckStates.begin(); itAttribState != m_attribCheckStates.end(); ++itAttribState )
		itAttribState->InputCtrl( this );
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
	std::vector< fs::CFileState > destFileStates; destFileStates.reserve( m_rTouchItems.size() );
	bool anyChanges = false;

	// apply valid edits, i.e. if not null
	for ( size_t i = 0; i != m_rTouchItems.size(); ++i )
	{
		const CTouchItem* pTouchItem = m_rTouchItems[ i ];

		fs::CFileState newFileState = pTouchItem->GetDestState();

		for ( std::vector< multi::CDateTimeState >::const_iterator itDateTimeState = m_dateTimeStates.begin(); itDateTimeState != m_dateTimeStates.end(); ++itDateTimeState )
			if ( itDateTimeState->CanApply() )
				itDateTimeState->Apply( newFileState );

		for ( std::vector< multi::CAttribCheckState >::const_iterator itAttribState = m_attribCheckStates.begin(); itAttribState != m_attribCheckStates.end(); ++itAttribState )
			if ( itAttribState->CanApply() )
				itAttribState->Apply( newFileState );

		if ( newFileState != pTouchItem->GetDestState() )
			anyChanges = true;

		destFileStates.push_back( newFileState );
	}

	return anyChanges ? new CChangeDestFileStatesCmd( m_pFileModel, destFileStates ) : NULL;
}

bool CTouchFilesDialog::VisibleAllSrcColumns( void ) const
{
	for ( int column = SrcAttributes; column <= SrcAccessTime; ++column )
		if ( 0 == m_fileListCtrl.GetColumnWidth( column ) )
			return false;

	return true;
}

void CTouchFilesDialog::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage )
{
	pMessage;

	if ( m_hWnd != NULL )
		if ( m_pFileModel == pSubject )
		{
			SetupDialog();

			switch ( utl::GetSafeTypeID( pMessage ) )
			{
				case cmd::ChangeDestFileStates:
					PostMakeDest();
					break;
			}
		}
		else if ( &CGeneralOptions::Instance() == pSubject )
			CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );
}

void CTouchFilesDialog::ClearFileErrors( void )
{
	m_errorItems.clear();

	if ( m_hWnd != NULL )
		m_fileListCtrl.Invalidate();
}

void CTouchFilesDialog::OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg )
{
	errMsg;

	if ( CTouchItem* pErrorItem = FindItemWithKey( srcPath ) )
		utl::AddUnique( m_errorItems, pErrorItem );

	EnsureVisibleFirstError();
}

void CTouchFilesDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	pCtrl;
	static const ui::CTextEffect s_modPathName( ui::Bold );
	static const ui::CTextEffect s_modDest( ui::Regular, CReportListControl::s_mismatchDestTextColor );
	static const ui::CTextEffect s_modSrc( ui::Regular, CReportListControl::s_deleteSrcTextColor );
	static const ui::CTextEffect s_errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );

	const CTouchItem* pTouchItem = CReportListControl::AsPtr<CTouchItem>( rowKey );
	const ui::CTextEffect* pTextEffect = NULL;
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

	if ( pTextEffect != NULL )
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

void CTouchFilesDialog::ModifyDiffTextEffectAt( lv::CMatchEffects& rEffects, LPARAM rowKey, int subItem, CReportListControl* pCtrl ) const
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
	std::vector< CTouchItem* >::const_iterator itFoundItem = utl::BinaryFind( m_rTouchItems, keyPath, CPathItemBase::ToFilePath() );
	return itFoundItem != m_rTouchItems.end() ? *itFoundItem : NULL;
}

void CTouchFilesDialog::MarkInvalidSrcItems( void )
{
	for ( std::vector< CTouchItem* >::const_iterator itTouchItem = m_rTouchItems.begin(); itTouchItem != m_rTouchItems.end(); ++itTouchItem )
		if ( !( *itTouchItem )->GetFilePath().FileExist() )
			utl::AddUnique( m_errorItems, *itTouchItem );
}

void CTouchFilesDialog::EnsureVisibleFirstError( void )
{
	if ( const CTouchItem* pFirstErrorItem = GetFirstErrorItem<CTouchItem>() )
		m_fileListCtrl.EnsureVisibleObject( pFirstErrorItem );

	m_fileListCtrl.Invalidate();				// trigger some highlighting
}

fs::TimeField CTouchFilesDialog::GetTimeField( UINT dtId )
{
	switch ( dtId )
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
	const bool firstInit = NULL == m_fileListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_FILE_TOUCH_LIST, m_fileListCtrl );
	DDX_Control( pDX, IDC_MODIFIED_DATE, m_modifiedDateCtrl );
	DDX_Control( pDX, IDC_CREATED_DATE, m_createdDateCtrl );
	DDX_Control( pDX, IDC_ACCESSED_DATE, m_accessedDateCtrl );

	ui::DDX_ButtonIcon( pDX, IDC_COPY_SOURCE_PATHS_BUTTON, ID_EDIT_COPY );
	ui::DDX_ButtonIcon( pDX, IDC_PASTE_FILES_BUTTON, ID_EDIT_PASTE );
	ui::DDX_ButtonIcon( pDX, IDC_RESET_FILES_BUTTON, ID_RESET_DEFAULT );

	if ( firstInit )
	{
		OnUpdate( m_pFileModel, NULL );
		CheckDlgButton( IDC_SHOW_SOURCE_INFO_CHECK, VisibleAllSrcColumns() );
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CTouchFilesDialog, CFileEditorBaseDialog )
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI_RANGE( IDC_UNDO_BUTTON, IDC_REDO_BUTTON, OnUpdateUndoRedo )
	ON_BN_CLICKED( IDC_COPY_SOURCE_PATHS_BUTTON, OnBnClicked_CopySourceFiles )
	ON_BN_CLICKED( IDC_PASTE_FILES_BUTTON, OnBnClicked_PasteDestStates )
	ON_BN_CLICKED( IDC_RESET_FILES_BUTTON, OnBnClicked_ResetDestFiles )
	ON_BN_CLICKED( IDC_SHOW_SOURCE_INFO_CHECK, OnBnClicked_ShowSrcColumns )
	ON_COMMAND_RANGE( ID_COPY_MODIFIED_DATE, ID_COPY_ACCESSED_DATE, OnCopyDateCell )
	ON_UPDATE_COMMAND_UI_RANGE( ID_COPY_MODIFIED_DATE, ID_COPY_ACCESSED_DATE, OnUpdateSelListItem )
	ON_COMMAND( ID_PUSH_TO_ATTRIBUTE_FIELDS, OnPushToAttributeFields )
	ON_UPDATE_COMMAND_UI( ID_PUSH_TO_ATTRIBUTE_FIELDS, OnUpdateSelListItem )
	ON_COMMAND( ID_PUSH_TO_ALL_FIELDS, OnPushToAllFields )
	ON_UPDATE_COMMAND_UI( ID_PUSH_TO_ALL_FIELDS, OnUpdateSelListItem )
	ON_COMMAND_RANGE( IDC_ATTRIB_READONLY_CHECK, IDC_ATTRIB_VOLUME_CHECK, OnToggle_Attribute )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_FILE_TOUCH_LIST, OnLvnItemChanged_TouchList )
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

			if ( m_pCmdSvc->UndoRedo( RollBackMode == m_mode ? svc::Undo : svc::Redo ) ||
				 PromptCloseDialog( PromptClose ) )
				__super::OnOK();
			else
				SwitchMode( EditMode );
			break;
		}
	}
}

void CTouchFilesDialog::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( &m_fileListCtrl == pWnd )
	{
		ui::TrackContextMenu( IDR_CONTEXT_MENU, popup::TouchList, this, screenPos );
		return;					// supress rising WM_CONTEXTMENU to the parent
	}

	__super::OnContextMenu( pWnd, screenPos );
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

void CTouchFilesDialog::OnBnClicked_CopySourceFiles( void )
{
	if ( !m_pFileModel->CopyClipSourceFileStates( this ) )
		AfxMessageBox( _T("Cannot copy source file states to clipboard!"), MB_ICONERROR | MB_OK );
}

void CTouchFilesDialog::OnBnClicked_PasteDestStates( void )
{
	try
	{
		ClearFileErrors();
		SafeExecuteCmd( m_pFileModel->MakeClipPasteDestFileStatesCmd( this ) );
	}
	catch ( CRuntimeException& e )
	{
		e.ReportError();
	}
}

void CTouchFilesDialog::OnBnClicked_ResetDestFiles( void )
{
	ClearFileErrors();
	SafeExecuteCmd( new CResetDestinationsCmd( m_pFileModel ) );
}

void CTouchFilesDialog::OnBnClicked_ShowSrcColumns( void )
{
	if ( VisibleAllSrcColumns() )
	{
		for ( int column = SrcAttributes; column <= SrcAccessTime; ++column )
			m_fileListCtrl.SetColumnWidth( column, 0 );		// hide column
	}
	else
		m_fileListCtrl.ResetColumnLayout();					// show all columns with default layout
}

void CTouchFilesDialog::OnCopyDateCell( UINT cmdId )
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

	const CTouchItem* pTouchItem = m_fileListCtrl.GetPtrAt<CTouchItem>( m_fileListCtrl.GetCurSel() );
	ASSERT_PTR( pTouchItem );
	CTextClipboard::CopyText( time_utl::FormatTimestamp( pTouchItem->GetSrcState().GetTimeField( dateField ) ), m_hWnd );
}

void CTouchFilesDialog::OnPushToAttributeFields( void )
{
	BYTE attributes = m_fileListCtrl.GetPtrAt<CTouchItem>( m_fileListCtrl.GetCurSel() )->GetSrcState().m_attributes;

	multi::SetInvalidAll( m_attribCheckStates );
	for ( std::vector< multi::CAttribCheckState >::iterator itAttribState = m_attribCheckStates.begin(); itAttribState != m_attribCheckStates.end(); ++itAttribState )
	{
		itAttribState->Accumulate( attributes );
		itAttribState->UpdateCtrl( this );
	}

	OnFieldChanged();
}

void CTouchFilesDialog::OnPushToAllFields( void )
{
	UpdateFieldsFromSel( m_fileListCtrl.GetCurSel() );
	OnFieldChanged();
}

void CTouchFilesDialog::OnUpdateSelListItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_fileListCtrl.GetCurSel() != -1 );
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

	if ( CReportListControl::IsSelectionChangeNotify( pNmList, LVIS_SELECTED | LVIS_FOCUSED ) )
	{
		//UpdateFieldsFromSel( m_fileListCtrl.GetCurSel() );
	}

	*pResult = 0;
}

void CTouchFilesDialog::OnDtnDateTimeChange( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMDATETIMECHANGE* pChange = (NMDATETIMECHANGE*)pNmHdr; pChange;
	*pResult = 0L;

#ifdef _DEBUG
	// Cannot break into the debugger due to a mouse hook set in CDateTimeCtrl implementation (Windows).
	//	https://stackoverflow.com/questions/18621575/are-there-issues-with-dtn-datetimechange-breakpoints-and-the-date-time-picker-co

	fs::TimeField field = GetTimeField( static_cast<UINT>( pNmHdr->idFrom ) );
	CDateTimeControl* pCtrl = checked_static_cast<CDateTimeControl*>( FromHandle( pNmHdr->hwndFrom ) );
	TRACE( _T(" - CTouchFilesDialog::OnDtnDateTimeChange for: %s = <%s>\n"), fs::GetTags_TimeField().FormatUi( field ).c_str(), time_utl::FormatTimestamp( pCtrl->GetDateTime() ).c_str() );
#endif

	if ( multi::CDateTimeState* pDateTimeState = multi::FindWithCtrlId( m_dateTimeStates, static_cast<UINT>( pNmHdr->idFrom ) ) )
		if ( pDateTimeState->InputCtrl( this ) )			// input the checked state so that custom draw can evaluate would-modify
			OnFieldChanged();
		else
			m_fileListCtrl.Invalidate();
}
