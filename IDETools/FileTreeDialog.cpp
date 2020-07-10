
#include "stdafx.h"
#include "FileTreeDialog.h"
#include "IncludeOptionsDialog.h"
#include "ProjectContext.h"
#include "FileLocatorDialog.h"
#include "SourceFileParser.h"
#include "Application_fwd.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/FileSystem.h"
#include "utl/UI/Clipboard.h"
#include "utl/UI/CtrlUiState.h"
#include "utl/UI/CmdUpdate.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/ShellDialogs.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_FILE_TREE, Size },
		{ IDC_FULLPATH_STATIC, MoveY },
		{ IDC_FULLPATH_EDIT, MoveY | SizeX },
		{ IDC_LAZY_PARSING_CHECK, MoveY },
		{ IDC_REMOVE_DUPLICATES_CHECK, MoveY },
		{ IDC_DEPTH_LEVEL_STATIC, MoveY },
		{ IDC_DEPTH_LEVEL_COMBO, MoveY },
		{ IDC_VIEW_MODE_STATIC, MoveY },
		{ IDC_VIEW_MODE_COMBO, MoveY },
		{ IDC_ORDER_STATIC, MoveY },
		{ IDC_ORDER_COMBO, MoveY },
		{ IDOK, MoveX },
		{ IDCANCEL, MoveX },
		{ IDC_LOCATE_FILE, MoveX },
		{ IDC_BROWSE_FILE, MoveX },
		{ ID_OPTIONS, MoveX },
		{ ID_EDIT_COPY, MoveX }
	};
}


CFileTreeDialog::CFileTreeDialog( const std::tstring& rootPath, CWnd* pParent )
	: CLayoutDialog( IDD_FILE_TREE_DIALOG, pParent )
	, m_rootPath( rootPath )
	, m_rOpt( CIncludeOptions::Instance() )

	, m_sourceLineNo( 0 )
	, m_nestingLevel( 0 )
	, m_depthLevelCombo( &CIncludeOptions::GetTags_DepthLevel() )
	, m_selFilePathEdit( ui::FilePath )
	, m_viewModeCombo( &CIncludeOptions::GetTags_ViewMode() )
	, m_orderCombo( &CIncludeOptions::GetTags_Ordering() )
	, m_accelTreeFocus( IDR_FILETREEFOCUS_ACCEL )
{
	m_regSection = _T("FileTreeDialog");
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	LoadDlgIcon( IDI_INCBROWSER_ICON );
	m_accelPool.AddAccelTable( new CAccelTable( IDR_FILETREE_ACCEL ) );

	m_selFilePathEdit.GetDetailButton()->SetIconId( IDC_FULLPATH_EDIT );

	m_rOpt.m_lastBrowsedFile = m_rootPath.Get();
}

CFileTreeDialog::~CFileTreeDialog()
{
	m_rOpt.Save();

	Clear();
}

void CFileTreeDialog::SetRootPath( const fs::CPath& rootPath )
{
	bool restoreTreeUi = m_hWnd != NULL && m_rOpt.m_selRecover && m_rootPath.Equivalent( rootPath.Get() );
	CTreeCtrlUiState treeState;

	if ( restoreTreeUi )
		treeState.SaveVisualState( m_treeCtrl );

	m_rootPath = rootPath;
	BuildIncludeTree();

	if ( restoreTreeUi )
		treeState.RestoreVisualState( m_treeCtrl );
}

void CFileTreeDialog::Clear( void )
{
	m_nestingLevel = 0;
	m_originalItems.clear();
	utl::ClearOwningContainer( m_treeItems );
}

void CFileTreeDialog::BuildIncludeTree( void )
{
	CWaitCursor busyCursor;
	Clear();

	CSourceFileParser rootParser( m_rootPath );		// add the root file path

	// add always desired source paths
	rootParser.AddSourceFiles( m_rOpt.m_fnAdded.GetPaths() );

	// fill-in the tree control
	CScopedLockRedraw freeze( &m_treeCtrl );
	CScopedInternalChange internalChange( &m_treeCtrl );

	m_treeCtrl.DeleteAllItems();

	std::vector< CIncludeNode* > treeItems;
	rootParser.SwapIncludeNodes( treeItems );		// acquire ownership
	for ( std::vector< CIncludeNode* >::const_iterator itItem = treeItems.begin(); itItem != treeItems.end(); ++itItem )
	{
		int orderIndex = 0;							// keep root items unsorted
		bool originalItem;
		TreeItemPair itemPair = AddTreeItem( TVI_ROOT, *itItem, orderIndex, originalItem );
		if ( itemPair.first != NULL )
		{
			if ( originalItem )
				AddIncludedChildren( itemPair, !m_rOpt.m_lazyParsing );		// insert children
		}
		else
			delete *itItem;
	}
	ReorderChildren();

	if ( HTREEITEM hRootItem = m_treeCtrl.GetRootItem() )
		if ( m_rOpt.m_openBlownUp )
			m_treeCtrl.ExpandBranch( hRootItem );
		else
			m_treeCtrl.Expand( hRootItem, TVE_EXPAND );

	CmUpdateDialogTitle();
}

