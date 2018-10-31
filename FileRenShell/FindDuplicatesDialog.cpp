
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
#include "utl/Crc32.h"
#include "utl/CmdInfoStore.h"
#include "utl/Command.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"
#include "utl/FmtUtils.h"
#include "utl/Guards.h"
#include "utl/ItemListDialog.h"
#include "utl/LongestCommonSubsequence.h"
#include "utl/ProgressDialog.h"
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


// CDuplicatesProgress class

class CDuplicatesProgress : private fs::IEnumerator
{
public:
	CDuplicatesProgress( CWnd* pParent );
	~CDuplicatesProgress();

	ui::IProgressBox* GetProgressBox( void ) { return &m_dlg; }
	fs::IEnumerator* GetProgressEnumerator( void ) { return this; }

	void EnterSection_ComputeFileSize( size_t fileCount );
	void EnterSection_ComputeCrc32( size_t itemCount );

	// file enumerator callback
	virtual void AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException );
	virtual void AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException );
private:
	CProgressDialog m_dlg;
};


// CDuplicatesProgress implementation

CDuplicatesProgress::CDuplicatesProgress( CWnd* pParent )
	: m_dlg( _T("Search for Duplicate Files"), CProgressDialog::LabelsCount )
{
	if ( m_dlg.Create( _T("Duplicate Files Search"), pParent ) )
	{
		m_dlg.SetStageLabel( _T("Search directory") );
		m_dlg.SetStepLabel( _T("Found file") );
		m_dlg.SetMarqueeProgress();
	}
}

CDuplicatesProgress::~CDuplicatesProgress()
{
	m_dlg.DestroyWindow();
}

void CDuplicatesProgress::EnterSection_ComputeFileSize( size_t fileCount )
{
	m_dlg.SetOperationLabel( _T("Compute Size of Files") );
	m_dlg.ShowStage( false );
	m_dlg.SetStepLabel( _T("Compute file size") );
	m_dlg.SetProgressItemCount( fileCount );
}

void CDuplicatesProgress::EnterSection_ComputeCrc32( size_t itemCount )
{
	m_dlg.SetOperationLabel( _T("Compute CRC32 Checksums") );
	m_dlg.SetStageLabel( _T("Group of duplicates") );
	m_dlg.SetStepLabel( _T("Compute file CRC32") );
	m_dlg.SetProgressItemCount( itemCount );
	m_dlg.SetProgressStep( 1 );			// advance progress on each step since individual computations are slow
}

void CDuplicatesProgress::AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException )
{
	m_dlg.AdvanceStepItem( pFilePath );
}

void CDuplicatesProgress::AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException )
{
	m_dlg.AdvanceStage( pSubDirPath );
}


// CFindDuplicatesDialog implementation

namespace reg
{
	static const TCHAR section_dialog[] = _T("FindDuplicatesDialog");
	static const TCHAR entry_minFileSize[] = _T("minFileSize");
	static const TCHAR entry_fileType[] = _T("fileType");
	static const TCHAR entry_fileTypeSpecs[] = _T("fileTypeSpecs");
}

namespace layout
{
	enum { TopPct = 30, BottomPct = 100 - TopPct };

