
#include "stdafx.h"
#include "FindDuplicatesDialog.h"
#include "DuplicateFileItem.h"
#include "DuplicateFilesFinder.h"
#include "FileModel.h"
#include "FileGroupCommands.h"
#include "GeneralOptions.h"
#include "Application.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/Crc32.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"
#include "utl/FmtUtils.h"
#include "utl/LongestCommonSubsequence.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include "utl/TimeUtils.h"
#include "utl/UI/Clipboard.h"
#include "utl/UI/Color.h"
#include "utl/UI/CheckStatePolicies.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/ItemListDialog.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/ShellDialogs.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


struct CheckDup : public ui::ICheckStatePolicy
{
	enum CheckState { UncheckedItem = BST_UNCHECKED, CheckedItem = BST_CHECKED, OriginalItem, _Count };

	static const ui::ICheckStatePolicy* Instance( void );

	// ui::ICheckStatePolicy interface
	virtual bool IsCheckedState( int checkState ) const { return CheckedItem == checkState; }
	virtual bool IsEnabledState( int checkState ) const { return checkState != OriginalItem; }
	virtual int Toggle( int checkState ) const;
};


const ui::ICheckStatePolicy* CheckDup::Instance( void )
{
	static CheckDup s_checkStatePolicy;
	return &s_checkStatePolicy;
}

int CheckDup::Toggle( int checkState ) const
{
	if ( checkState != OriginalItem )
		++checkState %= OriginalItem;

	return checkState;
}


namespace hlp
{
	struct AsGroupFileSize
	{
		UINT64 operator()( const CDuplicateFilesGroup* pGroup ) const { return pGroup->GetContentKey().m_fileSize; }
	};

	struct AsGroupCrc32
	{
		UINT operator()( const CDuplicateFilesGroup* pGroup ) const { return pGroup->GetContentKey().m_crc32; }
	};

	struct AsGroupDuplicateCount
	{
		size_t operator()( const CDuplicateFilesGroup* pGroup ) const { return pGroup->GetItems().size() - 1; }
	};

	bool HasUniqueGroups( const std::vector< CDuplicateFileItem* >& dupItems )
	{
		std::set< CDuplicateFilesGroup* > groups;
		for ( std::vector< CDuplicateFileItem* >::const_iterator itDupItem = dupItems.begin(); itDupItem != dupItems.end(); ++itDupItem )
			if ( !groups.insert( ( *itDupItem )->GetParentGroup() ).second )
				return false;

		return true;
	}

	CDuplicateFileItem* FindFirstDupItemWithFolderPath( const CDuplicateFilesGroup* pGroup, const fs::CPath& dirPath )
	{
		for ( std::vector< CDuplicateFileItem* >::const_iterator itItem = pGroup->GetItems().begin(); itItem != pGroup->GetItems().end(); ++itItem )
			if ( ( *itItem )->GetFilePath().GetParentPath() == dirPath )
				return *itItem;

		return NULL;
	}
}


namespace reg
{
	static const TCHAR section_dialog[] = _T("FindDuplicatesDialog");
	static const TCHAR entry_minFileSize[] = _T("minFileSize");
	static const TCHAR entry_fileType[] = _T("fileType");
	static const TCHAR entry_fileTypeSpecs[] = _T("fileTypeSpecs");
	static const TCHAR entry_highlightDuplicates[] = _T("HighlightDuplicates");
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
		{ IDC_DUPLICATE_FILES_LIST, SizeX | pctMoveY( TopPct ) | pctSizeY( BottomPct ) },
		{ IDC_STRIP_BAR_2, MoveX | pctMoveY( TopPct ) },
		{ IDC_OUTCOME_INFO_STATUS, SizeX | MoveY },

		{ IDC_GROUP_BOX_2, SizeX | MoveY },
		{ ID_DELETE_DUPLICATES, MoveY },
		{ ID_MOVE_DUPLICATES, MoveY },
		{ IDC_COMMIT_INFO_STATUS, SizeX | MoveY },

		{ IDOK, MoveX },
		{ IDCANCEL, MoveX },
		{ IDC_TOOLBAR_PLACEHOLDER, MoveX }
	};
}


