
#include "pch.h"
#include "RenamePages.h"
#include "RenameItem.h"
#include "RenameFilesDialog.h"
#include "GeneralOptions.h"
#include "FileModel.h"
#include "EditingCommands.h"
#include "PathItemSorting.h"
#include "resource.h"
#include "utl/Algorithms.h"
#include "utl/TextClipboard.h"
#include "utl/TimeUtils.h"
#include "utl/LongestCommonSubsequence.h"
#include "utl/UI/WndUtilsEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/ReportListControl.hxx"
#include "utl/UI/SelectionData.hxx"
#include "EditingCommands.hxx"


// CBaseRenameListPage implementation

namespace layout
{
	static const CLayoutStyle stylesList[] =
	{
		{ IDC_FILE_RENAME_LIST, Size }
	};
}

CBaseRenameListPage::CBaseRenameListPage( CRenameFilesDialog* pParentDlg, UINT listLayoutId )
	: CBaseRenamePage( IDD_REN_LIST_PAGE, pParentDlg )
	, m_fileListCtrl( listLayoutId )
{
	RegisterCtrlLayout( ARRAY_SPAN( layout::stylesList ) );

	m_fileListCtrl.SetUseAlternateRowColoring();
	m_fileListCtrl.SetTextEffectCallback( this );
	m_fileListCtrl.SetPersistSorting( false );			// disable sorting persistence since CFileModel persists the shared sorting
	m_fileListCtrl.SetFormatTableFlags( lv::SelRowsDisplayVisibleColumns );		// copy table as selected rows, using visible columns in display order

	// display filenames depending on m_ignoreExtension
	m_fileListCtrl.SetSubjectAdapter( m_pParentDlg->GetDisplayFilenameAdapter() );
	m_fileListCtrl.AddRecordCompare( ren::CRenameItemCriteria::Instance()->GetComparator( ren::RecordDefault ) );		// default row item comparator

	CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );
	// Note: focus retangle is not painted properly without double-buffering
}

CBaseRenameListPage::~CBaseRenameListPage()
{
	m_fileListCtrl.ReleaseSharedComparators();		// avoid deleting comparators owned by ren::CRenameItemCriteria singleton
}

void CBaseRenameListPage::SetupFileListView( void )
{
	CScopedInternalChange listChange( &m_fileListCtrl );
	lv::TScopedStatus_ByObject listStatus( &m_fileListCtrl );
	CScopedLockRedraw freeze( &m_fileListCtrl );

	m_fileListCtrl.DeleteAllItems();
	DoSetupFileListView();
	// note: items are already sorted in the shared sort order persisted in CFileModel
}

void CBaseRenameListPage::UpdateFileListViewItems( const std::vector<CRenameItem*>& selItems )
{
	CScopedInternalChange listChange( &m_fileListCtrl );
	CScopedLockRedraw freeze( &m_fileListCtrl );

	DoUpdateFileListViewItems( selItems );
}

void CBaseRenameListPage::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override
{
	const cmd::CommandType cmdType = static_cast<cmd::CommandType>( utl::GetSafeTypeID( pMessage ) ); cmdType;
	ASSERT_PTR( m_hWnd );

	if ( IsInternalChange() )
		return;			// skip updates from this

	if ( m_pParentDlg->GetFileModel() == pSubject )
	{
		if ( const CChangeDestPathsCmd* pDestPathsCmd = utl::GetSafeMatchCmd<CChangeDestPathsCmd>( pMessage, cmd::ChangeDestPaths ) )
			if ( pDestPathsCmd->HasSelItems() )
			{
				UpdateFileListViewItems( pDestPathsCmd->MakeSelItems() );
				return;
			}

		SetupFileListView();
	}
	else if ( &CGeneralOptions::Instance() == pSubject )
		CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );
	else if ( GetParentSheet() == pSubject )
	{
		if ( const COnRenameListSelChangedCmd* pSelChangedCmd = utl::GetSafeMatchCmd<COnRenameListSelChangedCmd>( pMessage, cmd::OnRenameListSelChanged ) )
			if ( pSelChangedCmd->GetPage() != this )			// selection changed in another page?
			{
				CScopedInternalChange listChange( &m_fileListCtrl );

				pSelChangedCmd->dbgTraceSelData( this );
				pSelChangedCmd->GetSelData().UpdateList( &m_fileListCtrl );

				if ( nullptr == pSelChangedCmd->GetPage() )		// selection changed from rename dialog parent?
					InvalidateFiles();							// trigger some highlighting
			}
	}
}