	static CLayoutStyle styles[] =
	{
		{ IDC_GROUP_BOX_1, SizeX | pctSizeY( TopPct ) },
		{ IDC_SOURCE_PATHS_LIST, SizeX | pctSizeY( TopPct ) },
		{ IDC_STRIP_BAR_1, MoveX },

		{ IDC_FILE_TYPE_STATIC, pctMoveY( TopPct ) },
		{ IDC_FILE_TYPE_COMBO, pctMoveY( TopPct ) },
		{ IDC_MINIMUM_SIZE_STATIC, pctMoveY( TopPct ) },
		{ IDC_MIN_FILE_SIZE_COMBO, pctMoveY( TopPct ) },
		{ IDC_FILE_SPEC_STATIC, pctMoveY( TopPct ) },
		{ IDC_FILE_SPEC_EDIT, SizeX | pctMoveY( TopPct ) },

		{ IDC_DUPLICATE_FILES_STATIC, pctMoveY( TopPct ) },
		{ IDC_DUPLICATE_FILES_INFO, SizeX | pctMoveY( TopPct ) },
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
	, m_srcPathsListCtrl( IDC_SOURCE_PATHS_LIST )
	, m_dupsListCtrl( IDC_DUPLICATE_FILES_LIST )
{
	for ( std::vector< fs::CPath >::const_iterator itSrcPath = m_pFileModel->GetSourcePaths().begin(); itSrcPath != m_pFileModel->GetSourcePaths().end(); ++itSrcPath )
		m_srcPathItems.push_back( new CSrcPathItem( *itSrcPath ) );

	m_nativeCmdTypes.push_back( cmd::ResetDestinations );
	REQUIRE( !m_srcPathItems.empty() );

	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	LoadDlgIcon( ID_TOUCH_FILES );

	m_srcPathsListCtrl.SetAcceptDropFiles();
	m_srcPathsListCtrl.AddRecordCompare( pred::NewComparator( pred::CompareCode() ) );	// default row item comparator
	CGeneralOptions::Instance().ApplyToListCtrl( &m_srcPathsListCtrl );

	m_dupsListCtrl.ModifyListStyleEx( 0, LVS_EX_CHECKBOXES );
	m_dupsListCtrl.SetSection( m_regSection + _T("\\List") );
	m_dupsListCtrl.SetTextEffectCallback( this );
	m_dupsListCtrl.SetPopupMenu( CReportListControl::OnSelection, NULL );				// let us track a custom menu
	CGeneralOptions::Instance().ApplyToListCtrl( &m_dupsListCtrl );

	m_dupsListCtrl.AddRecordCompare( pred::NewComparator( pred::CompareCode() ) );		// default row item comparator
	m_dupsListCtrl.AddColumnCompare( FileName, pred::NewComparator( pred::CompareDisplayCode() ) );
	m_dupsListCtrl.AddColumnCompare( DateModified, pred::NewPropertyComparator< CDuplicateFileItem >( CDuplicateFileItem::AsModifyTime() ), false );		// order date-time descending by default

	m_srcPathsToolbar.GetStrip().AddButton( ID_EDIT_LIST_ITEMS );

	m_minFileSizeCombo.SetItemSep( _T("|") );

	str::Split( m_fileTypeSpecs,
		AfxGetApp()->GetProfileString( reg::section_dialog, reg::entry_fileTypeSpecs, str::Join( GetTags_FileType().GetUiTags(), _T("|") ).c_str() ).GetString(),
		_T("|") );
}

CFindDuplicatesDialog::~CFindDuplicatesDialog()
{
	utl::ClearOwningContainer( m_srcPathItems );
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

// optimize performance:
//	step 1: compute file-size part of the content key, grouping duplicate candidates by file-size only
//	step 2: compute CRC32: real duplicates are within size-based duplicate groups
//
bool CFindDuplicatesDialog::SearchForDuplicateFiles( void )
{
	CWaitCursor wait;
	ULONGLONG minFileSize = 0;
	if ( num::ParseNumber( minFileSize, m_minFileSizeCombo.GetCurrentText() ) )
		minFileSize *= 1024;		// to KB

	CDuplicatesProgress progress( this );
	ui::IProgressBox* pProgressBox = progress.GetProgressBox();

	try
	{
		std::auto_ptr< utl::CSectionGuard > pSection( new utl::CSectionGuard( _T("# SearchForFiles") ) );

		std::vector< fs::CPath > foundPaths;
		SearchForFiles( foundPaths, progress.GetProgressEnumerator() );

		pSection.reset( new utl::CSectionGuard( _T("# File-size grouping") ) );

		CDuplicateGroupsStore groupsStore;
		size_t ignoredCount = 0;

		progress.EnterSection_ComputeFileSize( foundPaths.size() );

		for ( size_t i = 0; i != foundPaths.size(); ++i )
		{
			const fs::CPath& filePath = foundPaths[ i ];

			CFileContentKey contentKey;
			bool registered = false;

			if ( contentKey.ComputeFileSize( filePath ) )
				if ( contentKey.m_fileSize >= minFileSize )				// has minimum size?
					registered = groupsStore.RegisterPath( filePath, contentKey );

			if ( !registered )
				++ignoredCount;

			pProgressBox->AdvanceStepItem( filePath.Get() );
		}

		pSection.reset( new utl::CSectionGuard( _T("# ExtractDuplicateGroups w. CRC32") ) );
		progress.EnterSection_ComputeCrc32( groupsStore.GetTotalDuplicateItemCount() );		// count of duplicate candidates

		utl::COwningContainer< std::vector< CDuplicateFilesGroup* > > newDuplicateGroups;
		groupsStore.ExtractDuplicateGroups( newDuplicateGroups, ignoredCount, pProgressBox );

		m_duplicateGroups.swap( newDuplicateGroups );		// swap items and ownership

		std::tstring reportMessage = str::Format( _T("Found %d duplicates of total %d files"), m_dupsListCtrl.GetItemCount(), foundPaths.size() );
		if ( ignoredCount != 0 )
			reportMessage += str::Format( _T(" (ignored %d)"), ignoredCount );

		ui::SetDlgItemText( this, IDC_DUPLICATE_FILES_INFO, reportMessage );

		SetupDuplicateFileList();
		ClearFileErrors();
		return true;
	}
	catch ( CUserAbortedException& exc )
	{
		app::TraceException( exc );
		return false;			// cancelled by the user
	}
}

#include <hash_set>

void CFindDuplicatesDialog::SearchForFiles( std::vector< fs::CPath >& rFoundPaths, fs::IEnumerator* pProgressEnum ) const
{
	std::tstring wildSpec = m_fileSpecEdit.GetText();
	stdext::hash_set< fs::CPath > uniquePaths;

	for ( std::vector< CSrcPathItem* >::const_iterator itSrcPathItem = m_srcPathItems.begin(); itSrcPathItem != m_srcPathItems.end(); ++itSrcPathItem )
	{
		const fs::CPath& srcPath = ( *itSrcPathItem )->GetKeyPath();
		if ( fs::IsValidDirectory( srcPath.GetPtr() ) )
		{
			fs::CPathEnumerator found( pProgressEnum );
			fs::EnumFiles( &found, srcPath.GetPtr(), wildSpec.c_str(), Deep );

			rFoundPaths.reserve( rFoundPaths.size() + found.m_filePaths.size() );
			for ( fs::TPathSet::const_iterator itFilePath = found.m_filePaths.begin(); itFilePath != found.m_filePaths.end(); ++itFilePath )
				if ( uniquePaths.insert( *itFilePath ).second )		// path is unique?
					rFoundPaths.push_back( *itFilePath );
		}
		else if ( fs::IsValidFile( srcPath.GetPtr() ) )
			if ( uniquePaths.insert( srcPath ).second )				// path is unique?
				rFoundPaths.push_back( srcPath );
	}
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
		IDC_GROUP_BOX_1, IDC_SOURCE_PATHS_LIST,
		IDC_FILE_TYPE_STATIC, IDC_FILE_TYPE_COMBO, IDC_MINIMUM_SIZE_STATIC, IDC_MIN_FILE_SIZE_COMBO, IDC_FILE_SPEC_STATIC, IDC_FILE_SPEC_EDIT,
		IDC_SELECT_DUPLICATES_BUTTON, IDC_DELETE_DUPLICATES_BUTTON, IDC_MOVE_DUPLICATES_BUTTON, IDC_CLEAR_CRC32_CACHE_BUTTON
	};
	ui::EnableControls( *this, ctrlIds, COUNT_OF( ctrlIds ), !IsRollMode() );

//	if ( IsRollMode() )
//		m_anyChanges = false;
	//ui::EnableControl( *this, IDOK, m_mode != CommitFilesMode || m_anyChanges );

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

	for ( unsigned int index = 0; index != m_srcPathItems.size(); ++index )
		m_srcPathsListCtrl.InsertObjectItem( index, m_srcPathItems[ index ] );
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

	for ( unsigned int groupId = 0; groupId != m_duplicateGroups.size(); ++groupId )
	{
		const CDuplicateFilesGroup* pGroup = m_duplicateGroups[ groupId ];
		ASSERT( pGroup->GetItems().size() > 1 );

		std::tstring header = str::Format( _T("Group %d"), groupId + 1 );
		if ( pGroup->GetItems().size() > 2 )
			header += str::Format( _T(" (%d duplicates)"), pGroup->GetItems().size() - 1 );

		m_dupsListCtrl.InsertGroupHeader( groupId, groupId, header, LVGS_NORMAL | LVGS_COLLAPSIBLE );
		m_dupsListCtrl.SetGroupTask( groupId, _T("Select Duplicates") );

		for ( std::vector< CDuplicateFileItem* >::const_iterator itDupItem = pGroup->GetItems().begin(); itDupItem != pGroup->GetItems().end(); ++itDupItem, ++index )
		{
			ASSERT( pGroup == ( *itDupItem )->GetParentGroup() );
			m_dupsListCtrl.InsertObjectItem( index, *itDupItem );
			m_dupsListCtrl.SetSubItemText( index, DirPath, ( *itDupItem )->GetKeyPath().GetParentPath().GetPtr() );
			m_dupsListCtrl.SetSubItemText( index, Size, num::FormatFileSize( pGroup->GetContentKey().m_fileSize ) );
			m_dupsListCtrl.SetSubItemText( index, Crc32, num::FormatHexNumber( pGroup->GetContentKey().m_crc32, _T("%X") ) );
			m_dupsListCtrl.SetSubItemText( index, DateModified, time_utl::FormatTimestamp( ( *itDupItem )->GetModifyTime() ) );

			VERIFY( m_dupsListCtrl.SetRowGroupId( index, groupId ) );
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
	subItem;
	enum { LightPastelPink = RGB( 255, 240, 240 ) };
	static const ui::CTextEffect s_duplicate( ui::Regular, CLR_NONE, LightPastelPink );
	static const ui::CTextEffect s_errorBk( ui::Regular, color::Red, app::ColorErrorBk );

	const CDuplicateFileItem* pFileItem = CReportListControl::AsPtr< CDuplicateFileItem >( rowKey );
	const ui::CTextEffect* pTextEffect = NULL;

	if ( pFileItem->IsDuplicateItem() )
		pTextEffect = &s_duplicate;

	if ( utl::Contains( m_errorItems, pFileItem ) )
		rTextEffect |= s_errorBk;							// highlight error row background

	if ( pTextEffect != NULL )
		rTextEffect |= *pTextEffect;

	if ( EditMode == m_mode )								// duplicates list is dirty?
		rTextEffect.m_textColor = ui::GetBlendedColor( rTextEffect.m_textColor != CLR_NONE ? rTextEffect.m_textColor : m_dupsListCtrl.GetTextColor(), color::White );		// blend to gray
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
	DDX_Control( pDX, IDC_FILE_TYPE_COMBO, m_fileTypeCombo );
	DDX_Control( pDX, IDC_FILE_SPEC_EDIT, m_fileSpecEdit );
	DDX_Control( pDX, IDC_MIN_FILE_SIZE_COMBO, m_minFileSizeCombo );
	m_srcPathsToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignRight | V_AlignBottom );
	ui::DDX_ButtonIcon( pDX, IDC_DELETE_DUPLICATES_BUTTON, ID_REMOVE_ALL_ITEMS );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			ui::WriteComboItems( m_fileTypeCombo, GetTags_FileType().GetUiTags() );
			m_fileTypeCombo.SetCurSel( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_fileType, All ) );
			m_fileSpecEdit.SetText( m_fileTypeSpecs[ m_fileTypeCombo.GetCurSel() ] );
			m_minFileSizeCombo.LoadHistory( m_regSection.c_str(), reg::entry_minFileSize, _T("0|1|10|50|100|500|1000") );

			OnUpdate( m_pFileModel, NULL );
			SwitchMode( m_mode );
		}
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CFindDuplicatesDialog, CFileEditorBaseDialog )
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI_RANGE( IDC_UNDO_BUTTON, IDC_REDO_BUTTON, OnUpdateUndoRedo )
	ON_COMMAND( ID_EDIT_LIST_ITEMS, OnEditSrcPaths )
	ON_UPDATE_COMMAND_UI( ID_EDIT_LIST_ITEMS, OnUpdateEditSrcPaths )
	ON_CBN_SELCHANGE( IDC_FILE_TYPE_COMBO, OnCbnSelChange_FileType )
	ON_EN_CHANGE( IDC_FILE_SPEC_EDIT, OnEnChange_FileSpec )
	ON_CBN_EDITCHANGE( IDC_MIN_FILE_SIZE_COMBO, OnCbnChanged_MinFileSize )
	ON_CBN_SELCHANGE( IDC_MIN_FILE_SIZE_COMBO, OnCbnChanged_MinFileSize )
	ON_BN_CLICKED( IDC_SELECT_DUPLICATES_BUTTON, OnBnClicked_CheckSelectDuplicates )
	ON_BN_CLICKED( IDC_DELETE_DUPLICATES_BUTTON, OnBnClicked_DeleteDuplicates )
	ON_BN_CLICKED( IDC_MOVE_DUPLICATES_BUTTON, OnBnClicked_MoveDuplicates )
	ON_BN_CLICKED( IDC_CLEAR_CRC32_CACHE_BUTTON, OnBnClicked_ClearCrc32Cache )
	//ON_UPDATE_COMMAND_UI_RANGE( ID_COPY_MODIFIED_DATE, ID_COPY_ACCESSED_DATE, OnUpdateSelListItem )
	ON_NOTIFY( CReportListControl::LVN_DropFiles, IDC_SOURCE_PATHS_LIST, OnLvnDropFiles_SrcList )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_DUPLICATE_FILES_LIST, OnLvnItemChanged_TouchList )
