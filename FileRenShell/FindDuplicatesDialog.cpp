
#include "stdafx.h"
#include "FindDuplicatesDialog.h"
#include "DuplicateFileItem.h"
#include "FileModel.h"
#include "FileService.h"
#include "FileCommands.h"
#include "GeneralOptions.h"
#include "Application.h"
#include "resource.h"
#include "utl/Clipboard.h"
#include "utl/Color.h"
#include "utl/ContainerUtilities.h"
#include "utl/CRC32.h"
#include "utl/CmdInfoStore.h"
#include "utl/Command.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"
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
	enum { TopPct = 30, BottomPct = 100 - TopPct };

	static CLayoutStyle styles[] =
	{
		{ IDC_GROUP_BOX_1, SizeX | pctSizeY( TopPct ) },
		{ IDC_SOURCE_PATHS_LIST, SizeX | pctSizeY( TopPct ) },
		{ IDC_STRIP_BAR_1, MoveX | pctMoveY( TopPct ) },

		{ IDC_FILE_TYPE_STATIC, pctMoveY( TopPct ) },
		{ IDC_FILE_TYPE_COMBO, pctMoveY( TopPct ) },
		{ IDC_MINIMUM_SIZE_STATIC, pctMoveY( TopPct ) },
		{ IDC_MINIMUM_SIZE_COMBO, pctMoveY( TopPct ) },
		{ IDC_FILE_SPEC_STATIC, pctMoveY( TopPct ) },
		{ IDC_FILE_SPEC_EDIT, SizeX | pctMoveY( TopPct ) },

		{ IDC_DUPLICATE_FILES_STATIC, pctMoveY( TopPct ) },
		{ IDC_DUPLICATE_FILES_LIST, SizeX | pctMoveY( TopPct ) | pctSizeY( BottomPct ) },

		{ IDC_SELECT_DUPLICATES_BUTTON, MoveY },
		{ IDC_DELETE_DUPLICATES_BUTTON, MoveY },
		{ IDC_MOVE_DUPLICATES_BUTTON, MoveY },
		{ IDC_CLEAR_CRC32_CACHE_BUTTON, Move },

		{ IDOK, MoveX },
		{ IDCANCEL, MoveX },
		{ IDC_TOOLBAR_PLACEHOLDER, MoveX }
	};
}


CFindDuplicatesDialog::CFindDuplicatesDialog( CFileModel* pFileModel, CWnd* pParent )
	: CFileEditorBaseDialog( pFileModel, cmd::FindDuplicates, IDD_FIND_DUPLICATES_DIALOG, pParent )
	, m_sourcePaths( m_pFileModel->GetSourcePaths() )
	, m_dupsListCtrl( IDC_DUPLICATE_FILES_LIST )
	, m_anyChanges( false )
{
	m_nativeCmdTypes.push_back( cmd::ResetDestinations );
	m_mode = CommitFilesMode;
	REQUIRE( !m_sourcePaths.empty() );

	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	LoadDlgIcon( ID_TOUCH_FILES );

	m_srcPathsListCtrl.SetAcceptDropFiles();
	m_srcPathsListCtrl.AddRecordCompare( pred::NewComparator( pred::CompareCode() ) );	// default row item comparator
	CGeneralOptions::Instance().ApplyToListCtrl( &m_srcPathsListCtrl );

	m_dupsListCtrl.ModifyListStyleEx( 0, LVS_EX_CHECKBOXES );
	m_dupsListCtrl.SetSection( m_regSection + _T("\\List") );
	m_dupsListCtrl.SetUseAlternateRowColoring();
	m_dupsListCtrl.SetTextEffectCallback( this );
	m_dupsListCtrl.SetPopupMenu( CReportListControl::OnSelection, NULL );				// let us track a custom menu
	CGeneralOptions::Instance().ApplyToListCtrl( &m_dupsListCtrl );

	m_dupsListCtrl.AddRecordCompare( pred::NewComparator( pred::CompareCode() ) );		// default row item comparator
	m_dupsListCtrl.AddColumnCompare( FileName, pred::NewComparator( pred::CompareDisplayCode() ) );
}