void CBaseRenameListPage::InvalidateFiles( void )
{
	m_fileListCtrl.Invalidate();
}

bool CBaseRenameListPage::OnParentCommand( UINT cmdId ) const implement
{
	switch ( cmdId )
	{
		case ID_COPY_SEL_ITEMS:
			return m_fileListCtrl.CopyTable( lv::SelRowsDisplayVisibleColumns | ( ui::IsKeyPressed( VK_SHIFT ) ? lv::HeaderRow : 0 ) );
	}

	return false;
}

void CBaseRenameListPage::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const override
{
	subItem, pCtrl;

	static const ui::CTextEffect s_errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );
	const CRenameItem* pRenameItem = CReportListControl::AsPtr<CRenameItem>( rowKey );

	if ( m_pParentDlg->IsErrorItem( pRenameItem ) )
		rTextEffect.Combine( s_errorBk );
}

void CBaseRenameListPage::DoDataExchange( CDataExchange* pDX ) override
{
	bool firstInit = nullptr == m_fileListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_FILE_RENAME_LIST, m_fileListCtrl );

	if ( firstInit )
		if ( m_pParentDlg->IsInitialized() )						// not first time sheet creation? - prevent double init if this was the last active page
		{
			OnUpdate( m_pParentDlg->GetFileModel(), nullptr );		// initialize the page

			if ( nullptr == m_pParentDlg->GetFirstErrorItem<CRenameItem>() )
			{
				CScopedInternalChange listChange( &m_fileListCtrl );

				m_pParentDlg->GetSelData().UpdateList( &m_fileListCtrl );		// initial shared selection
			}
		}

	__super::DoDataExchange( pDX );
}

// message handlers

BEGIN_MESSAGE_MAP( CBaseRenameListPage, CBaseRenamePage )
	ON_NOTIFY( lv::LVN_ListSorted, IDC_FILE_RENAME_LIST, OnLvnListSorted_RenameList )
	ON_NOTIFY( lv::LVN_CopyTableText, IDC_FILE_TOUCH_LIST, OnLvnCopyTableText_RenameList )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_FILE_RENAME_LIST, OnLvnItemChanged_RenameList )
END_MESSAGE_MAP()

void CBaseRenameListPage::OnLvnListSorted_RenameList( NMHDR* pNmHdr, LRESULT* pResult )
{
	pNmHdr;
	*pResult = 0L;

	if ( !m_fileListCtrl.IsInternalChange() )		// sorted by user?
		CSortRenameListCmd( m_pParentDlg->GetFileModel(), &m_fileListCtrl, GetListSorting() ).Execute();		// fetch sorted items sequence into the file model, and notify observers
}

void CBaseRenameListPage::OnLvnCopyTableText_RenameList( NMHDR* pNmHdr, LRESULT* pResult )
{
	lv::CNmCopyTableText* pNmInfo = (lv::CNmCopyTableText*)pNmHdr;
	*pResult = 0L;		// continue default handling

	if ( ui::IsKeyPressed( VK_SHIFT ) )
		pNmInfo->m_textRows.push_back( pNmInfo->m_pColumnSet->FormatHeaderRow() );		// copy header row if SHIFT is pressed
}

void CBaseRenameListPage::OnLvnItemChanged_RenameList( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;

	if ( !IsInternalChange() && !m_fileListCtrl.IsInternalChange() )		// sorted by user?
		if ( m_fileListCtrl.IsSelectionCaretChangeNotify( pNmList ) )
		{
			CReportListControl::TraceNotify( pNmList );

			ui::CSelectionData<CRenameItem>& rSelData = m_pParentDlg->RefSelData();

			if ( rSelData.ReadList( &m_fileListCtrl ) )
				COnRenameListSelChangedCmd( this, rSelData ).Execute();		// synchronize selection across pages
		}

	*pResult = 0;
}