CFindDuplicatesDialog::CFindDuplicatesDialog( CFileModel* pFileModel, CWnd* pParent )
	: CFileEditorBaseDialog( pFileModel, cmd::FindDuplicates, IDD_FIND_DUPLICATES_DIALOG, pParent )
	, m_srcPathsListCtrl( IDC_SOURCE_PATHS_LIST )
	, m_dupsListCtrl( IDC_DUPLICATE_FILES_LIST )
	, m_commitInfoStatic( CRegularStatic::Bold )
	, m_highlightDuplicates( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_highlightDuplicates, true ) != FALSE )
{
	CPathItem::MakePathItems( m_srcPathItems, m_pFileModel->GetSourcePaths() );

	m_nativeCmdTypes.push_back( cmd::ResetDestinations );
	REQUIRE( !m_srcPathItems.empty() );

	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	LoadDlgIcon( ID_FIND_DUPLICATE_FILES );

	m_srcPathsListCtrl.SetAcceptDropFiles();
	m_srcPathsListCtrl.SetSubjectAdapter( ui::CCodeAdapter::Instance() );				// display full paths
	CGeneralOptions::Instance().ApplyToListCtrl( &m_srcPathsListCtrl );

	m_dupsListCtrl.ModifyListStyleEx( 0, LVS_EX_CHECKBOXES );
	m_dupsListCtrl.SetSection( m_regSection + _T("\\List") );
	m_dupsListCtrl.SetTextEffectCallback( this );
	m_dupsListCtrl.SetCheckStatePolicy( CheckDup::Instance() );
	m_dupsListCtrl.SetToggleCheckSelItems();

	m_dupsListCtrl.SetPopupMenu( CReportListControl::Nowhere, &GetDupListPopupMenu( CReportListControl::Nowhere ) );
	m_dupsListCtrl.SetPopupMenu( CReportListControl::OnSelection, &GetDupListPopupMenu( CReportListControl::OnSelection ) );
	m_dupsListCtrl.SetTrackMenuTarget( this );
	CGeneralOptions::Instance().ApplyToListCtrl( &m_dupsListCtrl );

	m_dupsListCtrl.AddColumnCompare( FileName, pred::NewPropertyComparator< CDuplicateFileItem, pred::TCompareNameExt >( CDuplicateFileItem::ToNameExt() ) );
	m_dupsListCtrl.AddColumnCompare( DateModified, pred::NewPropertyComparator< CDuplicateFileItem >( CDuplicateFileItem::AsModifyTime() ), false );		// order date-time descending by default
	m_dupsListCtrl.AddColumnCompare( DuplicateCount, NULL, false );		// order by duplicate count descending by default; NULL comparator since uses only group ordering

	m_srcPathsToolbar.GetStrip().AddButton( ID_EDIT_LIST_ITEMS );
	m_dupsToolbar.GetStrip()
		.AddButton( ID_CHECK_ALL_DUPLICATES )
		.AddButton( ID_UNCHECK_ALL_DUPLICATES )
		.AddSeparator()
		.AddButton( ID_KEEP_AS_ORIGINAL_FILE )
		.AddButton( ID_PICK_AS_ORIGINAL_FOLDER )
		.AddSeparator()
		.AddButton( ID_HIGHLIGHT_DUPLICATES )
		.AddSeparator()
		.AddButton( ID_CLEAR_CRC32_CACHE );

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

CMenu& CFindDuplicatesDialog::GetDupListPopupMenu( CReportListControl::ListPopup popupType )
{
	ASSERT( popupType != CReportListControl::OnGroup );		// use the base popup for OnGroup menu tracking

	static CMenu s_popupMenu[ CReportListControl::_ListPopupCount ];
	CMenu& rMenu = s_popupMenu[ popupType ];
	if ( NULL == rMenu.GetSafeHmenu() )
	{
		CMenu popupMenu;
		ui::LoadPopupSubMenu( rMenu, IDR_CONTEXT_MENU, popup::DuplicatesList, CReportListControl::OnSelection == popupType ? DupListOnSelection : DupListNowhere );
		ui::JoinMenuItems( rMenu, CPathItemListCtrl::GetStdPathListPopupMenu( popupType ) );
	}
	return rMenu;
}

void CFindDuplicatesDialog::ClearDuplicates( void )
{
	utl::ClearOwningContainer( m_duplicateGroups );
}

bool CFindDuplicatesDialog::SearchForDuplicateFiles( void )
{
	UINT64 minFileSize = 0;
	if ( num::ParseNumber( minFileSize, m_minFileSizeCombo.GetCurrentText() ) )
		minFileSize *= 1024;		// to KB

	try
	{
		CDuplicateFilesFinder finder( m_fileSpecEdit.GetText(), minFileSize );
		finder.FindDuplicates( m_duplicateGroups, m_srcPathItems, this );

		m_outcomeStatic.SetWindowText( FormatReport( finder.GetOutcome() ) );
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

bool CFindDuplicatesDialog::QueryCheckedDupFilePaths( std::vector< fs::CPath >& rDupFilePaths ) const
{
	rDupFilePaths.clear();

	std::vector< CDuplicateFileItem* > checkedDupItems;
	if ( m_dupsListCtrl.QueryObjectsWithCheckedState( checkedDupItems, CheckDup::CheckedItem ) )
		func::QueryItemsPaths( rDupFilePaths, checkedDupItems );

	return !rDupFilePaths.empty();
}

bool CFindDuplicatesDialog::DeleteDuplicateFiles( void )
{
	std::vector< fs::CPath > dupFilePaths;
	if ( QueryCheckedDupFilePaths( dupFilePaths ) )
		return ExecuteDuplicatesCmd( new CDeleteFilesCmd( dupFilePaths ) );

	return false;
}

bool CFindDuplicatesDialog::MoveDuplicateFiles( void )
{
	std::vector< fs::CPath > dupFilePaths;
	if ( QueryCheckedDupFilePaths( dupFilePaths ) )
	{
		static std::tstring s_destFolderPath;

		if ( shell::PickFolder( s_destFolderPath, this, 0, _T("Move Selected Duplicates to Folder") ) )
			return ExecuteDuplicatesCmd( new CMoveFilesCmd( dupFilePaths, s_destFolderPath ) );
	}

	return false;
}

bool CFindDuplicatesDialog::ExecuteDuplicatesCmd( utl::ICommand* pDupsCmd )
{
	ClearFileErrors();

	cmd::CScopedErrorObserver observe( this );
	return SafeExecuteCmd( pDupsCmd );
}

void CFindDuplicatesDialog::SwitchMode( Mode mode )
{
	m_mode = mode;
	if ( NULL == m_hWnd )
		return;

	static const UINT s_okIconId[] = { ID_FIND_DUPLICATE_FILES, ID_DELETE_DUPLICATES, 0, 0 };
	static const CEnumTags modeTags( _T("&Search|Delete...|Roll &Back|Roll &Fwd") );
	UpdateOkButton( modeTags.FormatUi( m_mode ), s_okIconId[ m_mode ] );

	static const UINT ctrlIds[] =
	{
		IDC_GROUP_BOX_1, IDC_SOURCE_PATHS_LIST,
		IDC_FILE_TYPE_STATIC, IDC_FILE_TYPE_COMBO, IDC_MINIMUM_SIZE_STATIC, IDC_MIN_FILE_SIZE_COMBO, IDC_FILE_SPEC_STATIC, IDC_FILE_SPEC_EDIT,
		ID_DELETE_DUPLICATES, ID_MOVE_DUPLICATES
	};
	ui::EnableControls( *this, ctrlIds, COUNT_OF( ctrlIds ), !IsRollMode() );
	ui::EnableControl( *this, IDC_DUPLICATE_FILES_STATIC, mode != EditMode );

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

void CFindDuplicatesDialog::PopStackTop( svc::StackType stackType )
{
	ASSERT( !IsRollMode() );

	if ( utl::ICommand* pTopCmd = PeekCmdForDialog( stackType ) )		// comand that is target for this dialog editor?
	{
		bool isThisMacro = cmd::FindDuplicates == pTopCmd->GetTypeID();

		ClearFileErrors();

		if ( isThisMacro )
			m_pFileModel->FetchFromStack( stackType );		// fetch dataset from the stack top macro command
		else
			m_pCmdSvc->UndoRedo( stackType );

		MarkInvalidSrcItems();

		if ( isThisMacro )							// file command?
			SwitchMode( svc::Undo == stackType ? RollBackMode : RollForwardMode );
		else if ( IsNativeCmd( pTopCmd ) )			// file state editing command?
			SwitchMode( CommitFilesMode );
	}
	else
		PopStackRunCrossEditor( stackType );		// end this dialog and execute the target dialog editor (depending on the command on stack top)
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
		ASSERT( pGroup->HasDuplicates() );

		std::tstring header = str::Format( _T("Group %d"), groupId + 1 );
		if ( pGroup->GetItems().size() > 2 )
			header += str::Format( _T(" (%d duplicates)"), pGroup->GetItems().size() - 1 );

		m_dupsListCtrl.InsertGroupHeader( groupId, groupId, header, LVGS_NORMAL | LVGS_COLLAPSIBLE );
		m_dupsListCtrl.SetGroupTask( groupId, _T("Toggle Duplicates") );

		for ( size_t pos = 0; pos != pGroup->GetItems().size(); ++pos, ++index )
		{
			CDuplicateFileItem* pDupItem = pGroup->GetItems()[ pos ];
			ASSERT( pGroup == pDupItem->GetParentGroup() );

			m_dupsListCtrl.InsertObjectItem( index, pDupItem );
			m_dupsListCtrl.SetSubItemText( index, FolderPath, pDupItem->GetFilePath().GetParentPath().GetPtr() );
			m_dupsListCtrl.SetSubItemText( index, Size, num::FormatFileSize( pGroup->GetContentKey().m_fileSize ) );
			m_dupsListCtrl.SetSubItemText( index, Crc32, num::FormatHexNumber( pGroup->GetContentKey().m_crc32, _T("%X") ) );
			m_dupsListCtrl.SetSubItemText( index, DateModified, time_utl::FormatTimestamp( pDupItem->GetModifyTime() ) );

			if ( pDupItem->IsOriginalItem() )
				m_dupsListCtrl.ModifyCheckState( index, CheckDup::OriginalItem );
			else
				m_dupsListCtrl.SetSubItemText( index, DuplicateCount, num::FormatNumber( pos ) );

			VERIFY( m_dupsListCtrl.SetItemGroupId( index, groupId ) );
		}
	}

	m_dupsListCtrl.InitialSortList();		// store original order and sort by current criteria
}

std::tstring CFindDuplicatesDialog::FormatReport( const CDupsOutcome& outcome ) const
{
	std::tstring reportMessage = str::Format( _T("Detected: %d groups, %d duplicate files. Found %d files, %d directories"),
		m_duplicateGroups.size(),
		CDuplicateGroupStore::GetDuplicateItemCount( m_duplicateGroups ),
		outcome.m_foundFileCount,
		outcome.m_searchedDirCount );

	if ( outcome.m_ignoredCount != 0 )
		reportMessage += str::Format( _T(" (%d ignored)"), outcome.m_ignoredCount );

	reportMessage += str::Format( _T(". Elapsed %s."), outcome.m_timer.FormatElapsedDuration( 2 ).c_str() );

	if ( CLogger* pLogger = app::GetLogger() )
		pLogger->LogString( str::Format( _T("Search for duplicates in {%s}  -  %s"), utl::MakeDisplayCodeList( m_srcPathItems, _T(", ") ).c_str(), reportMessage.c_str() ) );

	return reportMessage;
}

void CFindDuplicatesDialog::DisplayCheckedGroupsInfo( void )
{
	std::tstring text;

	std::vector< CDuplicateFileItem* > checkedDupItems;
	if ( m_dupsListCtrl.QueryObjectsWithCheckedState( checkedDupItems, CheckDup::CheckedItem ) )
	{
		std::set< CDuplicateFilesGroup* > dupGroups;
		std::set< fs::CPath > dirPaths;
		UINT64 totalFileSize = 0;

		for ( std::vector< CDuplicateFileItem* >::const_iterator itDupItem = checkedDupItems.begin(); itDupItem != checkedDupItems.end(); ++itDupItem )
		{
			CDuplicateFilesGroup* pParentGroup = ( *itDupItem )->GetParentGroup();

			dupGroups.insert( pParentGroup );
			dirPaths.insert( ( *itDupItem )->GetFilePath().GetParentPath() );
			totalFileSize += pParentGroup->GetContentKey().m_fileSize;
		}

		text = str::Format( _T("%d duplicate files checked to delete from %d groups in %d directories; total size %s"),
			checkedDupItems.size(),
			dupGroups.size(),
			dirPaths.size(),
			num::FormatFileSize( totalFileSize ).c_str() );
	}

	m_commitInfoStatic.SetWindowText( text );

	static const UINT s_ctrlIds[] = { ID_DELETE_DUPLICATES, ID_MOVE_DUPLICATES };
	bool canCommit = !checkedDupItems.empty() && !IsRollMode();
	ui::EnableControls( *this, s_ctrlIds, COUNT_OF( s_ctrlIds ), canCommit );
	ui::EnableControl( *this, IDOK, m_mode != CommitFilesMode || canCommit );
}

void CFindDuplicatesDialog::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage )
{
	pMessage;

	if ( m_hWnd != NULL )
		if ( m_pFileModel == pSubject )
		{
			SetupDialog();

/*			switch ( utl::GetSafeTypeID( pMessage ) )
			{
				case cmd::ChangeDestFileStates:
					PostMakeDest();
					break;
			}*/
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

void CFindDuplicatesDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	subItem;
	if ( pCtrl != &m_dupsListCtrl )
		return;

	enum BkColor { LightPastelGreen = RGB( 225, 250, 237 ), LightPastelPink = RGB( 255, 240, 240 ) };
	enum TextColor { DuplicateRed = RGB( 192, 0, 0 ) };

	static const ui::CTextEffect s_original;
	static const ui::CTextEffect s_duplicate( ui::Regular, DuplicateRed );
	static const ui::CTextEffect s_errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );

	const CDuplicateFileItem* pFileItem = CReportListControl::AsPtr< CDuplicateFileItem >( rowKey );
	const ui::CTextEffect* pTextEffect = pFileItem->IsOriginalItem() ? &s_original : &s_duplicate;

	if ( utl::Contains( m_errorItems, pFileItem ) )
		rTextEffect |= s_errorBk;							// highlight error row background

	if ( m_highlightDuplicates )
		rTextEffect |= *pTextEffect;

//	if ( FileName == subItem && pFileItem->IsOriginalItem() )
//		SetFlag( rTextEffect.m_fontEffect, ui::Bold );

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
			if ( !( *itItem )->GetFilePath().FileExist() )
				utl::AddUnique( m_errorItems, *itItem );
}

void CFindDuplicatesDialog::EnsureVisibleFirstError( void )
{
	if ( const CDuplicateFileItem* pFirstErrorItem = GetFirstErrorItem< CDuplicateFileItem >() )
		m_dupsListCtrl.EnsureVisibleObject( pFirstErrorItem );

	m_dupsListCtrl.Invalidate();				// trigger some highlighting
}

void CFindDuplicatesDialog::ToggleCheckGroupDuplicates( unsigned int groupId )
{
	ASSERT( groupId < m_duplicateGroups.size() );

	const CDuplicateFilesGroup* pCurrGroup = m_duplicateGroups[ groupId ];
	ASSERT( pCurrGroup->GetItems().front()->IsOriginalItem() );
	size_t dupsCheckedCount = 0;

	for ( std::vector< CDuplicateFileItem* >::const_iterator itItem = pCurrGroup->GetItems().begin() + 1; itItem != pCurrGroup->GetItems().end(); ++itItem )
		if ( CheckDup::CheckedItem == m_dupsListCtrl.GetObjectCheckState( *itItem ) )
			++dupsCheckedCount;

	CScopedInternalChange change( &m_dupsListCtrl );		// prevent selection block check-state changes
	CheckDup::CheckState checkState = pCurrGroup->GetDuplicatesCount() == dupsCheckedCount ? CheckDup::UncheckedItem : CheckDup::CheckedItem;		// toggle checked duplicates

	for ( std::vector< CDuplicateFileItem* >::const_iterator itItem = pCurrGroup->GetItems().begin() + 1; itItem != pCurrGroup->GetItems().end(); ++itItem )
		m_dupsListCtrl.ModifyObjectCheckState( *itItem, checkState );
}

template< typename CompareGroupPtr >
pred::CompareResult CFindDuplicatesDialog::CompareGroupsBy( int leftGroupId, int rightGroupId, CompareGroupPtr compareGroup ) const
{
	pred::CompareInOrder< CompareGroupPtr > compare( compareGroup, m_dupsListCtrl.GetSortByColumn().second );

	return compare( m_duplicateGroups[ leftGroupId ], m_duplicateGroups[ rightGroupId ] );
}

template< typename CompareItemPtr >
pred::CompareResult CFindDuplicatesDialog::CompareGroupsByItemField( int leftGroupId, int rightGroupId, CompareItemPtr compareItem ) const
{
	pred::CompareInOrder< CompareItemPtr > compare( compareItem, m_dupsListCtrl.GetSortByColumn().second );

	return compare( m_duplicateGroups[ leftGroupId ]->GetSortingItem( compare ), m_duplicateGroups[ rightGroupId ]->GetSortingItem( compare ) );
}

pred::CompareResult CALLBACK CFindDuplicatesDialog::CompareGroupFileName( int leftGroupId, int rightGroupId, const CFindDuplicatesDialog* pThis )
{
	return pThis->CompareGroupsByItemField( leftGroupId, rightGroupId, pred::CompareAdapterPtr< pred::CompareNaturalPath, CDuplicateFileItem::ToNameExt >() );
}

pred::CompareResult CALLBACK CFindDuplicatesDialog::CompareGroupFolderPath( int leftGroupId, int rightGroupId, const CFindDuplicatesDialog* pThis )
{
	return pThis->CompareGroupsByItemField( leftGroupId, rightGroupId, pred::CompareAdapterPtr< pred::CompareNaturalPath, CDuplicateFileItem::ToParentFolderPath >() );
}

pred::CompareResult CALLBACK CFindDuplicatesDialog::CompareGroupFileSize( int leftGroupId, int rightGroupId, const CFindDuplicatesDialog* pThis )
{
	return pThis->CompareGroupsBy( leftGroupId, rightGroupId, pred::CompareScalarAdapterPtr< hlp::AsGroupFileSize >() );
}

pred::CompareResult CALLBACK CFindDuplicatesDialog::CompareGroupFileCrc32( int leftGroupId, int rightGroupId, const CFindDuplicatesDialog* pThis )
{
	return pThis->CompareGroupsBy( leftGroupId, rightGroupId, pred::CompareScalarAdapterPtr< hlp::AsGroupCrc32 >() );
}

pred::CompareResult CALLBACK CFindDuplicatesDialog::CompareGroupDateModified( int leftGroupId, int rightGroupId, const CFindDuplicatesDialog* pThis )
{
	return pThis->CompareGroupsByItemField( leftGroupId, rightGroupId, pred::CompareScalarAdapterPtr< CDuplicateFileItem::AsModifyTime >() );
}

pred::CompareResult CALLBACK CFindDuplicatesDialog::CompareGroupDuplicateCount( int leftGroupId, int rightGroupId, const CFindDuplicatesDialog* pThis )
{
	return pThis->CompareGroupsBy( leftGroupId, rightGroupId, pred::CompareScalarAdapterPtr< hlp::AsGroupDuplicateCount >() );
}

BOOL CFindDuplicatesDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		m_dupsListCtrl.OnCmdMsg( id, code, pExtra, pHandlerInfo );		// allow handling list std commands (Copy, Select All)
}

void CFindDuplicatesDialog::DoDataExchange( CDataExchange* pDX )
{
	const bool firstInit = NULL == m_srcPathsListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_SOURCE_PATHS_LIST, m_srcPathsListCtrl );
	DDX_Control( pDX, IDC_DUPLICATE_FILES_LIST, m_dupsListCtrl );
	DDX_Control( pDX, IDC_FILE_TYPE_COMBO, m_fileTypeCombo );
	DDX_Control( pDX, IDC_FILE_SPEC_EDIT, m_fileSpecEdit );
	DDX_Control( pDX, IDC_MIN_FILE_SIZE_COMBO, m_minFileSizeCombo );
	DDX_Control( pDX, IDC_OUTCOME_INFO_STATUS, m_outcomeStatic );
	DDX_Control( pDX, IDC_COMMIT_INFO_STATUS, m_commitInfoStatic );
	m_srcPathsToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignRight | V_AlignBottom );
	m_dupsToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_2, H_AlignRight | V_AlignBottom );
	ui::DDX_ButtonIcon( pDX, ID_DELETE_DUPLICATES );
	ui::DDX_ButtonIcon( pDX, ID_MOVE_DUPLICATES );

	if ( firstInit )
	{
		ui::WriteComboItems( m_fileTypeCombo, GetTags_FileType().GetUiTags() );
		m_fileTypeCombo.SetCurSel( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_fileType, All ) );
		m_fileSpecEdit.SetText( m_fileTypeSpecs[ m_fileTypeCombo.GetCurSel() ] );
		m_minFileSizeCombo.LoadHistory( m_regSection.c_str(), reg::entry_minFileSize, _T("0|1|10|50|100|500|1000") );

		m_dupsListCtrl.GetStateImageList()->Add( CImageStore::SharedStore()->RetrieveIcon( ID_ORIGINAL_FILE )->GetHandle() );		// OriginalItem

		OnUpdate( m_pFileModel, NULL );
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CFindDuplicatesDialog, CFileEditorBaseDialog )
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI_RANGE( IDC_UNDO_BUTTON, IDC_REDO_BUTTON, OnUpdateUndoRedo )
	ON_COMMAND( ID_EDIT_LIST_ITEMS, OnEditSrcPaths )
	ON_UPDATE_COMMAND_UI( ID_EDIT_LIST_ITEMS, OnUpdateEditSrcPaths )
	ON_CBN_SELCHANGE( IDC_FILE_TYPE_COMBO, OnCbnSelChange_FileType )
	ON_EN_CHANGE( IDC_FILE_SPEC_EDIT, OnEnChange_FileSpec )
	ON_CBN_EDITCHANGE( IDC_MIN_FILE_SIZE_COMBO, OnCbnChanged_MinFileSize )
	ON_CBN_SELCHANGE( IDC_MIN_FILE_SIZE_COMBO, OnCbnChanged_MinFileSize )

	ON_COMMAND_RANGE( ID_CHECK_ALL_DUPLICATES, ID_UNCHECK_ALL_DUPLICATES, OnCheckAllDuplicates )
	ON_UPDATE_COMMAND_UI_RANGE( ID_CHECK_ALL_DUPLICATES, ID_UNCHECK_ALL_DUPLICATES, OnUpdateCheckAllDuplicates )
	ON_COMMAND( ID_TOGGLE_CHECK_GROUP_DUPLICATES, OnToggleCheckGroupDups )
	ON_UPDATE_COMMAND_UI( ID_TOGGLE_CHECK_GROUP_DUPLICATES, OnUpdateToggleCheckGroupDups )
	ON_COMMAND( ID_KEEP_AS_ORIGINAL_FILE, OnKeepAsOriginalFile )
	ON_UPDATE_COMMAND_UI( ID_KEEP_AS_ORIGINAL_FILE, OnUpdateKeepAsOriginalFile )
	ON_COMMAND( ID_PICK_AS_ORIGINAL_FOLDER, OnPickAsOriginalFolder )
	ON_UPDATE_COMMAND_UI( ID_PICK_AS_ORIGINAL_FOLDER, OnUpdatePickAsOriginalFolder )
	ON_COMMAND( ID_CLEAR_CRC32_CACHE, OnClearCrc32Cache )
	ON_UPDATE_COMMAND_UI( ID_CLEAR_CRC32_CACHE, OnUpdateClearCrc32Cache )
	ON_COMMAND( ID_HIGHLIGHT_DUPLICATES, OnToggleHighlightDuplicates )
	ON_UPDATE_COMMAND_UI( ID_HIGHLIGHT_DUPLICATES, OnUpdateHighlightDuplicates )

	ON_BN_CLICKED( ID_DELETE_DUPLICATES, OnBnClicked_DeleteDuplicates )
	ON_BN_CLICKED( ID_MOVE_DUPLICATES, OnBnClicked_MoveDuplicates )

	ON_NOTIFY( lv::LVN_DropFiles, IDC_SOURCE_PATHS_LIST, OnLvnDropFiles_SrcList )
	ON_NOTIFY( LVN_LINKCLICK, IDC_DUPLICATE_FILES_LIST, OnLvnLinkClick_DuplicateList )
	ON_NOTIFY( lv::LVN_CheckStatesChanged, IDC_DUPLICATE_FILES_LIST, OnLvnCheckStatesChanged_DuplicateList )
	ON_NOTIFY( lv::LVN_CustomSortList, IDC_DUPLICATE_FILES_LIST, OnLvnCustomSortList_DuplicateList )
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
				SwitchMode( EditMode );
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

