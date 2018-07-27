
#include "stdafx.h"
#include "RenamePages.h"
#include "RenameItem.h"
#include "RenameFilesDialog.h"
#include "GeneralOptions.h"
#include "FileModel.h"
#include "resource.h"
#include "utl/LongestCommonSubsequence.h"
#include "utl/UtilitiesEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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

//	m_accelPool.AddAccelTable( new CAccelTable( IDD_GENERAL_PAGE ) );
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

	m_fileListCtrl.DeleteAllItems();

	for ( unsigned int pos = 0; pos != m_pParentDlg->GetRenameItems().size(); ++pos )
	{
		CRenameItem* pItem = m_pParentDlg->GetRenameItems()[ pos ];

		m_fileListCtrl.InsertObjectItem( pos, pItem );		// Source
		m_fileListCtrl.SetSubItemText( pos, Destination, pItem->GetDestPath().GetNameExt() );
	}

	m_fileListCtrl.SetupDiffColumnPair( Source, Destination, path::GetMatch() );
	m_fileListCtrl.InitialSortList();
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

void CRenameListPage::EnsureVisibleItem( const CPathItemBase* pItem )
{
	if ( pItem != NULL )
		m_fileListCtrl.EnsureVisibleObject( pItem );

	InvalidateFiles();				// trigger some highlighting
}

void CRenameListPage::InvalidateFiles( void )
{
	m_fileListCtrl.Invalidate();
}

void CRenameListPage::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const
{
	subItem;

	static const ui::CTextEffect s_errorBk( ui::Regular, CLR_NONE, app::ColorErrorBk );
	const CRenameItem* pItem = CReportListControl::AsPtr< CRenameItem >( rowKey );

	if ( m_pParentDlg->IsErrorItem( pItem ) )
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
	, m_syncScrolling( SB_BOTH /*SB_VERT*/ )
{
	RegisterCtrlLayout( layout::stylesEdit, COUNT_OF( layout::stylesEdit ) );
	m_srcStatic.m_useText = m_destStatic.m_useText = true;

	m_srcEdit.SetKeepSelOnFocus();
	m_srcEdit.SetShowFocus();
	m_destEditor.SetKeepSelOnFocus();
	m_destEditor.SetShowFocus();
}

CRenameEditPage::~CRenameEditPage()
{
}

void CRenameEditPage::OnUpdate( utl::ISubject* pSubject, utl::IMessage* pMessage )
{
	pMessage;
	ASSERT_PTR( m_hWnd );

	if ( m_pParentDlg->GetFileModel() == pSubject )
		SetupFileEdits();
}

void CRenameEditPage::EnsureVisibleItem( const CPathItemBase* pItem )
{
	pItem;
}

void CRenameEditPage::InvalidateFiles( void )
{
}

void CRenameEditPage::SetupFileEdits( void )
{
	const std::vector< CRenameItem* >& renameItems = m_pParentDlg->GetRenameItems();

	std::vector< std::tstring > srcPaths, destPaths;
	srcPaths.reserve( renameItems.size() );
	destPaths.reserve( renameItems.size() );

	for ( std::vector< CRenameItem* >::const_iterator itRenameItem = renameItems.begin(); itRenameItem != renameItems.end(); ++itRenameItem )
	{
		srcPaths.push_back( ( *itRenameItem )->GetDisplayCode() );
		destPaths.push_back( ( *itRenameItem )->GetDestPath().GetNameExt() );
	}

	m_srcEdit.SetText( str::Join( srcPaths, CTextEdit::s_lineEnd ) );
	m_destEditor.SetText( str::Join( destPaths, CTextEdit::s_lineEnd ) );

	m_syncScrolling.Synchronize( &m_destEditor );			// sync v-scroll pos
}

void CRenameEditPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_srcEdit.m_hWnd;

	DDX_Control( pDX, IDC_RENAME_SRC_FILES_EDIT, m_srcEdit );
	DDX_Control( pDX, IDC_RENAME_DEST_FILES_EDIT, m_destEditor );
	DDX_Control( pDX, IDC_RENAME_SRC_FILES_STATIC, m_srcStatic );
	DDX_Control( pDX, IDC_RENAME_DEST_FILES_STATIC, m_destStatic );

	if ( firstInit )
	{
		m_srcEdit.AddToSyncScrolling( &m_syncScrolling );
		m_destEditor.AddToSyncScrolling( &m_syncScrolling );

		if ( m_pParentDlg->IsInitialized() )						// not first time sheet creation? - prevent double init if this was the last active page
			if ( !m_pParentDlg->HasDestPaths() )
				m_pParentDlg->GetFileModel()->ResetDestinations();	// this will call OnUpdate() for all observers
			else
				OnUpdate( m_pParentDlg->GetFileModel(), NULL );		// initialize the page

		m_destEditor.PostMessage( EM_SCROLLCARET );					// the only way to scroll the edit to make the caret visible (if outside client rect)
	}

	CBaseRenamePage::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CRenameEditPage, CBaseRenamePage )
END_MESSAGE_MAP()