// CRenameSimpleListPage implementation

CRenameSimpleListPage::CRenameSimpleListPage( CRenameFilesDialog* pParentDlg )
	: CBaseRenameListPage( pParentDlg, IDS_FILE_RENAME_SIMPLE_LIST_LAYOUT )
{
	m_fileListCtrl.SetSection( CFileModel::section_filesSheet + _T("\\List") );

	const ren::CRenameItemCriteria* pCriteria = ren::CRenameItemCriteria::Instance();

	m_fileListCtrl.AddColumnCompare( SrcPath, pCriteria->GetComparator( ren::SrcPath ) );
	m_fileListCtrl.AddColumnCompare( DestPath, pCriteria->GetComparator( ren::DestPath ) );

	std::pair<int, bool> listSorting = ToListSorting( m_pParentDlg->GetFileModel()->GetRenameSorting() );
	m_fileListCtrl.StoreSortByColumn( listSorting.first, listSorting.second );
}

void CRenameSimpleListPage::DoSetupFileListView( void ) override
{
	CDisplayFilenameAdapter* pDisplayAdapter = m_pParentDlg->GetDisplayFilenameAdapter();

	for ( unsigned int index = 0; index != m_pParentDlg->GetRenameItems().size(); ++index )
	{
		CRenameItem* pRenameItem = m_pParentDlg->GetRenameItems()[index];

		m_fileListCtrl.InsertObjectItem( index, pRenameItem );		// SrcPath
		m_fileListCtrl.SetSubItemText( index, DestPath, pDisplayAdapter->FormatFilename( pRenameItem->GetDestPath(), pRenameItem->IsDirectory() ) );
	}

	m_fileListCtrl.SetupDiffColumnPair( SrcPath, DestPath, path::TGetMatch() );
}

void CRenameSimpleListPage::DoUpdateFileListViewItems( const std::vector<CRenameItem*>& selItems )
{
	CDisplayFilenameAdapter* pDisplayAdapter = m_pParentDlg->GetDisplayFilenameAdapter();
	path::TGetMatch getMatchFunc;

	for ( CRenameItem* pRenameItem: selItems )
	{
		int index = m_fileListCtrl.FindItemIndex( pRenameItem );
		ASSERT( index != -1 );

		m_fileListCtrl.SetSubItemText( index, DestPath, pDisplayAdapter->FormatFilename( pRenameItem->GetDestPath(), pRenameItem->IsDirectory() ) );
		m_fileListCtrl.UpdateItemDiffColumnDest( index, DestPath, getMatchFunc );		// IMP: update the Dest side of DiffColumn pair!
	}
}

ren::TSortingPair CRenameSimpleListPage::GetListSorting( void ) const override
{
	return FromListSorting( m_fileListCtrl.GetSortByColumn() );
}

void CRenameSimpleListPage::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override
{
	if ( !IsInternalChange() )
		if ( CSortRenameListCmd* pSortCmd = dynamic_cast<CSortRenameListCmd*>( pMessage ) )
			if ( pSortCmd->GetPage() != this )		// external list has been sorted?
			{
				std::pair<int, bool> listSorting = ToListSorting( pSortCmd->m_sorting );

				m_fileListCtrl.StoreSortByColumn( listSorting.first, listSorting.second );
				// base method will update list items
			}

	__super::OnUpdate( pSubject, pMessage );
}

std::pair<int, bool> CRenameSimpleListPage::ToListSorting( const ren::TSortingPair& sorting )
{
	std::pair<int, bool> listSorting = sorting;

	switch ( sorting.first )
	{
		case ren::SrcPath:	listSorting.first = SrcPath; break;
		case ren::DestPath:	listSorting.first = DestPath; break;
		default:
			listSorting = std::make_pair( -1, true );	// incompatible order (sort by size or date), reset sorting in column header
	}
	return listSorting;
}

