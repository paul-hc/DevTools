
#include "stdafx.h"
#include "FindDuplicatesDialog.h"
#include "TouchItem.h"
#include "FileModel.h"
#include "FileService.h"
#include "FileCommands.h"
#include "GeneralOptions.h"
#include "Application.h"
#include "resource.h"
#include "utl/Clipboard.h"
#include "utl/Color.h"
#include "utl/ContainerUtilities.h"
#include "utl/CmdInfoStore.h"
#include "utl/Command.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/LongestCommonSubsequence.h"
#include "utl/RuntimeException.h"
#include "utl/MenuUtilities.h"
#include "utl/StringUtilities.h"
#include "utl/UtilitiesEx.h"
#include "utl/Thumbnailer.h"
#include "utl/TimeUtils.h"
#include "utl/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_dialog[] = _T("TouchDialog");
}

namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_DUPLICATE_FILES_LIST, Size },

		{ IDC_COPY_SOURCE_PATHS_BUTTON, MoveY },
		{ IDC_PASTE_FILES_BUTTON, MoveY },
		{ IDC_RESET_FILES_BUTTON, MoveY },

		{ IDOK, MoveX },
		{ IDCANCEL, MoveX },
		{ IDC_TOOLBAR_PLACEHOLDER, MoveX }
	};
}

CFindDuplicatesDialog::CFindDuplicatesDialog( CFileModel* pFileModel, CWnd* pParent )
	: CFileEditorBaseDialog( pFileModel, cmd::TouchFile, IDD_FIND_DUPLICATES_DIALOG, pParent )
	, m_rTouchItems( m_pFileModel->LazyInitTouchItems() )
	, m_fileListCtrl( IDC_DUPLICATE_FILES_LIST )
	, m_anyChanges( false )
{
	m_nativeCmdTypes.push_back( cmd::ResetDestinations );
	m_mode = CommitFilesMode;
	REQUIRE( !m_rTouchItems.empty() );

	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	LoadDlgIcon( ID_TOUCH_FILES );

	m_fileListCtrl.ModifyListStyleEx( 0, LVS_EX_GRIDLINES );
	m_fileListCtrl.SetSection( m_regSection + _T("\\List") );
	m_fileListCtrl.SetUseAlternateRowColoring();
	m_fileListCtrl.SetTextEffectCallback( this );
	m_fileListCtrl.SetPopupMenu( CReportListControl::OnSelection, NULL );				// let us track a custom menu
	CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );

	m_fileListCtrl.AddRecordCompare( pred::NewComparator( pred::CompareCode() ) );		// default row item comparator
	m_fileListCtrl.AddColumnCompare( PathName, pred::NewComparator( pred::CompareDisplayCode() ) );
	m_fileListCtrl.AddColumnCompare( DestModifyTime, pred::NewPropertyComparator< CTouchItem >( func::AsDestModifyTime() ), false );		// order date-time descending by default
	m_fileListCtrl.AddColumnCompare( DestCreationTime, pred::NewPropertyComparator< CTouchItem >( func::AsDestCreationTime() ), false );
	m_fileListCtrl.AddColumnCompare( DestAccessTime, pred::NewPropertyComparator< CTouchItem >( func::AsDestAccessTime() ), false );
	m_fileListCtrl.AddColumnCompare( SrcModifyTime, pred::NewPropertyComparator< CTouchItem >( func::AsSrcModifyTime() ), false );
	m_fileListCtrl.AddColumnCompare( SrcCreationTime, pred::NewPropertyComparator< CTouchItem >( func::AsSrcCreationTime() ), false );
	m_fileListCtrl.AddColumnCompare( SrcAccessTime, pred::NewPropertyComparator< CTouchItem >( func::AsSrcAccessTime() ), false );
}

CFindDuplicatesDialog::~CFindDuplicatesDialog()
{
}