bool CFileTreeDialog::AddIncludedChildren( TreeItemPair& rParentPair, bool doRecurse /*= true*/, bool avoidCircularDependency /*= false*/ )
{
	// check for nesting level overflow if we make full (non-delayed) parsing
	if ( !m_rOpt.m_lazyParsing && ( m_rOpt.m_maxNestingLevel > 0 && ( m_nestingLevel + 1 ) >= m_rOpt.m_maxNestingLevel ) )
		return false;

	if ( avoidCircularDependency && IsCircularDependency( rParentPair ) )
	{
		m_treeCtrl.SetItemState( rParentPair.first, TVIS_BOLD, TVIS_BOLD );
		return false;		// reject this circular dependency (and leave it marked as parsed)
	}

	++m_nestingLevel;

	CSourceFileParser fileParser( rParentPair.second->m_path );
	fileParser.ParseRootFile( m_rOpt.m_maxParseLines );

	int addedCount = 0, orderIndex = 0;
	std::vector< CIncludeNode* > treeItems;
	fileParser.SwapIncludeNodes( treeItems );			// acquire ownership

	for ( std::vector< CIncludeNode* >::const_iterator itItem = treeItems.begin(); itItem != treeItems.end(); ++itItem )
	{
		bool originalItem;
		TreeItemPair itemPair = AddTreeItem( rParentPair.first, *itItem, orderIndex, originalItem );
		if ( itemPair.first != NULL )
		{
			++addedCount;
			if ( doRecurse && originalItem )
				AddIncludedChildren( itemPair, true, avoidCircularDependency );		// deep recurse for children
		}
		else
			delete *itItem;
	}

	--m_nestingLevel;
	rParentPair.second->SetParsedChildrenCount( addedCount );
	return addedCount != 0;
}

TreeItemPair CFileTreeDialog::AddTreeItem( HTREEITEM hParent, CIncludeNode* pItemInfo, int& rOrderIndex, bool& rOriginalItem )
{
	ASSERT_PTR( pItemInfo );
	ASSERT( pItemInfo->IsValidFile() );
	static const TreeItemPair nullPair( NULL, NULL );

	if ( IsFileExcluded( pItemInfo->m_path ) )
		return nullPair;

	std::tstring uniqueFilePath = path::MakeCanonical( pItemInfo->m_path.GetPtr() );

	PathToItemMap::const_iterator itFound = m_originalItems.find( uniqueFilePath );
	rOriginalItem = itFound == m_originalItems.end();

	if ( m_rOpt.m_noDuplicates && !rOriginalItem )
		return nullPair;

	m_treeItems.push_back( pItemInfo );				// give ownership to the tree control

	ft::FileType imageIndex = pItemInfo->m_fileType;
	std::tstring itemText = BuildItemText( pItemInfo );

	pItemInfo->SetOrderIndex( rOrderIndex++ );		// post-increment only if item is added into tree

	TVINSERTSTRUCT is;
	is.hParent = hParent;
	is.hInsertAfter = TVI_LAST;
	// TV_ITEM
	is.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_STATE;
	is.item.pszText = (TCHAR*)itemText.c_str();
	is.item.iImage = is.item.iSelectedImage = imageIndex;
	is.item.lParam = (LPARAM)pItemInfo;
    is.item.state = rOriginalItem ? 0 : TVIS_CUT;
    is.item.stateMask = TVIS_CUT;
	if ( m_rOpt.m_lazyParsing && rOriginalItem )
	{
		// display the "+" expand button (lazy binding);
		// in order to avoid infinite recursion, we only do this for first occurences of the included files (original items).
		is.item.mask |= TVIF_CHILDREN;
		is.item.cChildren = I_CHILDRENCALLBACK;			// sends a TVN_GETDISPINFO notification for cChildren
	}

	HTREEITEM hItem = m_treeCtrl.InsertItem( &is );
	ASSERT_PTR( hItem );
	if ( rOriginalItem )
		m_originalItems[ uniqueFilePath ] = hItem;
	return TreeItemPair( hItem, pItemInfo );
}

// checks for circular dependencies between the current specified file and the parent nodes

bool CFileTreeDialog::IsCircularDependency( const TreeItemPair& currItem ) const
{
	for ( HTREEITEM hParent = currItem.first; ( hParent = m_treeCtrl.GetParentItem( hParent ) ) != NULL; )
		if ( const CIncludeNode* pParentItem = GetItemInfo( hParent) )
			if ( pParentItem->m_path == currItem.second->m_path )
				return true;		// found circular dependency

	return false;
}

// if an item is not already parsed, it parses and expandes it, if there are any kids

bool CFileTreeDialog::SafeBindItem( HTREEITEM hItem, CIncludeNode* pTreeItem )
{
	ASSERT_PTR( hItem );
	ASSERT_PTR( pTreeItem );

	if ( pTreeItem->IsParsed() )
		return false;

	TreeItemPair itemToBindPair( hItem, pTreeItem );
	PostMessage( WM_COMMAND, CM_UPDATE_DIALOG_TITLE );				// delayed update the dialog title (for file count statistics)
	return AddIncludedChildren( itemToBindPair, false, true );
}

bool CFileTreeDialog::UpdateCurrentSelection( void )
{
	fs::CPath fullPath;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	if ( hSelItem != NULL )
		if ( const CIncludeNode* pItemInfo = GetItemInfo( hSelItem ) )
			fullPath = pItemInfo->m_path;

	ui::SetWindowText( m_selFilePathEdit, fullPath.Get() );
	m_fileAssoc.SetPath( fullPath );
	if ( m_fileAssoc.IsValidKnownAssoc() )
		m_complemFilePath = m_fileAssoc.GetComplementaryAssoc();

	if ( m_complemFilePath.IsEmpty() )
		m_complemFilePath = fullPath;		// set to current file if no associations
	return hSelItem != NULL;
}