ren::TSortingPair CRenameSimpleListPage::FromListSorting( const std::pair<int, bool>& listSorting )
{
	ren::TSortingPair sorting( ren::RecordDefault, listSorting.second );

	switch ( listSorting.first )
	{
		default:
			ASSERT( false );
		case -1:		return ren::TSortingPair( ren::RecordDefault, true );
		case SrcPath:	return ren::TSortingPair( ren::SrcPath, listSorting.second );
		case DestPath:	return ren::TSortingPair( ren::DestPath, listSorting.second );
	}
}


// CRenameDetailsListPage implementation

CRenameDetailsListPage::CRenameDetailsListPage( CRenameFilesDialog* pParentDlg )
	: CBaseRenameListPage( pParentDlg, IDS_FILE_RENAME_DETAILS_LIST_LAYOUT )
{
	SetTitle( _T("List Details") );

	m_fileListCtrl.SetSection( CFileModel::section_filesSheet + _T("\\ListDetails") );

	const ren::CRenameItemCriteria* pCriteria = ren::CRenameItemCriteria::Instance();

	m_fileListCtrl.AddColumnCompare( ren::SrcPath, pCriteria->GetComparator( ren::SrcPath ) );
	m_fileListCtrl.AddColumnCompare( ren::SrcSize, pCriteria->GetComparator( ren::SrcSize ), false );					// order by size descending by default
	m_fileListCtrl.AddColumnCompare( ren::SrcDateModify, pCriteria->GetComparator( ren::SrcDateModify ), false );		// order by date descending by default
	m_fileListCtrl.AddColumnCompare( ren::DestPath, pCriteria->GetComparator( ren::DestPath ) );

	std::pair<int, bool> listSorting = m_pParentDlg->GetFileModel()->GetRenameSorting();		// straight ren::SortBy to column correspondence
	m_fileListCtrl.StoreSortByColumn( listSorting.first, listSorting.second );
}

void CRenameDetailsListPage::DoSetupFileListView( void ) override
{
	CDisplayFilenameAdapter* pDisplayAdapter = m_pParentDlg->GetDisplayFilenameAdapter();

	for ( unsigned int pos = 0; pos != m_pParentDlg->GetRenameItems().size(); ++pos )
	{
		CRenameItem* pRenameItem = m_pParentDlg->GetRenameItems()[pos];

		m_fileListCtrl.InsertObjectItem( pos, pRenameItem );		// ren::SrcPath
		m_fileListCtrl.SetSubItemText( pos, ren::SrcSize, num::FormatFileSize( pRenameItem->GetState().m_fileSize ) );
		m_fileListCtrl.SetSubItemText( pos, ren::SrcDateModify, time_utl::FormatTimestamp( pRenameItem->GetState().m_modifTime ) );
		m_fileListCtrl.SetSubItemText( pos, ren::DestPath, pDisplayAdapter->FormatFilename( pRenameItem->GetDestPath(), pRenameItem->IsDirectory() ) );
	}

	m_fileListCtrl.SetupDiffColumnPair( ren::SrcPath, ren::DestPath, path::TGetMatch() );
}

void CRenameDetailsListPage::DoUpdateFileListViewItems( const std::vector<CRenameItem*>& selItems )
{
	CDisplayFilenameAdapter* pDisplayAdapter = m_pParentDlg->GetDisplayFilenameAdapter();
	path::TGetMatch getMatchFunc;

	for ( CRenameItem* pRenameItem: selItems )
	{
		int index = m_fileListCtrl.FindItemIndex( pRenameItem );
		ASSERT( index != -1 );

		m_fileListCtrl.SetSubItemText( index, ren::DestPath, pDisplayAdapter->FormatFilename( pRenameItem->GetDestPath(), pRenameItem->IsDirectory() ) );
		m_fileListCtrl.UpdateItemDiffColumnDest( index, ren::DestPath, getMatchFunc );		// IMP: update the Dest side of DiffColumn pair!
	}
}