CFindDuplicatesDialog::~CFindDuplicatesDialog()
{
	ClearDuplicates();
}

const CEnumTags& CFindDuplicatesDialog::GetTags_FileType( void )
{
	static const CEnumTags tags(
		_T("*.*|")
		_T("*.jpeg;*.jpg;*.jpe;*.jp2;*.gif;*.bmp;*.pcx;*.png;*.tif;*.tiff;*.dib;*.ico;*.wmf;*.emf;*.tga;*.psd;*.rle|")
		_T("*.wav;*.mp3;*.m4a;*.flac;*.wma;*.aac;*.ac3;*.midi;*.mid;*.ogg;*.rmi;*.snd|")
		_T("*.avi;*.mpeg;*.mpg;*.wmv;*.m1v;*.m2v;*.mp1;*.mp2;*.mpv2;*.mp2v;*.divx|")
		_T("*.ext1;*.ext2;*.ext3")
	);
	return tags;
}

void CFindDuplicatesDialog::ClearDuplicates( void )
{
	utl::ClearOwningContainer( m_duplicateGroups );
}

void CFindDuplicatesDialog::SearchForDuplicateFiles( void )
{
	ClearDuplicates();

	std::tstring wildSpec = ui::GetDlgItemText( this, IDC_FILE_SPEC_EDIT );

	CDuplicateGroupsStore groupsStore;

	for ( std::vector< fs::CPath >::const_iterator itSrcPath = m_sourcePaths.begin(); itSrcPath != m_sourcePaths.end(); ++itSrcPath )
	{
		if ( fs::IsValidDirectory( itSrcPath->GetPtr() ) )
		{
			fs::CPathEnumerator found;
			fs::EnumFiles( &found, itSrcPath->GetPtr(), wildSpec.c_str(), Deep );

			for ( fs::TPathSet::const_iterator itFilePath = found.m_filePaths.begin(); itFilePath != found.m_filePaths.end(); ++itFilePath )
				groupsStore.Register( *itFilePath );
		}
		else if ( fs::IsValidFile( itSrcPath->GetPtr() ) )
			groupsStore.Register( *itSrcPath );
	}

	groupsStore.ExtractDuplicateGroups( m_duplicateGroups );

	ClearFileErrors();
}

bool CFindDuplicatesDialog::DeleteDuplicateFiles( void )
{
/*	CFileService svc;
	std::auto_ptr< CMacroCommand > pTouchMacroCmd = svc.MakeTouchCmds( m_rTouchItems );
	if ( pTouchMacroCmd.get() != NULL )
		if ( !pTouchMacroCmd->IsEmpty() )
		{
			ClearFileErrors();

			cmd::CScopedErrorObserver observe( this );
			return SafeExecuteCmd( pTouchMacroCmd.release() );
		}
		else
			return PromptCloseDialog();*/
	return false;
}

void CFindDuplicatesDialog::SwitchMode( Mode mode )
{
	m_mode = mode;
	if ( NULL == m_hWnd )
		return;

	static const CEnumTags modeTags( _T("&Search|Delete...|Roll &Back|Roll &Forward") );
	ui::SetDlgItemText( m_hWnd, IDOK, modeTags.FormatUi( m_mode ) );

	static const UINT ctrlIds[] =
	{
		IDC_GROUP_BOX_1, IDC_SOURCE_PATHS_LIST, IDC_FILE_TYPE_STATIC, IDC_FILE_TYPE_COMBO, IDC_MINIMUM_SIZE_STATIC, IDC_MINIMUM_SIZE_COMBO, IDC_FILE_SPEC_STATIC, IDC_FILE_SPEC_EDIT,
		IDC_SELECT_DUPLICATES_BUTTON, IDC_DELETE_DUPLICATES_BUTTON, IDC_MOVE_DUPLICATES_BUTTON, IDC_CLEAR_CRC32_CACHE_BUTTON
	};
	ui::EnableControls( *this, ctrlIds, COUNT_OF( ctrlIds ), !IsRollMode() );
	if ( IsRollMode() )
		m_anyChanges = false;

	ui::EnableControl( *this, IDOK, m_mode != CommitFilesMode || m_anyChanges );

	m_dupsListCtrl.Invalidate();			// do some custom draw magic
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
		bool isTouchMacro = cmd::FindDuplicates == pTopCmd->GetTypeID();

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
	SetupSrcPathsList();
}

