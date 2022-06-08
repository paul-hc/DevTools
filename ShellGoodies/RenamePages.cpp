
#include "stdafx.h"
#include "RenamePages.h"
#include "RenameItem.h"
#include "RenameFilesDialog.h"
#include "GeneralOptions.h"
#include "FileModel.h"
#include "EditingCommands.h"
#include "PathItemSorting.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/TimeUtils.h"
#include "utl/LongestCommonSubsequence.h"
#include "utl/UI/WndUtilsEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/ReportListControl.hxx"


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
	RegisterCtrlLayout( ARRAY_PAIR( layout::stylesList ) );

	m_fileListCtrl.SetUseAlternateRowColoring();
	m_fileListCtrl.SetTextEffectCallback( this );

	// display filenames depending on m_ignoreExtension
	m_fileListCtrl.SetSubjectAdapter( m_pParentDlg->GetDisplayFilenameAdapter() );
	m_fileListCtrl.AddRecordCompare( ren::CRenameItemCriteria::Instance()->GetComparator( ren::SrcPathDirsFirst ) );		// default row item comparator

	CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );
	// Note: focus retangle is not painted properly without double-buffering
}

CBaseRenameListPage::~CBaseRenameListPage()
{
	m_fileListCtrl.ReleaseSharedComparators();		// avoid deleting comparators owned by ren::CRenameItemCriteria singleton
}

void CBaseRenameListPage::SetupFileListView( void )
{
	CScopedListTextSelection sel( &m_fileListCtrl );

	CScopedLockRedraw freeze( &m_fileListCtrl );
	CScopedInternalChange internalChange( &m_fileListCtrl );

	m_fileListCtrl.DeleteAllItems();
	DoSetupFileListView();

	// note: items are already sorted in the shared sort order persisted in CFileModel
}

void CBaseRenameListPage::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override
{
	pMessage;
	ASSERT_PTR( m_hWnd );

	if ( m_pParentDlg->GetFileModel() == pSubject )
		SetupFileListView();
	else if ( &CGeneralOptions::Instance() == pSubject )
		CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );
}

void CBaseRenameListPage::EnsureVisibleItem( const CRenameItem* pRenameItem ) override
{
	if ( pRenameItem != NULL )
		m_fileListCtrl.EnsureVisibleObject( pRenameItem );

	InvalidateFiles();				// trigger some highlighting
}

void CBaseRenameListPage::InvalidateFiles( void )
{
	m_fileListCtrl.Invalidate();
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
	bool firstInit = NULL == m_fileListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_FILE_RENAME_LIST, m_fileListCtrl );

	if ( firstInit )
		OnUpdate( m_pParentDlg->GetFileModel(), NULL );			// initialize the page

	CBaseRenamePage::DoDataExchange( pDX );
}

// message handlers

BEGIN_MESSAGE_MAP( CBaseRenameListPage, CBaseRenamePage )
	ON_NOTIFY( lv::LVN_ListSorted, IDC_FILE_RENAME_LIST, OnLvnListSorted_RenameList )
END_MESSAGE_MAP()

void CBaseRenameListPage::OnLvnListSorted_RenameList( NMHDR* pNmHdr, LRESULT* pResult )
{
	pNmHdr;
	*pResult = 0L;

	if ( !m_fileListCtrl.IsInternalChange() )		// sorted by user?
		COnRenameListSortedCmd( m_pParentDlg->GetFileModel(), &m_fileListCtrl, GetListSorting() ).Execute();		// fetch sorted items sequence into the file model, and notify observers
}


// CRenameSimpleListPage implementation

CRenameSimpleListPage::CRenameSimpleListPage( CRenameFilesDialog* pParentDlg )
	: CBaseRenameListPage( pParentDlg, IDS_FILE_RENAME_SIMPLE_LIST_LAYOUT )
{
	m_fileListCtrl.SetSection( CFileModel::section_filesSheet + _T("\\ListPage\\SimpleList") );

	const ren::CRenameItemCriteria* pCriteria = ren::CRenameItemCriteria::Instance();

	m_fileListCtrl.AddColumnCompare( SrcPath, pCriteria->GetComparator( ren::SrcPath ) );
	m_fileListCtrl.AddColumnCompare( DestPath, pCriteria->GetComparator( ren::DestPath ) );
}