bool CFileTreeDialog::SetViewMode( ViewMode viewMode )
{
	if ( viewMode == m_rOpt.m_viewMode )
		return false;	// nothing changed

	m_rOpt.m_viewMode = viewMode;
	RefreshItemsText();
	return true;
}

void _RefreshItemText( CTreeControl* pTreeCtrl, HTREEITEM hItem, CFileTreeDialog* pDialog, int nestingLevel )
{
	nestingLevel;
	pTreeCtrl->SetItemText( hItem, pDialog->BuildItemText( pTreeCtrl->GetItemDataAs< CIncludeNode* >( hItem ) ).c_str() );
}

void CFileTreeDialog::RefreshItemsText( HTREEITEM hItem /*= TVI_ROOT*/ )
{
	ForEach( (IterFunc)_RefreshItemText, hItem, this );
}

std::tstring CFileTreeDialog::BuildItemText( const CIncludeNode* pItemInfo, ViewMode viewMode /*= vmDefaultMode*/ ) const
{
	ASSERT_PTR( pItemInfo );
	switch ( viewMode == vmDefaultMode ? m_rOpt.m_viewMode : viewMode )
	{
		case vmFileName:
			return pItemInfo->m_path.GetNameExt();
		case vmRelPathFileName:
			return pItemInfo->m_lineNo > 0 ? pItemInfo->m_includeTag.GetFilePath().Get() : BuildItemText( pItemInfo, vmFileName );
		case vmIncDirective:
			if ( pItemInfo->m_lineNo <= 0 || !pItemInfo->m_includeTag.IsEnclosed() )
				return BuildItemText( pItemInfo, vmFileName );
			else
				return pItemInfo->m_includeTag.FormatIncludeStatement();
		case vmFullPath:
			return pItemInfo->m_path.Get();
	}

	return _T("???");
}

void CFileTreeDialog::ForEach( IterFunc pIterFunc, HTREEITEM hItem /*= TVI_ROOT*/, void* pArgs /*= NULL*/, int nestingLevel /*= -1*/ )
{
	ASSERT_PTR( pIterFunc );

	if ( hItem != TVI_ROOT )
		pIterFunc( &m_treeCtrl, hItem, pArgs, nestingLevel );

	for ( HTREEITEM hChild = m_treeCtrl.GetChildItem( hItem ); hChild != NULL; hChild = m_treeCtrl.GetNextSiblingItem( hChild ) )
		ForEach( pIterFunc, hChild, pArgs, nestingLevel + 1 );
}

void _SafeBindItem( CTreeControl* pTreeCtrl, HTREEITEM hItem, CFileTreeDialog* pDialog, int nestingLevel )
{
	pTreeCtrl, nestingLevel;
	pDialog->SafeBindItem( hItem, pDialog->GetItemInfo( hItem ) );
}

void _AddMatchingPath( const CTreeControl* pTreeCtrl, HTREEITEM hItem, CMatchingItems* pResult, int nestingLevel )
{
	nestingLevel;
	ASSERT_PTR( pResult );
	CIncludeNode* pItemInfo = pTreeCtrl->GetItemDataAs< CIncludeNode* >( hItem );

	if ( pResult->IsPathMatch( pItemInfo ) )
		pResult->AddMatch( hItem, pItemInfo );
}

HTREEITEM CFileTreeDialog::FindNextItem( HTREEITEM hStartItem, bool forward /*= true*/ )
{
	if ( NULL == hStartItem )
		return NULL;

	ASSERT( m_treeCtrl.IsRealItem( hStartItem ) );

	BindAllItems();			// force binding all items

	CMatchingItems args( hStartItem, GetItemInfo( hStartItem )->m_path.Get() );
	ForEach( (IterFunc)_AddMatchingPath, TVI_ROOT, &args );

	ASSERT( args.m_startPos < args.m_matches.size() );
	if ( 1 == args.m_matches.size() )
		return hStartItem;		// same occurence with start

	size_t matchPos = utl::CircularAdvance( args.m_startPos, args.m_matches.size(), forward );
	return args.m_matches[ matchPos ].first;
}

HTREEITEM CFileTreeDialog::FindOriginalItem( HTREEITEM hStartItem )
{
	if ( NULL == hStartItem )
		return NULL;

	const CIncludeNode* pStartInfo = GetItemInfo( hStartItem );
	std::tstring fullPath = path::MakeCanonical( pStartInfo->m_path.GetPtr() );

	PathToItemMap::const_iterator itFound = m_originalItems.find( fullPath );
	if ( itFound == m_originalItems.end() )
		return NULL;
	return itFound->second;
}

bool CFileTreeDialog::SetOrder( Ordering ordering )
{
	if ( ordering == m_rOpt.m_ordering )
		return false;
	m_rOpt.m_ordering = ordering;
	m_treeCtrl.SetRedraw( FALSE );
	ReorderChildren();
	m_treeCtrl.SetRedraw( TRUE );
	m_treeCtrl.Invalidate( FALSE );
	return true;
}

int CALLBACK SortCallback( const CIncludeNode* pLeft, const CIncludeNode* pRight, CFileTreeDialog* pDialog )
{
	ASSERT( pLeft != NULL && pRight != NULL );
	switch ( pDialog->m_rOpt.m_ordering )
	{
		case ordNormal:
			return pLeft->GetOrderIndex() - pRight->GetOrderIndex();
		case ordReverse:
			return pRight->GetOrderIndex() - pLeft->GetOrderIndex();
		case ordAlphaText:
			return lstrcmp( pDialog->BuildItemText( pLeft ).c_str(), pDialog->BuildItemText( pRight ).c_str() );
		case ordAlphaFileName:
			return pred::CompareNaturalPath()( pLeft->m_path.GetNameExt(), pRight->m_path.GetNameExt() );
		default:
			ASSERT( false );
			return 0;
	}
}