void CFindDuplicatesDialog::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_fileType, m_fileTypeCombo.GetCurSel() );
	AfxGetApp()->WriteProfileString( reg::section_dialog, reg::entry_fileTypeSpecs, str::Join( m_fileTypeSpecs, _T("|") ).c_str() );
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_highlightDuplicates, m_highlightDuplicates );
	m_minFileSizeCombo.SaveHistory( m_regSection.c_str(), reg::entry_minFileSize );

	__super::OnDestroy();
}

void CFindDuplicatesDialog::OnIdleUpdateControls( void )
{
	__super::OnIdleUpdateControls();
	DisplayCheckedGroupsInfo();
}

void CFindDuplicatesDialog::OnUpdateUndoRedo( CCmdUI* pCmdUI )
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

void CFindDuplicatesDialog::OnFieldChanged( void )
{
	SwitchMode( EditMode );
}

void CFindDuplicatesDialog::OnEditSrcPaths( void )
{
	CItemListDialog dlg( this, ui::CItemContent( ui::DirPath ) );

	dlg.m_items.reserve( m_srcPathItems.size() );
	for ( std::vector< CPathItem* >::const_iterator itSrcPathItem = m_srcPathItems.begin(); itSrcPathItem != m_srcPathItems.end(); ++itSrcPathItem )
		dlg.m_items.push_back( ( *itSrcPathItem )->GetFilePath().Get() );

	if ( IDOK == dlg.DoModal() )
	{
		utl::ClearOwningContainer( m_srcPathItems );
		CPathItem::MakePathItems( m_srcPathItems, dlg.m_items );

		SetupSrcPathsList();
		SwitchMode( EditMode );
		GotoDlgCtrl( &m_srcPathsListCtrl );
	}
}