END_MESSAGE_MAP()

void CFindDuplicatesDialog::OnOK( void )
{
	switch ( m_mode )
	{
		case EditMode:
			if ( SearchForDuplicateFiles() )
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

void CFindDuplicatesDialog::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_fileType, m_fileTypeCombo.GetCurSel() );
	AfxGetApp()->WriteProfileString( reg::section_dialog, reg::entry_fileTypeSpecs, str::Join( m_fileTypeSpecs, _T("|") ).c_str() );
	m_minFileSizeCombo.SaveHistory( m_regSection.c_str(), reg::entry_minFileSize );

	__super::OnDestroy();
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

void CFindDuplicatesDialog::OnEditSrcPaths( void )
{
	CItemListDialog dlg( this, ui::CItemContent( ui::DirPath ) );

	dlg.m_items.reserve( m_srcPathItems.size() );
	for ( std::vector< CSrcPathItem* >::const_iterator itSrcPathItem = m_srcPathItems.begin(); itSrcPathItem != m_srcPathItems.end(); ++itSrcPathItem )
		dlg.m_items.push_back( ( *itSrcPathItem )->GetKeyPath().Get() );

	if ( IDOK == dlg.DoModal() )
	{
		utl::ClearOwningContainer( m_srcPathItems );
		for ( std::vector< std::tstring >::const_iterator itSrcPath = dlg.m_items.begin(); itSrcPath != dlg.m_items.end(); ++itSrcPath )
			m_srcPathItems.push_back( new CSrcPathItem( *itSrcPath ) );

		SetupSrcPathsList();
		SwitchMode( EditMode );
		GotoDlgCtrl( &m_srcPathsListCtrl );
	}
}

void CFindDuplicatesDialog::OnUpdateEditSrcPaths( CCmdUI* pCmdUI )
{
	pCmdUI;
}

void CFindDuplicatesDialog::OnCbnSelChange_FileType( void )
{
	m_fileSpecEdit.SetText( m_fileTypeSpecs[ m_fileTypeCombo.GetCurSel() ] );
	OnFieldChanged();
}

void CFindDuplicatesDialog::OnEnChange_FileSpec( void )
{
	m_fileTypeSpecs[ m_fileTypeCombo.GetCurSel() ] = m_fileSpecEdit.GetText();
	OnFieldChanged();
}

void CFindDuplicatesDialog::OnCbnChanged_MinFileSize( void )
{
	OnFieldChanged();
}

void CFindDuplicatesDialog::OnBnClicked_CheckSelectDuplicates( void )
{
	if ( !m_pFileModel->CopyClipSourceFileStates( this ) )
		AfxMessageBox( _T("Cannot copy source file states to clipboard!"), MB_ICONERROR | MB_OK );
}

void CFindDuplicatesDialog::OnBnClicked_DeleteDuplicates( void )
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

void CFindDuplicatesDialog::OnBnClicked_MoveDuplicates( void )
{
	ClearFileErrors();
	SafeExecuteCmd( new CResetDestinationsCmd( m_pFileModel ) );
}

void CFindDuplicatesDialog::OnBnClicked_ClearCrc32Cache( void )
{
	CFileContentKey::GetCrc32FileCache().Clear();
}

void CFindDuplicatesDialog::OnUpdateSelListItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_dupsListCtrl.GetCurSel() != -1 );
}

void CFindDuplicatesDialog::OnLvnDropFiles_SrcList( NMHDR* pNmHdr, LRESULT* pResult )
{
	CNmDropFiles* pNmDropFiles = (CNmDropFiles*)pNmHdr;
	*pResult = 0;

	std::vector< CSrcPathItem* > newPathItems; newPathItems.reserve( pNmDropFiles->m_filePaths.size() );

	for ( std::vector< std::tstring >::const_iterator itFilePath = pNmDropFiles->m_filePaths.begin(); itFilePath != pNmDropFiles->m_filePaths.end(); ++itFilePath )
		if ( NULL == func::FindItemWithKeyPath( m_srcPathItems, *itFilePath ) )			// unique path?
			newPathItems.push_back( new CSrcPathItem( *itFilePath ) );

	m_srcPathItems.insert( m_srcPathItems.end(), newPathItems.begin(), newPathItems.end() );

	SetupSrcPathsList();
	m_srcPathsListCtrl.SelectItems( newPathItems );

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