void CRenameSimpleListPage::DoSetupFileListView( void ) override
{
	CDisplayFilenameAdapter* pDisplayAdapter = m_pParentDlg->GetDisplayFilenameAdapter();

	for ( unsigned int pos = 0; pos != m_pParentDlg->GetRenameItems().size(); ++pos )
	{
		CRenameItem* pRenameItem = m_pParentDlg->GetRenameItems()[ pos ];

		m_fileListCtrl.InsertObjectItem( pos, pRenameItem );		// SrcPath
		m_fileListCtrl.SetSubItemText( pos, DestPath, pDisplayAdapter->FormatFilename( pRenameItem->GetDestPath() ) );
	}

	m_fileListCtrl.SetupDiffColumnPair( SrcPath, DestPath, path::TGetMatch() );
}

ren::TSortingPair CRenameSimpleListPage::GetListSorting( void ) const override
{
	std::pair<int, bool> sorting = m_fileListCtrl.GetSortByColumn();
	ren::SortBy sortBy = ren::SrcPathDirsFirst;

	switch ( sorting.first )
	{
		case SrcPath:
			sortBy = ren::SrcPath;
			break;
		case DestPath:
			sortBy = ren::DestPath;
			break;
	}
	return ren::TSortingPair( sortBy, sorting.second );
}

void CRenameSimpleListPage::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override
{
	if ( COnRenameListSortedCmd* pListSortedCmd = dynamic_cast<COnRenameListSortedCmd*>( pMessage ) )
		if ( pListSortedCmd->GetListCtrl() != &m_fileListCtrl )		// external list has been sorted?
		{
			std::pair<int, bool> userSorting = pListSortedCmd->GetSorting();

			switch ( userSorting.first )
			{
				case ren::SrcPath:	userSorting.first = SrcPath; break;
				case ren::DestPath:	userSorting.first = DestPath; break;
				default:
					userSorting = std::make_pair( -1, true );	// incompatible order (sort by size or date), reset sorting in column header
			}

			m_fileListCtrl.StoreSortByColumn( userSorting.first, userSorting.second );
		}

	__super::OnUpdate( pSubject, pMessage );		// this will update list items
}


// CRenameDetailsListPage implementation

CRenameDetailsListPage::CRenameDetailsListPage( CRenameFilesDialog* pParentDlg )
	: CBaseRenameListPage( pParentDlg, IDS_FILE_RENAME_DETAILS_LIST_LAYOUT )
{
	SetTitle( _T("List Details") );

	m_fileListCtrl.SetSection( CFileModel::section_filesSheet + _T("\\ListPage\\DetailsList") );

	const ren::CRenameItemCriteria* pCriteria = ren::CRenameItemCriteria::Instance();

	m_fileListCtrl.AddColumnCompare( ren::SrcPath, pCriteria->GetComparator( ren::SrcPath ) );
	m_fileListCtrl.AddColumnCompare( ren::SrcSize, pCriteria->GetComparator( ren::SrcSize ), false );					// order size descending by default
	m_fileListCtrl.AddColumnCompare( ren::SrcDateModify, pCriteria->GetComparator( ren::SrcDateModify ), false );		// order date-time descending by default
	m_fileListCtrl.AddColumnCompare( ren::DestPath, pCriteria->GetComparator( ren::DestPath ) );
}

void CRenameDetailsListPage::DoSetupFileListView( void ) override
{
	CDisplayFilenameAdapter* pDisplayAdapter = m_pParentDlg->GetDisplayFilenameAdapter();

	for ( unsigned int pos = 0; pos != m_pParentDlg->GetRenameItems().size(); ++pos )
	{
		CRenameItem* pRenameItem = m_pParentDlg->GetRenameItems()[ pos ];

		m_fileListCtrl.InsertObjectItem( pos, pRenameItem );		// ren::SrcPath
		m_fileListCtrl.SetSubItemText( pos, ren::SrcSize, num::FormatFileSize( pRenameItem->GetState().m_fileSize ) );
		m_fileListCtrl.SetSubItemText( pos, ren::SrcDateModify, time_utl::FormatTimestamp( pRenameItem->GetState().m_modifTime ) );
		m_fileListCtrl.SetSubItemText( pos, ren::DestPath, pDisplayAdapter->FormatFilename( pRenameItem->GetDestPath() ) );
	}

	m_fileListCtrl.SetupDiffColumnPair( ren::SrcPath, ren::DestPath, path::TGetMatch() );
}