void CFindDuplicatesDialog::OnUpdateEditSrcPaths( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !IsRollMode() );
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

void CFindDuplicatesDialog::OnCheckAllDuplicates( UINT cmdId )
{
	CheckDup::CheckState toCheckState = ID_CHECK_ALL_DUPLICATES == cmdId ? CheckDup::CheckedItem : CheckDup::UncheckedItem;
	CScopedInternalChange changeDups( &m_dupsListCtrl );

	for ( UINT i = 0, count = m_dupsListCtrl.GetItemCount(); i != count; ++i )
	{
		const CDuplicateFileItem* pItem = m_dupsListCtrl.GetPtrAt< CDuplicateFileItem >( i );

		m_dupsListCtrl.ModifyCheckState( i, pItem->IsOriginalItem() ? CheckDup::OriginalItem : toCheckState );
	}
}

void CFindDuplicatesDialog::OnUpdateCheckAllDuplicates( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( CommitFilesMode == m_mode && !m_duplicateGroups.empty() );
}

void CFindDuplicatesDialog::OnToggleCheckGroupDups( void )
{
	int itemIndex = m_dupsListCtrl.GetCaretIndex();
	ToggleCheckGroupDuplicates( m_dupsListCtrl.GetItemGroupId( itemIndex ) );
}