bool CFileTreeDialog::ReorderChildren( HTREEITEM hParent /*= TVI_ROOT*/ )
{
	if ( TVI_ROOT == hParent )
	{	// special case for root items -> sort only their children but not themselves
		for ( HTREEITEM hItemRoot = m_treeCtrl.GetRootItem(); hItemRoot != NULL; hItemRoot = m_treeCtrl.GetNextSiblingItem( hItemRoot ) )
			ReorderChildren( hItemRoot );
		return true;
	}
	else
	{
		TVSORTCB sortData = { hParent, (PFNTVCOMPARE)SortCallback, (LPARAM)this };
		return TreeView_SortChildrenCB( m_treeCtrl, &sortData, TRUE ) != FALSE;
	}
}

bool CFileTreeDialog::IsOriginalItem( HTREEITEM hItem ) const
{
	const CIncludeNode* pTreeItem = GetItemInfo( hItem );
	PathToItemMap::const_iterator itFound = m_originalItems.find( pTreeItem->m_path.Get() );
	return itFound != m_originalItems.end() && hItem == itFound->second;
}

bool CFileTreeDialog::IsFileExcluded( const fs::CPath& path ) const
{
	if ( path == m_rootPath )
		return false;		// never exclude m_rootPath itself

	return m_rOpt.m_fnIgnored.AnyIsPartOf( path );
}

bool CFileTreeDialog::HasValidComplementary( void ) const		// true if the selected file has a valid complementary
{
	if ( m_complemFilePath.IsEmpty() )
		return false;

	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
		return GetItemInfo( hSelItem )->m_path != m_complemFilePath;
	return false;
}

TreeItemPair CFileTreeDialog::GetSelectedItem( void ) const
{
	TreeItemPair itemPair( m_treeCtrl.GetSelectedItem(), NULL );
	if ( itemPair.first != NULL )
		itemPair.second = GetItemInfo( itemPair.first );
	return itemPair;
}

void CFileTreeDialog::UpdateOptionCtrl( void )
{
	CheckDlgButton( IDC_LAZY_PARSING_CHECK, m_rOpt.m_lazyParsing );
	CheckDlgButton( IDC_REMOVE_DUPLICATES_CHECK, m_rOpt.m_noDuplicates );

	ui::EnableWindow( m_depthLevelCombo, !m_rOpt.m_lazyParsing );
	if ( !m_rOpt.m_lazyParsing )
		m_depthLevelCombo.SetValue( m_rOpt.m_maxNestingLevel );
}

void CFileTreeDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_depthLevelCombo.m_hWnd;

	DDX_Control( pDX, IDC_DEPTH_LEVEL_COMBO, m_depthLevelCombo );
	DDX_Control( pDX, IDC_FILE_TREE, m_treeCtrl );
	DDX_Control( pDX, IDC_FULLPATH_EDIT, m_selFilePathEdit );
	DDX_Control( pDX, IDC_VIEW_MODE_COMBO, m_viewModeCombo );
	DDX_Control( pDX, IDC_ORDER_COMBO, m_orderCombo );
	ui::DDX_ButtonIcon( pDX, IDC_LOCATE_FILE, IDD_FILE_LOCATOR_DIALOG );
	ui::DDX_ButtonIcon( pDX, IDC_BROWSE_FILE, ID_FILE_OPEN );
	ui::DDX_ButtonIcon( pDX, ID_OPTIONS );
	ui::DDX_ButtonIcon( pDX, ID_EDIT_COPY );

	ui::DDX_Bool( pDX, IDC_LAZY_PARSING_CHECK, m_rOpt.m_lazyParsing );
	ui::DDX_Bool( pDX, IDC_REMOVE_DUPLICATES_CHECK, m_rOpt.m_noDuplicates );

	if ( firstInit )
	{
		DragAcceptFiles( TRUE );
		m_titlePrefix = ui::GetWindowText( this );
		m_treeCtrl.SetImageList( &ft::GetFileTypeImageList(), TVSIL_NORMAL );
		m_treeCtrl.SetImageList( &ft::GetFileTypeImageList(), TVSIL_STATE );

		if ( CToolTipCtrl* pTreeTooltip = m_treeCtrl.GetToolTips() )
			pTreeTooltip->SetDelayTime( TTDT_INITIAL, 500 );		// modify the initial delay for item hover

		m_depthLevelCombo.SetExtendedUI();

		UpdateOptionCtrl();
		OnToggle_LazyParsing();
		m_viewModeCombo.SetValue( m_rOpt.m_viewMode );
		m_orderCombo.SetValue( m_rOpt.m_ordering );

		BuildIncludeTree();
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CFileTreeDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_WM_DROPFILES()
	ON_WM_INITMENUPOPUP()
	ON_NOTIFY( TVN_SELCHANGED, IDC_FILE_TREE, TVnSelChanged_FileTree )
	ON_NOTIFY( TVN_ITEMEXPANDING, IDC_FILE_TREE, TVnItemExpanding_FileTree )
	ON_NOTIFY( TVN_GETDISPINFO, IDC_FILE_TREE, TVnGetDispInfo_FileTree )
	ON_NOTIFY( NM_CUSTOMDRAW, IDC_FILE_TREE, TVnCustomDraw_FileTree )
	ON_NOTIFY( NM_RCLICK, IDC_FILE_TREE, TVnRclick_FileTree )
	ON_NOTIFY( NM_DBLCLK, IDC_FILE_TREE, TVnDblclk_FileTree )
	ON_NOTIFY( TVN_BEGINLABELEDIT, IDC_FILE_TREE, TVnBeginLabelEdit_FileTree )
	ON_NOTIFY( TVN_GETINFOTIP, IDC_FILE_TREE, TVnGetInfoTip_FileTree )
	ON_CBN_SELCHANGE( IDC_VIEW_MODE_COMBO, CBnSelChangeViewModeCombo )
	ON_CBN_SELCHANGE( IDC_ORDER_COMBO, CBnSelChangeOrder )
	ON_COMMAND( CM_UPDATE_DIALOG_TITLE, CmUpdateDialogTitle )
	ON_BN_CLICKED( ID_FILE_OPEN, OnOK )
	ON_BN_CLICKED( ID_OPTIONS, CmOptions )
	ON_BN_CLICKED( IDC_BROWSE_FILE, CmFileOpen )
	ON_COMMAND( CM_FIND_NEXT, CmFindNext )
	ON_COMMAND( CM_FIND_PREV, CmFindPrev )
	ON_COMMAND( CM_FIND_ORIGIN, CmFindOriginal )
	ON_COMMAND( CM_EXPAND_BRANCH, CmExpandBranch )
	ON_COMMAND( CM_COLAPSE_BRANCH, CmColapseBranch )
	ON_COMMAND( ID_REFRESH, CmRefresh )
	ON_COMMAND( CM_OPEN_COMPLEMENTARY, CmOpenComplementary )
	ON_COMMAND( CM_OPEN_INCLUDE_LINE, CmOpenIncludeLine )
	ON_COMMAND( CM_EXPLORE_FILE, CmExploreFile )
	ON_COMMAND( ID_EDIT_COPY, CmCopyFullPath )
	ON_COMMAND( CM_BROWSE_FROM_SEL, CmBrowseFile )
	ON_CBN_CLOSEUP( IDC_DEPTH_LEVEL_COMBO, CBnCloseupDepthLevel )
	ON_BN_CLICKED( IDC_LAZY_PARSING_CHECK, OnToggle_LazyParsing )
	ON_BN_CLICKED( IDC_REMOVE_DUPLICATES_CHECK, OnToggle_NoDuplicates )
	ON_COMMAND( CM_EDIT_LABEL, CmEditLabel )
	ON_UPDATE_COMMAND_UI( CM_EDIT_LABEL, UUI_EditLabel )
	ON_BN_CLICKED( IDC_LOCATE_FILE, CmLocateFile )
	ON_BN_CLICKED( ID_EDIT_COPY, CmCopyToClipboard )
	ON_COMMAND_RANGE( CM_TEXT_VIEW_FILE, CM_VIEW_FILE, CmViewFile )
	ON_UPDATE_COMMAND_UI( IDOK, UUI_AnySelected )
	ON_UPDATE_COMMAND_UI( CM_OPEN_INCLUDE_LINE, UUI_OpenIncludeLine )
	ON_UPDATE_COMMAND_UI( CM_BROWSE_FROM_SEL, UUI_AnySelected )
	ON_UPDATE_COMMAND_UI_RANGE( CM_FIND_NEXT, CM_FIND_ORIGIN, UUI_FindDupOccurences )
	ON_UPDATE_COMMAND_UI( CM_OPEN_COMPLEMENTARY, UUI_OpenComplementary )
END_MESSAGE_MAP()

BOOL CFileTreeDialog::PreTranslateMessage( MSG* pMsg )
{
	return
		m_accelTreeFocus.TranslateIfOwnsFocus( pMsg, m_hWnd, m_treeCtrl ) ||		// special processing when tree has focus
		CLayoutDialog::PreTranslateMessage( pMsg );
}

void CFileTreeDialog::OnDestroy( void )
{
	CLayoutDialog::OnDestroy();
}

void CFileTreeDialog::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( pWnd == &m_treeCtrl )
	{
		CMenu contextMenu;
		ui::LoadPopupMenu( contextMenu, IDR_CONTEXT_MENU, m_rOpt.m_noDuplicates ? app::IncludeTree_NoDupsPopup : app::IncludeTreePopup );
		ui::TrackPopupMenu( contextMenu, this, screenPos );
	}
}

void CFileTreeDialog::OnDropFiles( HDROP hDropInfo )
{
	std::vector< fs::CPath > filePaths;
	shell::QueryDroppedFiles( filePaths, hDropInfo );

	if ( !filePaths.empty() )
	{
		SetRootPath( filePaths.front() );
		SetForegroundWindow();
	}
}

void CFileTreeDialog::OnInitMenuPopup( CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu )
{
	CLayoutDialog::OnInitMenuPopup( pPopupMenu, nIndex, bSysMenu );
	if ( !bSysMenu )
		ui::UpdateMenuUI( this, pPopupMenu );
}

void CFileTreeDialog::TVnSelChanged_FileTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	NM_TREEVIEW* pNmTreeView = (NM_TREEVIEW*)pNmHdr;
	pNmTreeView;
	UpdateCurrentSelection();
	*pResult = 0;
}