bool CFindDuplicatesDialog::TouchFiles( void )
{
	CFileService svc;
	std::auto_ptr< CMacroCommand > pTouchMacroCmd = svc.MakeTouchCmds( m_rTouchItems );
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

void CFindDuplicatesDialog::SwitchMode( Mode mode )
{
	m_mode = mode;
	if ( NULL == m_hWnd )
		return;

	static const CEnumTags modeTags( _T("&Store|&Touch|Roll &Back|Roll &Forward") );
	ui::SetDlgItemText( m_hWnd, IDOK, modeTags.FormatUi( m_mode ) );

	static const UINT ctrlIds[] =
	{
		IDC_COPY_SOURCE_PATHS_BUTTON, IDC_PASTE_FILES_BUTTON, IDC_RESET_FILES_BUTTON,
		//IDC_MODIFIED_DATE, IDC_CREATED_DATE, IDC_ACCESSED_DATE,
		//IDC_ATTRIB_READONLY_CHECK, IDC_ATTRIB_HIDDEN_CHECK, IDC_ATTRIB_SYSTEM_CHECK, IDC_ATTRIB_ARCHIVE_CHECK
	};
	ui::EnableControls( *this, ctrlIds, COUNT_OF( ctrlIds ), !IsRollMode() );

	m_anyChanges = utl::Any( m_rTouchItems, std::mem_fun( &CTouchItem::IsModified ) );
	ui::EnableControl( *this, IDOK, m_mode != CommitFilesMode || m_anyChanges );

	m_fileListCtrl.Invalidate();			// do some custom draw magic
}

void CFindDuplicatesDialog::PostMakeDest( bool silent /*= false*/ )
{
	if ( !silent )
		GotoDlgCtrl( GetDlgItem( IDOK ) );

	EnsureVisibleFirstError();
	SwitchMode( CommitFilesMode );
}

void CFindDuplicatesDialog::PopStackTop( cmd::StackType stackType )
{
	ASSERT( !IsRollMode() );

	if ( utl::ICommand* pTopCmd = PeekCmdForDialog( stackType ) )		// comand that is target for this dialog editor?
	{
		bool isTouchMacro = cmd::TouchFile == pTopCmd->GetTypeID();

		ClearFileErrors();

		if ( isTouchMacro )
			m_pFileModel->FetchFromStack( stackType );		// fetch dataset from the stack top macro command
		else
			m_pFileModel->UndoRedo( stackType );

		MarkInvalidSrcItems();

		if ( isTouchMacro )							// file command?
			SwitchMode( cmd::Undo == stackType ? RollBackMode : RollForwardMode );
		else if ( IsNativeCmd( pTopCmd ) )			// file state editing command?
			SwitchMode( CommitFilesMode );
	}
	else
		PopStackRunCrossEditor( stackType );		// end this dialog and execute the target dialog editor
}

void CFindDuplicatesDialog::SetupDialog( void )
{
	SetupFileListView();							// fill in and select the found files list
}

void CFindDuplicatesDialog::SetupFileListView( void )
{
	CScopedListTextSelection sel( &m_fileListCtrl );

	CScopedLockRedraw freeze( &m_fileListCtrl );
	CScopedInternalChange internalChange( &m_fileListCtrl );

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

	m_fileListCtrl.SetupDiffColumnPair( SrcAttributes, DestAttributes, str::GetMatch() );
	m_fileListCtrl.SetupDiffColumnPair( SrcModifyTime, DestModifyTime, str::GetMatch() );
	m_fileListCtrl.SetupDiffColumnPair( SrcCreationTime, DestCreationTime, str::GetMatch() );
	m_fileListCtrl.SetupDiffColumnPair( SrcAccessTime, DestAccessTime, str::GetMatch() );

	m_fileListCtrl.InitialSortList();
}

void CFindDuplicatesDialog::InputFields( void )
{
}

void CFindDuplicatesDialog::ApplyFields( void )
{
	if ( utl::ICommand* pCmd = MakeChangeDestFileStatesCmd() )
	{
		ClearFileErrors();
		SafeExecuteCmd( pCmd );
	}
}

utl::ICommand* CFindDuplicatesDialog::MakeChangeDestFileStatesCmd( void )
{
	std::vector< fs::CFileState > destFileStates; destFileStates.reserve( m_rTouchItems.size() );
	bool anyChanges = false;

	// apply valid edits, i.e. if not null
	for ( size_t i = 0; i != m_rTouchItems.size(); ++i )
	{
		const CTouchItem* pTouchItem = m_rTouchItems[ i ];

		fs::CFileState newFileState = pTouchItem->GetDestState();

		if ( newFileState != pTouchItem->GetDestState() )
			anyChanges = true;

		destFileStates.push_back( newFileState );
	}

	return anyChanges ? new CChangeDestFileStatesCmd( m_pFileModel, destFileStates ) : NULL;
}

void CFindDuplicatesDialog::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage )
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