void CFindDuplicatesDialog::OnUpdateToggleCheckGroupDups( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_dupsListCtrl.GetCaretIndex() != -1 );
}

void CFindDuplicatesDialog::OnKeepAsOriginalFile( void )
{
	std::vector< CDuplicateFileItem* > selItems;
	if ( m_dupsListCtrl.QuerySelectionAs( selItems ) )
	{
		utl::RemoveIf( selItems, std::mem_fun( &CDuplicateFileItem::IsOriginalItem ) );
		std::for_each( selItems.begin(), selItems.end(), std::mem_fun( &CDuplicateFileItem::MakeOriginalItem ) );

		std::vector< CDuplicateFileItem* > checkedItems;
		m_dupsListCtrl.QueryObjectsWithCheckedState( checkedItems, CheckDup::CheckedItem );

		SetupDuplicateFileList();
		m_dupsListCtrl.SelectObjects( selItems );

		CScopedInternalChange change( &m_dupsListCtrl );
		utl::RemoveIf( checkedItems, std::mem_fun( &CDuplicateFileItem::IsOriginalItem ) );
		m_dupsListCtrl.SetObjectsCheckedState( &checkedItems, CheckDup::CheckedItem, false );			// restore original checked state (except new originals)
	}
}

void CFindDuplicatesDialog::OnUpdateKeepAsOriginalFile( CCmdUI* pCmdUI )
{
	bool enable = false;
	if ( m_dupsListCtrl.AnySelected() )
	{
		std::vector< CDuplicateFileItem* > selItems;
		if ( m_dupsListCtrl.QuerySelectionAs( selItems ) )
			if ( utl::Any( selItems, std::mem_fun( &CDuplicateFileItem::IsDuplicateItem ) ) )
				enable = hlp::HasUniqueGroups( selItems );
	}

	pCmdUI->Enable( enable );
}