void CFindDuplicatesDialog::SetupSrcPathsList( void )
{
	CScopedListTextSelection sel( &m_srcPathsListCtrl );
	CScopedLockRedraw freeze( &m_srcPathsListCtrl );
	CScopedInternalChange internalChange( &m_srcPathsListCtrl );

	m_srcPathsListCtrl.DeleteAllItems();

	for ( unsigned int index = 0; index != m_sourcePaths.size(); ++index )
	{
		const fs::CPath& srcPath = m_sourcePaths[ index ];
		m_srcPathsListCtrl.InsertItem( LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM, index, srcPath.GetPtr(), 0, 0, CReportListControl::Transparent_Image, reinterpret_cast< LPARAM >( srcPath.GetPtr() ) );
	}
}

void CFindDuplicatesDialog::SetupDuplicateFileList( void )
{
	CScopedListTextSelection sel( &m_dupsListCtrl );

	CScopedLockRedraw freeze( &m_dupsListCtrl );
	CScopedInternalChange internalChange( &m_dupsListCtrl );

	m_dupsListCtrl.DeleteAllItems();
	m_dupsListCtrl.RemoveAllGroups();

	m_dupsListCtrl.EnableGroupView( !m_duplicateGroups.empty() );

	unsigned int index = 0;			// strictly item index

	for ( size_t groupPos = 0; groupPos != m_duplicateGroups.size(); ++groupPos )
	{
		const CDuplicateFilesGroup* pGroup = m_duplicateGroups[ groupPos ];
		ASSERT( pGroup->GetItems().size() > 1 );

		for ( std::vector< CDuplicateFileItem* >::const_iterator itDupItem = pGroup->GetItems().begin(); itDupItem != pGroup->GetItems().end(); ++itDupItem, ++index )
		{
			ASSERT( pGroup == ( *itDupItem )->GetParentGroup() );
			m_dupsListCtrl.InsertObjectItem( index, *itDupItem );		// PathName
			m_dupsListCtrl.SetSubItemText( index, DirPath, ( *itDupItem )->GetKeyPath().GetParentPath().GetPtr() );
			m_dupsListCtrl.SetSubItemText( index, Size, num::FormatFileSize( pGroup->GetContentKey().m_fileSize ) );
			m_dupsListCtrl.SetSubItemText( index, CRC32, num::FormatHexNumber( pGroup->GetContentKey().m_crc32, _T("%X") ) );
		}
	}
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
		{
			CGeneralOptions::Instance().ApplyToListCtrl( &m_srcPathsListCtrl );
			CGeneralOptions::Instance().ApplyToListCtrl( &m_dupsListCtrl );
		}
}

void CFindDuplicatesDialog::ClearFileErrors( void )
{
	m_errorItems.clear();

	if ( m_hWnd != NULL )
		m_dupsListCtrl.Invalidate();
}

void CFindDuplicatesDialog::OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg )
{
	errMsg;

	if ( CDuplicateFileItem* pErrorItem = FindItemWithKey( srcPath ) )
		utl::AddUnique( m_errorItems, pErrorItem );

	EnsureVisibleFirstError();
}

void CFindDuplicatesDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const
{
	static const ui::CTextEffect s_dup( ui::Bold );
	static const ui::CTextEffect s_errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );

	const CDuplicateFileItem* pFileItem = CReportListControl::AsPtr< CDuplicateFileItem >( rowKey );
	const ui::CTextEffect* pTextEffect = NULL;

	switch ( subItem )
	{
		case FileName:
		case DirPath:
			if ( pFileItem->IsDuplicateItem() )
				pTextEffect = &s_dup;
			break;
	}

	if ( utl::Contains( m_errorItems, pFileItem ) )
		rTextEffect |= s_errorBk;							// highlight error row background

	if ( pTextEffect != NULL )
		rTextEffect |= *pTextEffect;
}