void CFileTreeDialog::TVnItemExpanding_FileTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	NM_TREEVIEW* pNmTreeView = (NM_TREEVIEW*)pNmHdr;

	*pResult = 0;					// validate item expansion
	if ( m_rOpt.m_lazyParsing )
		if ( m_treeCtrl.IsExpandLazyChildren( pNmTreeView ) )
		{
			SafeBindItem( pNmTreeView->itemNew.hItem, reinterpret_cast< CIncludeNode* >( pNmTreeView->itemNew.lParam ) );
			m_treeCtrl.SetItemState( pNmTreeView->itemNew.hItem, TVIS_EXPANDEDONCE, TVIS_EXPANDEDONCE );
		}
}

void CFileTreeDialog::TVnGetDispInfo_FileTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMTVDISPINFO* pDispInfo = (NMTVDISPINFO*)pNmHdr;
	const CIncludeNode* pTreeItem = reinterpret_cast< CIncludeNode* >( pDispInfo->item.lParam );

	if ( HasFlag( pDispInfo->item.mask, TVIF_CHILDREN ) )
		pDispInfo->item.cChildren = pTreeItem->IsParsed() ? pTreeItem->GetParsedChildrenCount() : 1;

	*pResult = 0;
}

void CFileTreeDialog::TVnCustomDraw_FileTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMTVCUSTOMDRAW* pDraw = (NMTVCUSTOMDRAW*)pNmHdr;
	HTREEITEM hItem = reinterpret_cast< HTREEITEM >( pDraw->nmcd.dwItemSpec );
	const CIncludeNode* pTreeItem = reinterpret_cast< CIncludeNode* >( pDraw->nmcd.lItemlParam );

	*pResult = CDRF_DODEFAULT;
	switch ( pDraw->nmcd.dwDrawStage )
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			if ( !IsOriginalItem( hItem ) )
			{
				pDraw->clrText = color::Grey60;
				*pResult = CDRF_NEWFONT;
			}
			else
			{
				switch ( pTreeItem->m_location )
				{
					case inc::AdditionalPath:
						pDraw->clrText = color::Blue;
						break;
					case inc::StandardPath:
						pDraw->clrText = color::Green;
						break;
					case inc::SourcePath:
						pDraw->clrText = color::DarkRed;
						break;
					case inc::LibraryPath:
					case inc::BinaryPath:
						pDraw->clrText = color::Red;
						break;
					case inc::LocalPath:
					case inc::AbsolutePath:
					default:
						return;
				}
				*pResult = CDRF_NEWFONT;
			}
			break;
	}
}

void CFileTreeDialog::TVnBeginLabelEdit_FileTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	TV_DISPINFO* pTvDispInfo = (TV_DISPINFO*)pNmHdr;

	pTvDispInfo;
	if ( CEdit* pLabelEdit = m_treeCtrl.GetEditControl() )
		pLabelEdit->SetReadOnly( TRUE );
	*pResult = 0;
}

void CFileTreeDialog::TVnGetInfoTip_FileTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMTVGETINFOTIP* pNmInfoTip = (NMTVGETINFOTIP*)pNmHdr;

	if ( pNmInfoTip != NULL )
	{
		static std::tstring tooltipTextBuffer;
		CIncludeNode* pTreeItem = GetItemInfo( pNmInfoTip->hItem );
		bool isDup = !IsOriginalItem( pNmInfoTip->hItem );

		tooltipTextBuffer = str::Format( _T("%s%s - %s"),
			pTreeItem->m_path.GetPtr(),
			isDup ? _T(" (duplicate item)") : _T(""),
			inc::GetTags_Location().FormatUi( pTreeItem->m_location ).c_str() );

		// modify the tooltip text color according to duplicate state of this item
		if ( CToolTipCtrl* pTreeTooltip = m_treeCtrl.GetToolTips() )
			pTreeTooltip->SetTipTextColor( ::GetSysColor( isDup ? COLOR_GRAYTEXT : COLOR_INFOTEXT ) );

		pNmInfoTip->pszText = (LPTSTR)tooltipTextBuffer.c_str();
		pNmInfoTip->cchTextMax = INFOTIPSIZE;
	}
	*pResult = 0;
}

void CFileTreeDialog::TVnRclick_FileTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	pNmHdr;
	CPoint mouse, clientPoint;
	HTREEITEM hItem;
	UINT hit = 0;

	::GetCursorPos( &mouse );
	clientPoint = mouse;
	m_treeCtrl.ScreenToClient( &clientPoint );
	hItem = m_treeCtrl.HitTest( clientPoint, &hit );
	if ( hItem != NULL )
		m_treeCtrl.SelectItem( hItem );
	*pResult = 0; // WM_CONTEXTMENU will be further received
}

void CFileTreeDialog::TVnDblclk_FileTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	pNmHdr;
	CPoint mouse;
	HTREEITEM hItem;
	UINT hit = 0;

	::GetCursorPos( &mouse );
	m_treeCtrl.ScreenToClient( &mouse );
	hItem = m_treeCtrl.HitTest( mouse, &hit );
	*pResult = 0;
	if ( hItem != NULL && hit == TVHT_ONITEMLABEL )
	{
		*pResult = 1;
		PostMessage( WM_COMMAND, IDOK );
	}
}

void CFileTreeDialog::CBnCloseupDepthLevel( void )
{
	int maxNestingLevelOrg = m_rOpt.m_maxNestingLevel;

	m_rOpt.m_maxNestingLevel = m_depthLevelCombo.GetValue();
	if ( m_rOpt.m_maxNestingLevel != maxNestingLevelOrg )
		OutputRootPath();
}

void CFileTreeDialog::OnToggle_LazyParsing( void )
{
	m_rOpt.m_lazyParsing = IsDlgButtonChecked( IDC_LAZY_PARSING_CHECK ) != FALSE;
	ui::EnableWindow( m_depthLevelCombo, !m_rOpt.m_lazyParsing );
	m_depthLevelCombo.SetValue( m_rOpt.m_lazyParsing ? 0 : m_rOpt.m_maxNestingLevel );
}