void CFindDuplicatesDialog::OnPickAsOriginalFolder( void )
{
	int selIndex = m_dupsListCtrl.GetCurSel();
	if ( -1 == selIndex )
		return;

	std::vector< CDuplicateFileItem* > checkedDupItems;
	m_dupsListCtrl.QueryObjectsWithCheckedState( checkedDupItems, CheckDup::CheckedItem );

	fs::CPath dirPath = m_dupsListCtrl.GetPtrAt< CDuplicateFileItem >( selIndex )->GetFilePath().GetParentPath();
	std::vector< CDuplicateFileItem* > originalItems;

	for ( std::vector< CDuplicateFilesGroup* >::const_iterator itGroup = m_duplicateGroups.begin(); itGroup != m_duplicateGroups.end(); ++itGroup )
		if ( CDuplicateFileItem* pItem = hlp::FindFirstDupItemWithFolderPath( *itGroup, dirPath ) )
		{
			pItem->MakeOriginalItem();
			originalItems.push_back( pItem );
		}

	SetupDuplicateFileList();
	m_dupsListCtrl.SelectObjects( originalItems );

	// remove any new original items from the checked items
	utl::RemoveIf( checkedDupItems, std::mem_fun( &CDuplicateFileItem::IsOriginalItem ) );

	CScopedInternalChange change( &m_dupsListCtrl );
	m_dupsListCtrl.SetObjectsCheckedState( &checkedDupItems, CheckDup::CheckedItem, false );			// restore original checked state (except new originals)
}