CDuplicateFileItem* CFindDuplicatesDialog::FindItemWithKey( const fs::CPath& keyPath ) const
{
	for ( std::vector< CDuplicateFilesGroup* >::const_iterator itGroup = m_duplicateGroups.begin(); itGroup != m_duplicateGroups.end(); ++itGroup )
		if ( CDuplicateFileItem* pFoundItem = ( *itGroup )->FindItem( keyPath ) )
			return pFoundItem;

	return NULL;
}

void CFindDuplicatesDialog::MarkInvalidSrcItems( void )
{
	for ( std::vector< CDuplicateFilesGroup* >::const_iterator itGroup = m_duplicateGroups.begin(); itGroup != m_duplicateGroups.end(); ++itGroup )
		for ( std::vector< CDuplicateFileItem* >::const_iterator itItem = ( *itGroup )->GetItems().begin(); itItem != ( *itGroup )->GetItems().end(); ++itItem )
			if ( !( *itItem )->GetKeyPath().FileExist() )
				utl::AddUnique( m_errorItems, *itItem );
}

void CFindDuplicatesDialog::EnsureVisibleFirstError( void )
{
	if ( const CDuplicateFileItem* pFirstErrorItem = GetFirstErrorItem< CDuplicateFileItem >() )
		m_dupsListCtrl.EnsureVisibleObject( pFirstErrorItem );

	m_dupsListCtrl.Invalidate();				// trigger some highlighting
}

BOOL CFindDuplicatesDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		m_dupsListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CFindDuplicatesDialog::DoDataExchange( CDataExchange* pDX )
{
	const bool firstInit = NULL == m_srcPathsListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_SOURCE_PATHS_LIST, m_srcPathsListCtrl );
	DDX_Control( pDX, IDC_DUPLICATE_FILES_LIST, m_dupsListCtrl );

	//ui::DDX_ButtonIcon( pDX, IDC_SELECT_DUPLICATES_BUTTON, ID_EDIT_COPY );
	ui::DDX_ButtonIcon( pDX, IDC_DELETE_DUPLICATES_BUTTON, ID_REMOVE_ALL_ITEMS );
	//ui::DDX_ButtonIcon( pDX, IDC_MOVE_DUPLICATES_BUTTON, ID_RESET_DEFAULT );

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
	ON_NOTIFY( CReportListControl::LVN_DropFiles, IDC_SOURCE_PATHS_LIST, OnLvnDropFiles_SrcList )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_DUPLICATE_FILES_LIST, OnLvnItemChanged_TouchList )
END_MESSAGE_MAP()

void CFindDuplicatesDialog::OnOK( void )
{
	switch ( m_mode )
	{
		case EditMode:
			SearchForDuplicateFiles();
			PostMakeDest();
			break;
		case CommitFilesMode:
			if ( DeleteDuplicateFiles() )
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
	if ( &m_dupsListCtrl == pWnd )
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
	pCmdUI->Enable( m_dupsListCtrl.GetCurSel() != -1 );
}

void CFindDuplicatesDialog::OnLvnDropFiles_SrcList( NMHDR* pNmHdr, LRESULT* pResult )
{
	CNmDropFiles* pNmDropFiles = (CNmDropFiles*)pNmHdr;
	*pResult = 0;

	for ( std::vector< std::tstring >::const_iterator itFilePath = pNmDropFiles->m_filePaths.begin(); itFilePath != pNmDropFiles->m_filePaths.end(); ++itFilePath )
		utl::AddUnique( m_sourcePaths, *itFilePath );

	SwitchMode( EditMode );
}

void CFindDuplicatesDialog::OnLvnItemChanged_TouchList( NMHDR* pNmHdr, LRESULT* pResult )
{
	NM_LISTVIEW* pNmList = (NM_LISTVIEW*)pNmHdr;

	if ( CReportListControl::IsSelectionChangedNotify( pNmList, LVIS_SELECTED | LVIS_FOCUSED ) )
	{
		//UpdateFieldsFromSel( m_dupsListCtrl.GetCurSel() );
	}

	*pResult = 0;
}