void CFileTreeDialog::OnToggle_NoDuplicates( void )
{
	m_rOpt.m_noDuplicates = IsDlgButtonChecked( IDC_REMOVE_DUPLICATES_CHECK ) != FALSE;
	OutputRootPath();
}

void CFileTreeDialog::CBnSelChangeViewModeCombo( void )
{
	SetViewMode( m_viewModeCombo.GetEnum< ViewMode >() );
}

void CFileTreeDialog::CBnSelChangeOrder( void )
{
	SetOrder( m_orderCombo.GetEnum< Ordering >() );
}

void CFileTreeDialog::CmOptions( void )
{
	CIncludeOptionsDialog dlg( &m_rOpt, this );
	if ( IDOK == dlg.DoModal() )
	{
		OutputRootPath();
		UpdateOptionCtrl();
		GotoDlgCtrl( &m_treeCtrl );
	}
}

void CFileTreeDialog::CmFileOpen( void )
{
	static TCHAR sourceFileFilter[] =
		_T("C++ Files (c;cpp;cxx;h;hxx;hpp;inc;rc;tli;tlh)|*.c;*.cpp;*.cxx;*.h;*.hxx;*.hpp;*.inc;*.rc;*.tli;*.tlh|")
		_T("C++ Source Files (c;cpp;cxx;rc;tli)|*.c;*.cpp;*.cxx;*.rc;*.tli|")
		_T("C++ Header Files (h;hxx;hpp;inc;tlh)|*.h;*.hxx;*.hpp;*.inc;*.tlh|")
		_T("IDL Files (idl;odl;h)|*.idl;*.odl;*.h|")
		_T("Resource Files (rc;h;rh)|*.rc;*.h;*.rh|")
		_T("All Files|*.*||");

	if ( !ui::IsKeyPressed( VK_SHIFT ) )		// SHIFT down -> open from current selected
		if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
			m_rOpt.m_lastBrowsedFile = GetItemInfo( hSelItem )->m_path.Get();

	if ( shell::BrowseForFile( m_rOpt.m_lastBrowsedFile, this, shell::FileOpen, sourceFileFilter ) )
		SetRootPath( m_rOpt.m_lastBrowsedFile );
}

void CFileTreeDialog::CmUpdateDialogTitle( void )
{
	std::tstring title = m_titlePrefix;
	int incCount = m_treeCtrl.GetCount() - 1;
	if ( incCount >= 0 )
	{
		int uniqueIncCount = (int)m_originalItems.size() - 1, dupCount = incCount - uniqueIncCount;
		title = str::Format( m_rOpt.m_noDuplicates || 0 == dupCount ? _T("%s - [%s] - %d files") : _T("%s - [%s] - %d items: %d files + %d duplicates"),
			m_titlePrefix.c_str(),
			m_rootPath.GetNameExt(),
			incCount, uniqueIncCount, dupCount );
	}
	ui::SetWindowText( m_hWnd, title );
}

void CFileTreeDialog::CmFindNext( void )
{
	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
		if ( HTREEITEM hItemFound = FindNextItem( hSelItem, true ) )
			if ( hItemFound != hSelItem )
				m_treeCtrl.SelectItem( hItemFound );
}

void CFileTreeDialog::CmFindPrev( void )
{
	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
		if ( HTREEITEM hItemFound = FindNextItem( hSelItem, false ) )
			if ( hItemFound != hSelItem )
				m_treeCtrl.SelectItem( hItemFound );
}

void CFileTreeDialog::CmFindOriginal( void )
{
	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
		if ( HTREEITEM hItemFound = FindOriginalItem( hSelItem ) )
			if ( hItemFound != hSelItem )
				m_treeCtrl.SelectItem( hItemFound );
}

void CFileTreeDialog::UUI_FindDupOccurences( CCmdUI* pCmdUI )
{
	bool doEnable = false;

	if ( !m_rOpt.m_noDuplicates )
		if ( m_rOpt.m_lazyParsing )
			doEnable = true;		// avoid costly processing (since findNextItem will bind everything) -> assume there are dups
		else
		{	// we may have dups
			if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
			{
				HTREEITEM hItemFound = NULL;
				switch ( pCmdUI->m_nID )
				{
					case CM_FIND_NEXT:		hItemFound = FindNextItem( hSelItem, true ); break;
					case CM_FIND_PREV:		hItemFound = FindNextItem( hSelItem, false ); break;
					case CM_FIND_ORIGIN:	hItemFound = FindOriginalItem( hSelItem ); break;
				}
				doEnable = hItemFound != NULL && hItemFound != hSelItem;
			}
		}

	pCmdUI->Enable( doEnable );
}

void CFileTreeDialog::CmExpandBranch( void )
{
	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
	{
		CWaitCursor busyCursor;
		UINT oldCount = m_treeCtrl.GetCount();
		m_treeCtrl.ExpandBranch( hSelItem );

		if ( m_treeCtrl.GetCount() != oldCount )
			CmUpdateDialogTitle();
	}
}

void CFileTreeDialog::CmColapseBranch( void )
{
	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
		m_treeCtrl.ExpandBranch( hSelItem, false );
}

void CFileTreeDialog::CmRefresh( void )
{
	if ( !m_treeCtrl.IsInternalChange() )
		OutputRootPath();
}