void CFindDuplicatesDialog::OnUpdatePickAsOriginalFolder( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_dupsListCtrl.SingleSelected() );
}

void CFindDuplicatesDialog::OnBnClicked_DeleteDuplicates( void )
{
	if ( DeleteDuplicateFiles() )
		SwitchMode( EditMode );
}

void CFindDuplicatesDialog::OnBnClicked_MoveDuplicates( void )
{
	if ( MoveDuplicateFiles() )
		SwitchMode( EditMode );
}

void CFindDuplicatesDialog::OnClearCrc32Cache( void )
{
	fs::CCrc32FileCache::Instance().Clear();
}

void CFindDuplicatesDialog::OnUpdateClearCrc32Cache( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !fs::CCrc32FileCache::Instance().IsEmpty() );
}

void CFindDuplicatesDialog::OnToggleHighlightDuplicates( void )
{
	m_highlightDuplicates = !m_highlightDuplicates;
	m_dupsListCtrl.Invalidate();
}

void CFindDuplicatesDialog::OnUpdateHighlightDuplicates( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( m_highlightDuplicates );
}

void CFindDuplicatesDialog::OnUpdateSelListItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_dupsListCtrl.GetCurSel() != -1 );
}

void CFindDuplicatesDialog::OnLvnDropFiles_SrcList( NMHDR* pNmHdr, LRESULT* pResult )
{
	const lv::CNmDropFiles* pNmDropFiles = (lv::CNmDropFiles*)pNmHdr;
	*pResult = 0;

	std::vector< CPathItem* > newPathItems; newPathItems.reserve( pNmDropFiles->m_filePaths.size() );

	for ( std::vector< std::tstring >::const_iterator itFilePath = pNmDropFiles->m_filePaths.begin(); itFilePath != pNmDropFiles->m_filePaths.end(); ++itFilePath )
		if ( NULL == func::FindItemWithPath( m_srcPathItems, *itFilePath ) )			// unique path?
			newPathItems.push_back( new CPathItem( *itFilePath ) );

	m_srcPathItems.insert( m_srcPathItems.begin() + pNmDropFiles->m_dropItemIndex, newPathItems.begin(), newPathItems.end() );

	SetupSrcPathsList();
	m_srcPathsListCtrl.SelectObjects( newPathItems );

	SwitchMode( EditMode );
}