ren::TSortingPair CRenameDetailsListPage::GetListSorting( void ) const override
{
	std::pair<int, bool> sorting = m_fileListCtrl.GetSortByColumn();
	return ren::TSortingPair( static_cast<ren::SortBy>( sorting.first ), sorting.second );
}

void CRenameDetailsListPage::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override
{
	if ( COnRenameListSortedCmd* pListSortedCmd = dynamic_cast<COnRenameListSortedCmd*>( pMessage ) )
		if ( pListSortedCmd->GetListCtrl() != &m_fileListCtrl )		// external list has been sorted?
		{
			std::pair<int, bool> userSorting = pListSortedCmd->GetSorting();		// one-to-one enum values with Column

			m_fileListCtrl.StoreSortByColumn( userSorting.first, userSorting.second );
		}

	__super::OnUpdate( pSubject, pMessage );		// this will update list items
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
	, m_destEditor( true )
	, m_srcStatic( CThemeItem( L"HEADER", HP_HEADERITEM, HIS_NORMAL ) )
	, m_destStatic( CThemeItem( L"HEADER", HP_HEADERITEM, HIS_NORMAL ) )
	, m_syncScrolling( SB_BOTH )
{
	RegisterCtrlLayout( ARRAY_PAIR( layout::stylesEdit ) );
//	SetUseLazyUpdateData( true );			// need data transfer on OnSetActive()/OnKillActive()

	m_srcStatic.m_useText = m_destStatic.m_useText = true;

	m_srcEdit.SetKeepSelOnFocus();
	m_srcEdit.SetShowFocus();
	m_destEditor.SetKeepSelOnFocus();
	m_destEditor.SetUsePasteTransact();			// get just one EN_CHANGE notification after paste
	m_destEditor.SetShowFocus();
}

CRenameEditPage::~CRenameEditPage()
{
}

void CRenameEditPage::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage ) override
{
	pMessage;
	ASSERT_PTR( m_hWnd );

	if ( !IsInternalChange() )
		if ( m_pParentDlg->GetFileModel() == pSubject )
			SetupFileEdits();
}

void CRenameEditPage::EnsureVisibleItem( const CRenameItem* pRenameItem ) override
{
	if ( pRenameItem != NULL )
	{
		size_t linePos = utl::FindPos( m_pParentDlg->GetRenameItems(), pRenameItem );
		if ( linePos != utl::npos )
		{
			m_srcEdit.SetSelRange( m_srcEdit.GetLineRange( static_cast<CTextEdit::TLine>( linePos ) ) );
			m_destEditor.SetSelRange( m_destEditor.GetLineRange( static_cast<CTextEdit::TLine>( linePos ) ) );
		}
	}
}

void CRenameEditPage::CommitLocalEdits( void )
{
	if ( m_hWnd != NULL && m_destEditor.GetModify() )
		if ( InputDestPaths() && AnyChanges() )
			m_pParentDlg->SafeExecuteCmd( new CChangeDestPathsCmd( m_pParentDlg->GetFileModel(), m_newDestPaths, _T("Edit destination filenames manually") ) );
}

void CRenameEditPage::SetupFileEdits( void )
{
	const std::vector< CRenameItem* >& renameItems = m_pParentDlg->GetRenameItems();
	CDisplayFilenameAdapter* pDisplayAdapter = m_pParentDlg->GetDisplayFilenameAdapter();

	std::vector< std::tstring > srcPaths, destPaths;
	srcPaths.reserve( renameItems.size() );
	destPaths.reserve( renameItems.size() );

	for ( std::vector< CRenameItem* >::const_iterator itRenameItem = renameItems.begin(); itRenameItem != renameItems.end(); ++itRenameItem )
	{
		srcPaths.push_back( pDisplayAdapter->FormatFilename( ( *itRenameItem )->GetSrcPath() ) );
		destPaths.push_back( pDisplayAdapter->FormatFilename( ( *itRenameItem )->GetDestPath() ) );
	}

	bool firstSetup = 0 == ::GetWindowTextLength( m_srcEdit );

	m_srcEdit.SetText( str::Join( srcPaths, CTextEdit::s_lineEnd ) );
	m_destEditor.SetText( str::Join( destPaths, CTextEdit::s_lineEnd ) );

	if ( firstSetup )
		EnsureVisibleItem( m_pParentDlg->GetFirstErrorItem< CRenameItem >() );

	m_syncScrolling.Synchronize( &m_destEditor );			// sync v-scroll pos
}