ren::TSortingPair CRenameDetailsListPage::GetListSorting( void ) const override
{
	std::pair<int, bool> sorting = m_fileListCtrl.GetSortByColumn();
	return ren::TSortingPair( static_cast<ren::SortBy>( sorting.first ), sorting.second );
}

void CRenameDetailsListPage::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override
{
	if ( !IsInternalChange() )
		if ( const CSortRenameListCmd* pSortCmd = utl::GetSafeMatchCmd<CSortRenameListCmd>( pMessage, cmd::SortRenameList ) )
			if ( pSortCmd->GetPage() != this )		// external list has been sorted?
			{
				std::pair<int, bool> userSorting = pSortCmd->m_sorting;		// one-to-one enum values with Column

				m_fileListCtrl.StoreSortByColumn( userSorting.first, userSorting.second );
				// base method will update list items
			}

	__super::OnUpdate( pSubject, pMessage );
}


// CRenameEditPage implementation

namespace layout
{
	static const CLayoutStyle stylesEdit[] =
	{
		{ IDC_RENAME_SRC_FILES_STATIC, pctSizeX( 50 ) },
		{ IDC_RENAME_DEST_FILES_STATIC, pctMoveX( 50 ) | pctSizeX( 50 ) },
		{ IDC_RENAME_SRC_FILES_EDIT, pctSizeX( 50 ) | SizeY },
		{ IDC_RENAME_DEST_FILES_EDIT, pctMoveX( 50 ) | pctSizeX( 50 ) | SizeY }
	};
}

CRenameEditPage::CRenameEditPage( CRenameFilesDialog* pParentDlg )
	: CBaseRenamePage( IDD_REN_EDIT_PAGE, pParentDlg )
	, m_pDestAdapter( new CDestFilenameAdapter( m_pParentDlg->GetDisplayFilenameAdapter() ) )
	, m_destEdit( true )
	, m_srcStatic( CThemeItem( L"HEADER", HP_HEADERITEM, HIS_NORMAL ) )
	, m_destStatic( CThemeItem( L"HEADER", HP_HEADERITEM, HIS_NORMAL ) )
	, m_syncScrolling( SB_BOTH )
{
	RegisterCtrlLayout( ARRAY_SPAN( layout::stylesEdit ) );
	SetUseLazyUpdateData( true );			// needs data input on page deactivation, e.g. OnSetActive()/OnKillActive()

	m_srcStatic.m_useText = m_destStatic.m_useText = true;

	m_srcEdit.SetKeepSelOnFocus();
	m_srcEdit.SetShowFocus();
	m_srcEdit.SetSubjectAdapter( m_pParentDlg->GetDisplayFilenameAdapter() );
	m_destEdit.SetSubjectAdapter( m_pDestAdapter.get() );
	m_destEdit.SetTextInputCallback( this );
}

CRenameEditPage::~CRenameEditPage()
{
}

void CRenameEditPage::SetupFileEdits( void )
{
	CScopedInternalChange pageChange( this );
	const std::vector<CRenameItem*>& renameItems = m_pParentDlg->GetRenameItems();

	m_srcEdit.StoreItems( renameItems );
	m_destEdit.StoreItems( renameItems );

	SelectItems( m_pParentDlg->GetSelData() );
}

void CRenameEditPage::UpdateFileEdits( const std::vector<CRenameItem*>& selRenameItems )
{
	CScopedInternalChange pageChange( this );

	m_srcEdit.UpdateItems( selRenameItems );
	m_destEdit.UpdateItems( selRenameItems );

	SelectItems( m_pParentDlg->GetSelData() );
}

void CRenameEditPage::CommitLocalEdits( void )
{
	if ( m_hWnd != nullptr && m_destEdit.GetModify() )
	{
		std::vector<fs::CPath> newDestPaths;

		if ( InputDestPaths( newDestPaths ) )
		{
			m_destEdit.SetModify( false );

			if ( AnyChanges( newDestPaths ) )
			{
				CScopedInternalChange pageChange( this );

				m_pParentDlg->SafeExecuteCmd( new CChangeDestPathsCmd( m_pParentDlg->GetFileModel(), nullptr, newDestPaths, _T("Edit destination filenames manually") ) );
			}
		}
	}
}