void CFindDuplicatesDialog::OnLvnLinkClick_DuplicateList( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLVLINK* pLinkInfo = (NMLVLINK*)pNmHdr;

	ToggleCheckGroupDuplicates( pLinkInfo->iSubItem );
	*pResult = 0;
}

void CFindDuplicatesDialog::OnLvnCheckStatesChanged_DuplicateList( NMHDR* pNmHdr, LRESULT* pResult )
{
	lv::CNmCheckStatesChanged* pInfo = (lv::CNmCheckStatesChanged*)pNmHdr;
	pInfo;
	*pResult = 0L;
}

void CFindDuplicatesDialog::OnLvnCustomSortList_DuplicateList( NMHDR* pNmHdr, LRESULT* pResult )
{
	pNmHdr;
	*pResult = FALSE;
	std::pair< int, bool > currSort = m_dupsListCtrl.GetSortByColumn();	// <sortByColumn, sortAscending>

	switch ( currSort.first )
	{
		case FileName:				// sort groups AND items
			m_dupsListCtrl.SortGroups( (PFNLVGROUPCOMPARE)&CompareGroupFileName, this );
			break;					// sorted the groups, but keep on sorting the items
		case FolderPath:			// sort groups AND items
			m_dupsListCtrl.SortGroups( (PFNLVGROUPCOMPARE)&CompareGroupFolderPath, this );
			break;					// sorted the groups, but keep on sorting the items
		case Size:
			m_dupsListCtrl.SortGroups( (PFNLVGROUPCOMPARE)&CompareGroupFileSize, this );
			*pResult = TRUE;		// done, prevent list item internal sorting
			break;
		case Crc32:
			m_dupsListCtrl.SortGroups( (PFNLVGROUPCOMPARE)&CompareGroupFileCrc32, this );
			*pResult = TRUE;		// done, prevent list item internal sorting
			break;
		case DateModified:			// sort groups AND items
			m_dupsListCtrl.SortGroups( (PFNLVGROUPCOMPARE)&CompareGroupDateModified, this );
			break;					// sorted the groups, but keep on sorting the items
		case DuplicateCount:
			m_dupsListCtrl.SortGroups( (PFNLVGROUPCOMPARE)&CompareGroupDuplicateCount, this );
			*pResult = TRUE;		// done, prevent list item internal sorting
			break;
	}
}