void CFileTreeDialog::OnOK( void )
{
	if ( m_treeCtrl.GetEditControl() != NULL )
		m_treeCtrl.SetFocus();				// first cancel label edit modal state
	else
	{
		if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
			m_rootPath = GetItemInfo( hSelItem )->m_path.Get();
		CLayoutDialog::OnOK();
	}
}

void CFileTreeDialog::OnCancel( void )
{
	if ( m_treeCtrl.GetEditControl() != NULL )
		m_treeCtrl.SetFocus();				// first cancel label edit modal state
	else
		CLayoutDialog::OnCancel();
}

void CFileTreeDialog::CmOpenComplementary( void )
{
	if ( !m_complemFilePath.IsEmpty() )
	{
		m_rootPath = m_complemFilePath;
		EndDialog( CM_OPEN_COMPLEMENTARY );
	}
}

void CFileTreeDialog::CmOpenIncludeLine( void )
{
	// need parent for this
	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
	{
		// store include reference parent file path and line number
		CIncludeNode* pItemInfo = GetItemInfo( hSelItem );
		ASSERT_PTR( pItemInfo );
		m_rootPath = GetItemInfo( m_treeCtrl.GetParentItem( hSelItem ) )->m_path;
		m_sourceLineNo = pItemInfo->m_lineNo;
		EndDialog( CM_OPEN_INCLUDE_LINE );
	}
}

void CFileTreeDialog::CmExploreFile( void )
{
	TreeItemPair itemPair = GetSelectedItem();
	if ( itemPair.first != NULL && itemPair.second->m_path.FileExist() )
		shell::ExploreAndSelectFile( itemPair.second->m_path.GetPtr() );
}

void CFileTreeDialog::CmViewFile( UINT cmdId )
{
	TreeItemPair itemPair = GetSelectedItem();
	if ( itemPair.first != NULL && itemPair.second->m_path.FileExist() )
	{	// use text key (.txt) for text view, or the default for run
		shell::Execute( this, itemPair.second->m_path.GetPtr(), NULL, SEE_MASK_FLAG_DDEWAIT, NULL, NULL, cmdId == CM_TEXT_VIEW_FILE ? _T(".txt") : NULL );
	}
}

void CFileTreeDialog::CmCopyFullPath( void )
{
	TreeItemPair itemPair = GetSelectedItem();
	if ( itemPair.first != NULL && itemPair.second->m_path.FileExist() )
		CClipboard::CopyText( itemPair.second->m_path.Get() );
}

void CFileTreeDialog::CmLocateFile( void )
{
	if ( m_treeCtrl.IsInternalChange() )
		return;

	CFileLocatorDialog dialog( this );

	if ( !ui::IsKeyPressed( VK_SHIFT ) )
	{	// not SHIFT down: inherit current selection dir path
		if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
			if ( CIncludeNode* pItemInfo = GetItemInfo( hSelItem ) )
				dialog.SetLocalCurrentFile( pItemInfo->m_path.Get() );
	}
	if ( IDOK == dialog.DoModal() )
		SetRootPath( dialog.m_selectedFilePath );
}

void CFileTreeDialog::CmBrowseFile( void )
{
	if ( m_treeCtrl.IsInternalChange() )
		return;

	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
		if ( CIncludeNode* pItemInfo = GetItemInfo( hSelItem ) )
			SetRootPath( pItemInfo->m_path.Get() );
}

void CFileTreeDialog::UUI_AnySelected( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( AnySelected() );
	if ( pCmdUI->m_nID == IDOK && pCmdUI->m_pMenu != NULL )
		pCmdUI->m_pMenu->SetDefaultItem( IDOK );
}

void CFileTreeDialog::UUI_OpenIncludeLine( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !IsRootItem( m_treeCtrl.GetSelectedItem() ) );
}

void CFileTreeDialog::UUI_OpenComplementary( CCmdUI* pCmdUI )
{
	bool hasComplementary = HasValidComplementary();

	pCmdUI->Enable( hasComplementary );
	if ( pCmdUI->m_pMenu != NULL )
	{
		CString itemFormat;
		if ( pCmdUI->m_pMenu->GetMenuString( pCmdUI->m_nID, itemFormat, MF_BYCOMMAND ) > 0 )
		{
			std::tstring itemCore = _T("Complementary");

			if ( hasComplementary )
				itemCore = arg::Enquote( m_complemFilePath.GetFilename() );

			pCmdUI->SetText( str::Format( itemFormat, itemCore.c_str() ).c_str() );
		}
	}
}

void CFileTreeDialog::CmEditLabel( void )
{
	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
		m_treeCtrl.EditLabel( hSelItem );
}

void CFileTreeDialog::UUI_EditLabel( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( ::GetFocus() == m_treeCtrl.m_hWnd && m_treeCtrl.GetSelectedItem() != NULL );
}

void _appendItemTextLine( CTreeControl* pTreeCtrl, HTREEITEM hItem, std::tstring* pDestString, int nestingLevel )
{
	ASSERT_PTR( pDestString );

	std::tstring itemText = pTreeCtrl->GetItemText( hItem );
	if ( !itemText.empty() )		// not an unparsed sub-item placeholder
	{
		if ( nestingLevel > 0 )
			*pDestString += std::tstring( nestingLevel, _T('\t') );

		*pDestString += itemText + _T("\r\n");
	}
}

void CFileTreeDialog::CmCopyToClipboard( void )
{
	std::tstring allItemsText;
	ForEach( (IterFunc)_appendItemTextLine, TVI_ROOT, &allItemsText );
	CClipboard::CopyText( allItemsText );
}