bool CRenameEditPage::InputDestPaths( OUT std::vector<fs::CPath>& rNewDestPaths )
{
	ui::CTextValidator validator( &m_destEdit );

	if ( ui::ITextInput::Error == OnEditInput( validator ) )
		return false;

	const std::vector<CRenameItem*>& renameItems = m_pParentDlg->GetRenameItems();

	rNewDestPaths.clear();
	rNewDestPaths.reserve( renameItems.size() );

	for ( size_t i = 0; i != renameItems.size(); ++i )
		rNewDestPaths.push_back( m_pDestAdapter->ParseCode( validator.m_lines[i], renameItems[i] ) );		// <dest_dir_path or src_dir_path>/filename

	return true;
}

bool CRenameEditPage::AnyChanges( const std::vector<fs::CPath>& newDestPaths ) const
{
	const std::vector<CRenameItem*>& renameItems = m_pParentDlg->GetRenameItems();
	ASSERT( newDestPaths.size() == renameItems.size() );

	for ( size_t i = 0; i != renameItems.size(); ++i )
		if ( renameItems[i]->GetDestPath().Get() != newDestPaths[i].Get() )			// case sensitive compare
			return true;

	return false;
}

bool CRenameEditPage::SelectItems( const ui::CSelectionData<CRenameItem>& selData )
{
	CScopedInternalChange pageChange( this );

	bool selChanged = selData.UpdateEdit( &m_srcEdit );

	selChanged |= selData.UpdateEdit( &m_destEdit );

	m_syncScrolling.Synchronize( &m_destEdit );			// sync v-scroll pos
	return selChanged;
}

bool CRenameEditPage::SyncSelectItemLine( const CTextListEditor& fromEdit, CTextListEditor* pToEdit )
{
	pToEdit;
	if ( !IsInternalChange() )
	{
		ui::CSelectionData<CRenameItem>& rSelData = m_pParentDlg->RefSelData();

		if ( rSelData.ReadEdit( &fromEdit ) )
		{
			COnRenameListSelChangedCmd( this, rSelData ).Execute();		// synchronize selection for other pages

			rSelData.UpdateEdit( pToEdit );
			return true;
		}
	}

	return false;
}

void CRenameEditPage::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override
{
	const cmd::CommandType cmdType = static_cast<cmd::CommandType>( utl::GetSafeTypeID( pMessage ) ); cmdType;
	ASSERT_PTR( m_hWnd );

	if ( IsInternalChange() )
		return;			// skip updates from this

	if ( m_pParentDlg->GetFileModel() == pSubject )
	{
		if ( const CChangeDestPathsCmd* pDestPathsCmd = utl::GetSafeMatchCmd<CChangeDestPathsCmd>( pMessage, cmd::ChangeDestPaths ) )
			if ( pDestPathsCmd->HasSelItems() )
			{
				UpdateFileEdits( pDestPathsCmd->MakeSelItems() );
				return;
			}

		SetupFileEdits();
	}
	else if ( GetParentSheet() == pSubject )
	{
		if ( const COnRenameListSelChangedCmd* pSelChangedCmd = utl::GetSafeMatchCmd<COnRenameListSelChangedCmd>( pMessage, cmd::OnRenameListSelChanged ) )
			if ( pSelChangedCmd->GetPage() != this )			// external page changed selection?
			{
				pSelChangedCmd->dbgTraceSelData( this );
				SelectItems( pSelChangedCmd->GetSelData() );
			}
	}
}

ui::ITextInput::Result CRenameEditPage::OnEditInput( IN OUT ui::CTextValidator& rValidator ) implement
{
	ASSERT( rValidator.m_pEdit == &m_destEdit );
	ASSERT( rValidator.m_lines.size() == m_pParentDlg->GetRenameItems().size() );		// already validated by the edit

	pred::ValidateFilename validateFunc( CGeneralOptions::Instance().m_trimFname );

	validateFunc = utl::for_each( rValidator.m_lines, validateFunc );

	if ( validateFunc.m_invalidCount != 0 )
		return ui::ITextInput::Error;		// invalid filename
	else if ( validateFunc.m_amendedCount != 0 )
		return ui::ITextInput::Warning;		// amended filename

	return ui::ITextInput::Success;
}