void CFindDuplicatesDialog::ClearFileErrors( void )
{
	m_errorItems.clear();

	if ( m_hWnd != NULL )
		m_fileListCtrl.Invalidate();
}

void CFindDuplicatesDialog::OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg )
{
	errMsg;

	if ( CTouchItem* pErrorItem = FindItemWithKey( srcPath ) )
		utl::AddUnique( m_errorItems, pErrorItem );

	EnsureVisibleFirstError();
}

void CFindDuplicatesDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const
{
	static const ui::CTextEffect s_modPathName( ui::Bold );
	static const ui::CTextEffect s_modDest( ui::Regular, CReportListControl::s_mismatchDestTextColor );
	static const ui::CTextEffect s_modSrc( ui::Regular, CReportListControl::s_deleteSrcTextColor );
	static const ui::CTextEffect s_errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );

	const CTouchItem* pTouchItem = CReportListControl::AsPtr< CTouchItem >( rowKey );
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
				//wouldModify = ...;
				break;
		}

		if ( wouldModify )
			rTextEffect.m_textColor = ui::GetBlendedColor( rTextEffect.m_textColor != CLR_NONE ? rTextEffect.m_textColor : m_fileListCtrl.GetTextColor(), color::White );		// blend to gray
	}
}

void CFindDuplicatesDialog::ModifyDiffTextEffectAt( CListTraits::CMatchEffects& rEffects, LPARAM rowKey, int subItem ) const
{
	rowKey;
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

CTouchItem* CFindDuplicatesDialog::FindItemWithKey( const fs::CPath& keyPath ) const
{
	std::vector< CTouchItem* >::const_iterator itFoundItem = utl::BinaryFind( m_rTouchItems, keyPath, CPathItemBase::ToKeyPath() );
	return itFoundItem != m_rTouchItems.end() ? *itFoundItem : NULL;
}

void CFindDuplicatesDialog::MarkInvalidSrcItems( void )
{
	for ( std::vector< CTouchItem* >::const_iterator itTouchItem = m_rTouchItems.begin(); itTouchItem != m_rTouchItems.end(); ++itTouchItem )
		if ( !( *itTouchItem )->GetKeyPath().FileExist() )
			utl::AddUnique( m_errorItems, *itTouchItem );
}

void CFindDuplicatesDialog::EnsureVisibleFirstError( void )
{
	if ( const CTouchItem* pFirstErrorItem = GetFirstErrorItem< CTouchItem >() )
		m_fileListCtrl.EnsureVisibleObject( pFirstErrorItem );

	m_fileListCtrl.Invalidate();				// trigger some highlighting
}

BOOL CFindDuplicatesDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		m_fileListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CFindDuplicatesDialog::DoDataExchange( CDataExchange* pDX )
{
	const bool firstInit = NULL == m_fileListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_DUPLICATE_FILES_LIST, m_fileListCtrl );

	ui::DDX_ButtonIcon( pDX, IDC_COPY_SOURCE_PATHS_BUTTON, ID_EDIT_COPY );
	ui::DDX_ButtonIcon( pDX, IDC_PASTE_FILES_BUTTON, ID_EDIT_PASTE );
	ui::DDX_ButtonIcon( pDX, IDC_RESET_FILES_BUTTON, ID_RESET_DEFAULT );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			OnUpdate( m_pFileModel, NULL );
			SwitchMode( m_mode );
		}
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CFindDuplicatesDialog, CFileEditorBaseDialog )
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI_RANGE( IDC_UNDO_BUTTON, IDC_REDO_BUTTON, OnUpdateUndoRedo )
	ON_BN_CLICKED( IDC_COPY_SOURCE_PATHS_BUTTON, OnBnClicked_CopySourceFiles )
	ON_BN_CLICKED( IDC_PASTE_FILES_BUTTON, OnBnClicked_PasteDestStates )
	ON_BN_CLICKED( IDC_RESET_FILES_BUTTON, OnBnClicked_ResetDestFiles )
	//ON_UPDATE_COMMAND_UI_RANGE( ID_COPY_MODIFIED_DATE, ID_COPY_ACCESSED_DATE, OnUpdateSelListItem )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_DUPLICATE_FILES_LIST, OnLvnItemChanged_TouchList )