bool CRenameEditPage::InputDestPaths( void )
{
	m_newDestPaths.clear();

	std::vector< std::tstring > filenames;
	str::Split( filenames, m_destEditor.GetText().c_str(), CTextEdit::s_lineEnd );

	for ( std::vector< std::tstring >::const_iterator itFilename = filenames.begin(); itFilename != filenames.end(); ++itFilename )
	{
		std::tstring coreFilename = *itFilename;
		str::Trim( coreFilename );

		if ( !path::IsValid( coreFilename ) )
			return false;
	}

	if ( !filenames.empty() )			// an extra line end?
	{
		std::vector< std::tstring >::iterator itLastPath = filenames.end() - 1;
		if ( itLastPath->empty() )
			filenames.erase( itLastPath );
	}

	const std::vector< CRenameItem* >& renameItems = m_pParentDlg->GetRenameItems();

	if ( filenames.size() != renameItems.size() )
		return false;

	CDisplayFilenameAdapter* pDisplayAdapter = m_pParentDlg->GetDisplayFilenameAdapter();

	m_newDestPaths.reserve( renameItems.size() );

	for ( size_t i = 0; i != renameItems.size(); ++i )
		m_newDestPaths.push_back( pDisplayAdapter->ParseFilename( filenames[ i ], renameItems[ i ]->GetSafeDestPath() ) );		// DEST[SRC]_dir_path/filename

	return true;
}

bool CRenameEditPage::AnyChanges( void ) const
{
	const std::vector< CRenameItem* >& renameItems = m_pParentDlg->GetRenameItems();
	ASSERT( m_newDestPaths.size() == renameItems.size() );

	for ( size_t i = 0; i != renameItems.size(); ++i )
		if ( renameItems[ i ]->GetDestPath().Get() != m_newDestPaths[ i ].Get() )			// case sensitive compare
			return true;

	return false;
}

void CRenameEditPage::DoDataExchange( CDataExchange* pDX ) override
{
	bool firstInit = NULL == m_srcEdit.m_hWnd;

	DDX_Control( pDX, IDC_RENAME_SRC_FILES_STATIC, m_srcStatic );
	DDX_Control( pDX, IDC_RENAME_DEST_FILES_STATIC, m_destStatic );
	DDX_Control( pDX, IDC_RENAME_SRC_FILES_EDIT, m_srcEdit );
	DDX_Control( pDX, IDC_RENAME_DEST_FILES_EDIT, m_destEditor );

	if ( firstInit )
	{
		m_srcEdit.AddToSyncScrolling( &m_syncScrolling );
		m_destEditor.AddToSyncScrolling( &m_syncScrolling );

		if ( m_pParentDlg->IsInitialized() )						// not first time sheet creation? - prevent double init if this was the last active page
		{
			// page lazy activation
			if ( !m_pParentDlg->HasDestPaths() )
				CResetDestinationsCmd( m_pParentDlg->GetFileModel() ).Execute();	// this will call OnUpdate() for all observers
			else
				OnUpdate( m_pParentDlg->GetFileModel(), NULL );		// initialize the page
		}

		m_destEditor.EnsureCaretVisible();
	}

	CBaseRenamePage::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CRenameEditPage, CBaseRenamePage )
	ON_WM_CTLCOLOR()
	ON_EN_CHANGE( IDC_RENAME_DEST_FILES_EDIT, OnEnChange_DestPaths )
	ON_EN_KILLFOCUS( IDC_RENAME_DEST_FILES_EDIT, OnEnKillFocus_DestPaths )
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

void CRenameEditPage::OnEnChange_DestPaths( void )
{
	CScopedInternalChange scopedInternal( this );

	if ( !InputDestPaths() )
	{
		m_destEditor.Undo();
		ui::BeepSignal();
	}
}

void CRenameEditPage::OnEnKillFocus_DestPaths( void )
{
	CommitLocalEdits();
}