bool CRenameEditPage::OnParentCommand( UINT cmdId ) const implement
{
	switch ( cmdId )
	{
		case ID_COPY_SEL_ITEMS:
		{
			std::vector<std::tstring> selItemLines;
			m_destEdit.QueryItemLines( selItemLines, m_pParentDlg->GetSelData().GetSelItems() );
			return CTextClipboard::CopyToLines( selItemLines, m_hWnd );
		}
	}

	return false;
}

bool CRenameEditPage::RestoreFocusControl( void )
{	// called on page activation by CLayoutPropertyPage::OnSetActive()
	if ( !m_destEdit.SetTopItem( m_pParentDlg->GetSelData().GetTopVisibleItem() ) )
		m_destEdit.EnsureCaretVisible();		// fix: caret of the edits does not scroll into view when page is not active

	return __super::RestoreFocusControl();
}

void CRenameEditPage::DoDataExchange( CDataExchange* pDX ) override
{
	bool firstInit = nullptr == m_srcEdit.m_hWnd;

	DDX_Control( pDX, IDC_RENAME_SRC_FILES_STATIC, m_srcStatic );
	DDX_Control( pDX, IDC_RENAME_DEST_FILES_STATIC, m_destStatic );
	DDX_Control( pDX, IDC_RENAME_SRC_FILES_EDIT, m_srcEdit );
	DDX_Control( pDX, IDC_RENAME_DEST_FILES_EDIT, m_destEdit );

	if ( firstInit )
	{
		m_srcEdit.AddToSyncScrolling( &m_syncScrolling );
		m_destEdit.AddToSyncScrolling( &m_syncScrolling );

		// increase the editing buffer to maximum (potentially very large text with large selection, deep search, etc.):
		m_srcEdit.SetLimitText( 0 );		// INT_MAX characters limit
		m_destEdit.SetLimitText( 0 );		// INT_MAX characters limit

		if ( m_pParentDlg->IsInitialized() )						// not first time sheet creation? - prevent double init if this was the last active page
		{
			// page lazy initialization on activation
			if ( !m_pParentDlg->HasDestPaths() )
				CResetDestinationsMacroCmd( m_pParentDlg->GetFileModel() ).Execute();	// this will call OnUpdate() for all observers
			else
				OnUpdate( m_pParentDlg->GetFileModel(), nullptr );		// initialize the page
		}

		m_destEdit.EnsureCaretVisible();
	}

	if ( DialogSaveChanges == pDX->m_bSaveAndValidate )
		CommitLocalEdits();

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CRenameEditPage, CBaseRenamePage )
	ON_WM_CTLCOLOR()
	ON_EN_USER_SELCHANGE( IDC_RENAME_SRC_FILES_EDIT, OnEnUserSelChange_SrcPaths )
	ON_EN_USER_SELCHANGE( IDC_RENAME_DEST_FILES_EDIT, OnEnUserSelChange_DestPaths )
END_MESSAGE_MAP()

HBRUSH CRenameEditPage::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlType )
{
	HBRUSH hBrushFill = __super::OnCtlColor( pDC, pWnd, ctlType );

	if ( pWnd == &m_srcEdit )
	{
		// Make the read-only edit box look read-only. Strangely by default is displayed with white background, like a writable edit box.
		pDC->SetBkColor( ::GetSysColor( COLOR_BTNFACE ) );
		return ::GetSysColorBrush( COLOR_BTNFACE );
	}

	return hBrushFill;
}

void CRenameEditPage::OnEnUserSelChange_SrcPaths( void )
{
	SyncSelectItemLine( m_srcEdit, &m_destEdit );
}

void CRenameEditPage::OnEnUserSelChange_DestPaths( void )
{
	SyncSelectItemLine( m_destEdit, &m_srcEdit );
}
