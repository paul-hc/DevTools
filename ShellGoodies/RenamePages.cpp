
#include "stdafx.h"
#include "RenamePages.h"
#include "RenameItem.h"
#include "RenameFilesDialog.h"
#include "GeneralOptions.h"
#include "FileModel.h"
#include "EditingCommands.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/LongestCommonSubsequence.h"
#include "utl/UI/UtilitiesEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/ReportListControl.hxx"


namespace reg
{
	static const TCHAR section_list[] = _T("RenameDialog\\FilesSheet\\ListPage\\List");
}


// CBaseRenamePage implementation

CBaseRenamePage::CBaseRenamePage( UINT templateId, CRenameFilesDialog* pParentDlg )
	: CLayoutPropertyPage( templateId )
	, m_pParentDlg( pParentDlg )
{
	ASSERT_PTR( m_pParentDlg );
}


// CRenameListPage implementation

namespace layout
{
	static const CLayoutStyle stylesList[] =
	{
		{ IDC_FILE_RENAME_LIST, Size }
	};
}

CRenameListPage::CRenameListPage( CRenameFilesDialog* pParentDlg )
	: CBaseRenamePage( IDD_REN_LIST_PAGE, pParentDlg )
	, m_fileListCtrl( IDC_FILE_RENAME_LIST )
{
	RegisterCtrlLayout( layout::stylesList, COUNT_OF( layout::stylesList ) );

	m_fileListCtrl.SetSection( reg::section_list );
	m_fileListCtrl.SetUseAlternateRowColoring();
	m_fileListCtrl.SetTextEffectCallback( this );

	// display filenames depending on m_ignoreExtension
	m_fileListCtrl.SetSubjectAdapter( m_pParentDlg->GetDisplayFilenameAdapter() );

	CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );
	// Note: focus retangle is not painted properly without double-buffering
}

CRenameListPage::~CRenameListPage()
{
}

void CRenameListPage::SetupFileListView( void )
{
	CScopedListTextSelection sel( &m_fileListCtrl );

	CScopedLockRedraw freeze( &m_fileListCtrl );
	CScopedInternalChange internalChange( &m_fileListCtrl );
	CDisplayFilenameAdapter* pDisplayAdapter = m_pParentDlg->GetDisplayFilenameAdapter();

	m_fileListCtrl.DeleteAllItems();

	for ( unsigned int pos = 0; pos != m_pParentDlg->GetRenameItems().size(); ++pos )
	{
		CRenameItem* pRenameItem = m_pParentDlg->GetRenameItems()[ pos ];

		m_fileListCtrl.InsertObjectItem( pos, pRenameItem );		// Source
		m_fileListCtrl.SetSubItemText( pos, Destination, pDisplayAdapter->FormatFilename( pRenameItem->GetDestPath() ) );
	}

	m_fileListCtrl.SetupDiffColumnPair( Source, Destination, path::TGetMatch() );
	m_fileListCtrl.InitialSortList();		// store original order and sort by current criteria
}

void CRenameListPage::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage )
{
	pMessage;
	ASSERT_PTR( m_hWnd );

	if ( m_pParentDlg->GetFileModel() == pSubject )
		SetupFileListView();
	else if ( &CGeneralOptions::Instance() == pSubject )
		CGeneralOptions::Instance().ApplyToListCtrl( &m_fileListCtrl );
}

void CRenameListPage::EnsureVisibleItem( const CRenameItem* pRenameItem )
{
	if ( pRenameItem != NULL )
		m_fileListCtrl.EnsureVisibleObject( pRenameItem );

	InvalidateFiles();				// trigger some highlighting
}

void CRenameListPage::InvalidateFiles( void )
{
	m_fileListCtrl.Invalidate();
}

void CRenameListPage::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	subItem, pCtrl;

	static const ui::CTextEffect s_errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );
	const CRenameItem* pRenameItem = CReportListControl::AsPtr<CRenameItem>( rowKey );

	if ( m_pParentDlg->IsErrorItem( pRenameItem ) )
		rTextEffect.Combine( s_errorBk );
}

void CRenameListPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_fileListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_FILE_RENAME_LIST, m_fileListCtrl );

	if ( firstInit )
		OnUpdate( m_pParentDlg->GetFileModel(), NULL );			// initialize the page

	CBaseRenamePage::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CRenameListPage, CBaseRenamePage )
END_MESSAGE_MAP()


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
	RegisterCtrlLayout( layout::stylesEdit, COUNT_OF( layout::stylesEdit ) );
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

void CRenameEditPage::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage )
{
	pMessage;
	ASSERT_PTR( m_hWnd );

	if ( !IsInternalChange() )
		if ( m_pParentDlg->GetFileModel() == pSubject )
			SetupFileEdits();
}

void CRenameEditPage::EnsureVisibleItem( const CRenameItem* pRenameItem )
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

void CRenameEditPage::DoDataExchange( CDataExchange* pDX )
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