END_MESSAGE_MAP()

void CFindDuplicatesDialog::OnOK( void )
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

			if ( m_pFileModel->UndoRedo( RollBackMode == m_mode ? cmd::Undo : cmd::Redo ) ||
				 PromptCloseDialog( PromptClose ) )
				__super::OnOK();
			else
				SwitchMode( EditMode );
			break;
		}
	}
}

void CFindDuplicatesDialog::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( &m_fileListCtrl == pWnd )
	{
		CMenu popupMenu;
		ui::LoadPopupMenu( popupMenu, IDR_CONTEXT_MENU, popup::TouchList );
		ui::TrackPopupMenu( popupMenu, this, screenPos );
		return;					// supress rising WM_CONTEXTMENU to the parent
	}

	__super::OnContextMenu( pWnd, screenPos );
}

void CFindDuplicatesDialog::OnUpdateUndoRedo( CCmdUI* pCmdUI )
{
	switch ( pCmdUI->m_nID )
	{
		case IDC_UNDO_BUTTON:
			pCmdUI->Enable( !IsRollMode() && m_pFileModel->CanUndoRedo( cmd::Undo ) );
			break;
		case IDC_REDO_BUTTON:
			pCmdUI->Enable( !IsRollMode() && m_pFileModel->CanUndoRedo( cmd::Redo ) );
			break;
	}
}

void CFindDuplicatesDialog::OnFieldChanged( void )
{
	SwitchMode( EditMode );
}

void CFindDuplicatesDialog::OnBnClicked_CopySourceFiles( void )
{
	if ( !m_pFileModel->CopyClipSourceFileStates( this ) )
		AfxMessageBox( _T("Cannot copy source file states to clipboard!"), MB_ICONERROR | MB_OK );
}

void CFindDuplicatesDialog::OnBnClicked_PasteDestStates( void )
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

void CFindDuplicatesDialog::OnBnClicked_ResetDestFiles( void )
{
	ClearFileErrors();
	SafeExecuteCmd( new CResetDestinationsCmd( m_pFileModel ) );
}

void CFindDuplicatesDialog::OnUpdateSelListItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_fileListCtrl.GetCurSel() != -1 );
}

void CFindDuplicatesDialog::OnLvnItemChanged_TouchList( NMHDR* pNmHdr, LRESULT* pResult )
{
	NM_LISTVIEW* pNmList = (NM_LISTVIEW*)pNmHdr;

	if ( CReportListControl::IsSelectionChangedNotify( pNmList, LVIS_SELECTED | LVIS_FOCUSED ) )
	{
		//UpdateFieldsFromSel( m_fileListCtrl.GetCurSel() );
	}

	*pResult = 0;
}
