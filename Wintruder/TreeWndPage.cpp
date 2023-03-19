
#include "stdafx.h"
#include "TreeWndPage.h"
#include "Application.h"
#include "AppService.h"
#include "resource.h"
#include "wnd/WndImageRepository.h"
#include "wnd/WndUtils.h"
#include "utl/Logger.h"
#include "utl/Timer.h"
#include "utl/StringUtilities.h"
#include "utl/TextClipboard.h"
#include "utl/UI/AccelTable.h"
#include "utl/UI/Icon.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/SystemTray_fwd.h"
#include "utl/UI/WndUtilsEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace wt
{
	size_t AddItemText( std::tstring& rItemsText, const CTreeControl* pTreeCtrl, HTREEITEM hItem, unsigned int indent )
	{
		ASSERT_PTR( pTreeCtrl );
		ASSERT_PTR( hItem );
		std::tstring prefix( indent * 2, _T(' ') );
		stream::Tag( rItemsText, prefix + (const TCHAR*)pTreeCtrl->GetItemText( hItem ), _T("\r\n") );

		size_t itemCount = 1;
		for ( HTREEITEM hChild = pTreeCtrl->GetChildItem( hItem ); hChild != NULL; hChild = pTreeCtrl->GetNextItem( hChild, TVGN_NEXT ) )
			itemCount += AddItemText( rItemsText, pTreeCtrl, hChild, indent + 1 );
		return itemCount;
	}

	void LogWindow( HWND hWnd, int indent )
	{
		app::GetLogger()->Log( _T("[%d] %swnd: %s"), indent, std::tstring( indent * 2, _T(' ') ).c_str(), wnd::FormatBriefWndInfo( hWnd ).c_str() );
	}


	// CItemInfo implementation

	CItemInfo::CItemInfo( HWND hWnd, int image /*= -1*/ )
		: m_text( wnd::FormatBriefWndInfo( hWnd ) )
	{
		Construct();
		ASSERT( IsWindow( hWnd ) );

		if ( -1 == image )
			if ( HICON hIcon = wnd::GetWindowIcon( hWnd ) )
				image = Image_Transparent;
			else
				image = CWndImageRepository::Instance().LookupImage( hWnd );

		DWORD style = ui::GetStyle( hWnd );

		m_item.state = HasFlag( style, WS_DISABLED ) ? TVIS_CUT : 0;
		m_item.pszText = const_cast<TCHAR*>( m_text.c_str() );
		m_item.iImage = m_item.iSelectedImage = image;
		m_item.lParam = (LPARAM)hWnd;
	}

	CItemInfo::CItemInfo( const CTreeControl* pTreeCtrl, HTREEITEM hItem )
	{
		Construct();
		m_item.hItem = hItem;
		m_item.mask |= TVIF_HANDLE;

		if ( pTreeCtrl->GetItem( &m_item ) )
			if ( m_item.pszText != NULL )
				m_text = m_item.pszText;
	}

	void CItemInfo::Construct( void )
	{
		ZeroMemory( &m_item, sizeof( m_item ) );
		m_item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_PARAM;
		m_item.stateMask = TVIS_CUT;
	}


	// CWndTreeBuilder implementation

	CWndTreeBuilder::CWndTreeBuilder( CTreeControl* pTreeCtrl, CLogger* pLogger /*= NULL*/ )
		: m_pTreeCtrl( pTreeCtrl )
		, m_pLogger( pLogger )
	{
	}

	void CWndTreeBuilder::AddWndItem( HWND hWnd )
	{
		ASSERT( ::IsWindow( hWnd ) );

		HWND hParentWnd = wnd::GetRealParent( hWnd );

		InsertWndItem( hWnd, RegisterParentWnd( hParentWnd ) );
	}

	const TTreeItemIndent& CWndTreeBuilder::InsertWndItem( HWND hWnd, const TTreeItemIndent& parentPair, HTREEITEM hInsertAfter /*= TVI_LAST*/ )
	{
		ASSERT_PTR( hWnd );

		HTREEITEM hItemParent = parentPair.first;
		CItemInfo info( hWnd, TVI_ROOT == hItemParent ? Image_Desktop : -1 );
		TTreeItemIndent& rItemPair = m_wndToItemMap[ hWnd ];

		rItemPair.first = m_pTreeCtrl->InsertItem( info.m_item.mask, info.m_item.pszText,
												   info.m_item.iImage, info.m_item.iSelectedImage, info.m_item.state, info.m_item.stateMask,
												   info.m_item.lParam, hItemParent, hInsertAfter );
		rItemPair.second = parentPair.second + 1;

		if ( !m_insertedEffect.IsNull() )
			m_pTreeCtrl->MarkItem( rItemPair.first, m_insertedEffect );

		LogWnd( hWnd, rItemPair.second );
		return rItemPair;
	}

	const TTreeItemIndent& CWndTreeBuilder::MergeWndItem( HWND hWnd, const TTreeItemIndent& parentPair )
	{
		HTREEITEM hItem = m_pTreeCtrl->FindItemWithData( hWnd, parentPair.first, Shallow );

		if ( NULL == hItem )
			return InsertWndItem( hWnd, parentPair, FindInsertAfter( parentPair.first, hWnd ) );
		else
		{
			CItemInfo info( hWnd, TVI_ROOT == parentPair.first ? Image_Desktop : -1 );

			VERIFY(
				m_pTreeCtrl->SetItem( hItem, info.m_item.mask, info.m_item.pszText,
									  info.m_item.iImage, info.m_item.iSelectedImage, info.m_item.state, info.m_item.stateMask,
									  info.m_item.lParam )
			);

			return m_wndToItemMap[ hWnd ] = std::make_pair( hItem, parentPair.second + 1 );
		}
	}

	HTREEITEM CWndTreeBuilder::FindInsertAfter( HTREEITEM hParentItem, HWND hWnd ) const
	{
		if ( HWND hWndPrev = ::GetWindow( hWnd, GW_HWNDPREV ) )
			if ( HTREEITEM hPrevItem = m_pTreeCtrl->FindItemWithData( hWndPrev, hParentItem, Shallow ) )
				return hPrevItem;

		return TVI_LAST;
	}

	const TTreeItemIndent& CWndTreeBuilder::MapParentItem( HTREEITEM hParentItem )
	{
		ASSERT_PTR( hParentItem );

		HWND hWndParent = m_pTreeCtrl->GetItemDataAs<HWND>( hParentItem );
		ASSERT_PTR( hWndParent );

		TTreeItemIndent& rPair = m_wndToItemMap[ hWndParent ];

		rPair.first = hParentItem;
		rPair.second = m_pTreeCtrl->GetItemIndentLevel( hParentItem );
		return rPair;
	}

	const TTreeItemIndent* CWndTreeBuilder::FindWndItem( HWND hWnd ) const
	{
		std::unordered_map< HWND, TTreeItemIndent >::const_iterator itFound = m_wndToItemMap.find( hWnd );
		return itFound != m_wndToItemMap.end() ? &itFound->second : NULL;
	}

	const TTreeItemIndent& CWndTreeBuilder::RegisterParentWnd( HWND hWndParent )
	{
		if ( const TTreeItemIndent* pFoundPair = FindWndItem( hWndParent ) )
			return *pFoundPair;

		ENSURE( NULL == hWndParent );		// if a child window, should have already been registered by traversing from root via InsertWndItem()

		TTreeItemIndent& rPair = m_wndToItemMap[ hWndParent ];	// include NULL hWnd for TVI_ROOT, indent level of -1

		rPair.first = TVI_ROOT;
		rPair.second = m_pTreeCtrl->GetItemIndentLevel( TVI_ROOT );
		return rPair;
	}

	void CWndTreeBuilder::LogWnd( HWND hWnd, int indent ) const
	{
		if ( m_pLogger != NULL )
			wt::LogWindow( hWnd, indent );
	}

	template< typename IteratorT >
	HTREEITEM CWndTreeBuilder::MergeBranchPath( IteratorT itStartWnd, IteratorT itEndWnd )
	{
		REQUIRE( itStartWnd != itEndWnd );

		TTreeItemIndent parentPair( TVI_ROOT, -1 );

		if ( HWND hWndParent = wnd::GetRealParent( *itStartWnd ) )		// not starting at the destop window?
			parentPair = MapParentItem( m_pTreeCtrl->FindItemWithData( hWndParent ) );

		for ( ; itStartWnd != itEndWnd; ++itStartWnd )
			parentPair = MergeWndItem( *itStartWnd, parentPair );

		return parentPair.first;		// the deep item merged into the tree
	}
}


// CTreeWndPage implementation

namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_WINDOW_TREE, Size }
	};
}


CTreeWndPage::CTreeWndPage( void )
	: CLayoutPropertyPage( IDD_TREE_WND_PAGE )
	, m_destroying( false )
{
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );
	app::GetSvc().AddObserver( this );

	m_treeCtrl.SetTextEffectCallback( this );
	m_treeCtrl.StoreImageList( CWndImageRepository::Instance().GetImageList() );

	m_accelPool.AddAccelTable( new CAccelTable( IDD_TREE_WND_PAGE ) );

	ui::LoadPopupMenu( m_treeCtrl.GetContextMenu(), IDR_CONTEXT_MENU, app::TreePopup, ui::CheckedMenuImages );
	m_treeCtrl.GetContextMenu().SetDefaultItem( CM_HIGHLIGHT_WINDOW );
}

CTreeWndPage::~CTreeWndPage()
{
	app::GetSvc().RemoveObserver( this );
}

void CTreeWndPage::OnTargetWndChanged( const CWndSpot& targetWnd )
{
	if ( m_treeCtrl.IsInternalChange() )
		return;

	HTREEITEM hTargetItem = NULL;

	if ( !targetWnd.IsNull() )
	{
		hTargetItem = FindItemWithWnd( targetWnd );

		if ( NULL == hTargetItem || !m_treeCtrl.RefreshItem( hTargetItem ) )		// refresh item text, state
			ui::BeepSignal( MB_ICONWARNING );		// window not found or stale

		bool stale = hTargetItem != NULL && !ui::IsValidWindow( m_treeCtrl.GetItemDataAs<HWND>( hTargetItem ) );

		if ( NULL == hTargetItem || stale )
			if ( app::GetOptions()->m_autoUpdateRefresh )
			{
				app::GetSvc().PublishEvent( app::RefreshWndTree );		// content refresh and target selection
				hTargetItem = m_treeCtrl.GetSelectedItem();				// should be the currently selected
			}
			else
			{
				const bool slowWindows = wnd::HasSlowWindows(); slowWindows;
				hTargetItem = RefreshBranchOf( targetWnd.m_hWnd );
			}
	}

	if ( hTargetItem != m_treeCtrl.GetSelectedItem() )
	{
		CScopedInternalChange scopedChange( &m_treeCtrl );
		m_treeCtrl.SelectItem( hTargetItem );
	}
}

void CTreeWndPage::OnAppEvent( app::Event appEvent )
{
	switch ( appEvent )
	{
		case app::RefreshWndTree:
			RefreshTreeContents();
			break;
		case app::RefreshBranch:
			if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
				RefreshTreeBranch( hSelItem );
			break;
		case app::RefreshSiblings:
			if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
				RefreshTreeParentBranch( hSelItem );
			break;
	}
}

void CTreeWndPage::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	subItem, pCtrl;

	HTREEITEM hItem = reinterpret_cast<HTREEITEM>( rowKey );
	HWND hWnd = m_treeCtrl.GetItemDataAs<HWND>( hItem );

	if ( !ui::IsValidWindow( hWnd ) )
		rTextEffect.m_textColor = app::StaleWndColor;
	else
	{
		if ( !ui::IsVisible( hWnd ) )
			rTextEffect.m_fontEffect |= ui::Italic;

		if ( ui::IsDisabled( hWnd ) )
			rTextEffect.m_textColor = ::GetSysColor( COLOR_GRAYTEXT );

		if ( wnd::IsSlowWindow( hWnd ) )
			rTextEffect.m_textColor = ui::GetBlendedColor( rTextEffect.m_textColor != CLR_NONE ? rTextEffect.m_textColor : m_treeCtrl.GetActualTextColor(), app::SlowWndColor );
	}
}


void CTreeWndPage::RefreshTreeContents( void )
{
	CScopedTrayIconBalloon scopedBalloon( NULL, _T("Refreshing all windows, please wait."), NULL, app::Info, 30 );

	CScopedInternalChange scopedChange( &m_treeCtrl );
	CScopedLockRedraw freeze( &m_treeCtrl, new CScopedWindowBorder( &m_treeCtrl, color::Salmon ) );

	SetupTreeItems();		// insert all windows in the system starting from the root desktop window (all others windows are desktop's children)

	if ( HTREEITEM hRootItem = m_treeCtrl.GetRootItem() )
		m_treeCtrl.Expand( hRootItem, TVE_EXPAND );

	HTREEITEM hTargetItem = FindValidTargetItem();
	m_treeCtrl.SelectItem( hTargetItem );
}

bool CTreeWndPage::RefreshTreeItem( HTREEITEM hItem )
{	// updates item contents if changed
	ASSERT_PTR( hItem );

	HWND hWnd = m_treeCtrl.GetItemDataAs<HWND>( hItem );
	if ( !ui::IsValidWindow( hWnd ) )
		return false;

	wt::CItemInfo oldInfo( &m_treeCtrl, hItem );
	wt::CItemInfo newInfo( hWnd );

	if ( newInfo != oldInfo )
	{
		CScopedInternalChange scopedChange( &m_treeCtrl );
		newInfo.m_item.hItem = hItem;
		m_treeCtrl.SetItem( &newInfo.m_item );			// refresh item
	}
	return true;
}

void CTreeWndPage::RefreshTreeBranch( HTREEITEM hItem )
{
	ASSERT_PTR( hItem );

	if ( !RefreshTreeItem( hItem ) )
	{
		m_treeCtrl.DeleteItem( hItem );
		return;
	}

	HWND hWndTarget = m_treeCtrl.GetItemDataAs<HWND>( hItem );

	{
		CScopedTrayIconBalloon scopedBalloon( NULL, _T("Refreshing missing windows."), NULL, app::Warning, 20 );
		CScopedInternalChange scopedChange( &m_treeCtrl );
		CScopedLockRedraw freeze( &m_treeCtrl, new CScopedWindowBorder( &m_treeCtrl, color::Salmon ) );
		CWaitCursor wait;

		m_treeCtrl.DeleteChildren( hItem );

		wt::CWndTreeBuilder builder( &m_treeCtrl, NULL );

		builder.MapParentItem( hItem );
		builder.BuildChildren( hWndTarget );

		m_treeCtrl.Expand( hItem, TVE_EXPAND );
		m_treeCtrl.SelectItem( hItem );
	}

	m_treeCtrl.EnsureVisible( hItem );
}

void CTreeWndPage::RefreshTreeParentBranch( HTREEITEM hItem )
{
	ASSERT_PTR( hItem );

	HWND hWndTarget = m_treeCtrl.GetItemDataAs<HWND>( hItem );
	HTREEITEM hParentItem = m_treeCtrl.GetParentItem( hItem );
	if ( NULL == hParentItem )
		return;

	{
		CScopedInternalChange scopedChange( &m_treeCtrl );
		CScopedLockRedraw freeze( &m_treeCtrl, new CScopedWindowBorder( &m_treeCtrl, color::Salmon ) );
		CWaitCursor wait;
		HWND hWndParent = m_treeCtrl.GetItemDataAs<HWND>( hParentItem );

		m_treeCtrl.DeleteChildren( hParentItem );

		wt::CWndTreeBuilder builder( &m_treeCtrl, NULL );
		int indent = builder.MapParentItem( hParentItem ).second; indent;

		//app::GetLogger()->Log( _T("# Refresh siblings of:") ); wt::LogWindow( hWndTarget, indent );

		builder.BuildChildren( hWndParent );

		m_treeCtrl.Expand( hParentItem, TVE_EXPAND );

		hItem = m_treeCtrl.FindItemWithData( hWndTarget );
		if ( hItem != NULL )
			m_treeCtrl.SelectItem( hItem );
	}

	if ( hItem != NULL )
		m_treeCtrl.EnsureVisible( hItem );
}

HTREEITEM CTreeWndPage::RefreshBranchOf( HWND hWndTarget )	// refresh branch under desktop window down to the hWndTarget item
{
	std::vector<HWND> branchPath;
	wnd::QueryAncestorBranchPath( branchPath, hWndTarget, ::GetDesktopWindow() );		// bottom-up ancestor path, excluding hWndEnd

	HTREEITEM hTargetItem = NULL;

	if ( !branchPath.empty() )
	{
		CScopedInternalChange scopedChange( &m_treeCtrl );
		CScopedLockRedraw freeze( &m_treeCtrl, new CScopedWindowBorder( &m_treeCtrl, color::Salmon ) );
		CWaitCursor wait;

		app::GetLogger()->Log( _T("# Refresh missing window branch of:") );

		wt::CWndTreeBuilder builder( &m_treeCtrl, app::GetLogger() );

		builder.SetInsertedEffect( ui::CTextEffect::MakeColor( app::MergeInsertWndColor ) );

		HTREEITEM hParentItem = builder.MergeBranchPath( branchPath.rbegin(), branchPath.rend() - 1 );	// step 1: merge excluding the deepest item
		builder.BuildChildren( branchPath[ 1 ] );														// step 2: refresh entirely the parent of deepest item, so that all siblings are inserted
		hTargetItem = m_treeCtrl.FindItemWithData( branchPath[ 0 ], hParentItem, Shallow );
		ENSURE( hTargetItem != NULL );

		m_treeCtrl.Expand( m_treeCtrl.GetParentItem( hTargetItem ), TVE_EXPAND );		// expand parent so target item is in sight
		m_treeCtrl.SelectItem( hTargetItem );
	}

	if ( hTargetItem != NULL )
		m_treeCtrl.EnsureVisible( hTargetItem );

	return hTargetItem;
}


void CTreeWndPage::SetupTreeItems( void )
{
	m_treeCtrl.DeleteAllItems();

	enum BuildMethod { EnumWindows, LoopWindowsOld } buildMethod = EnumWindows;
	CTimer timer;
	CWaitCursor wait;

	if ( EnumWindows == buildMethod )
	{	// enum windows
		app::GetLogger()->Log( _T("# Enum All Windows") );
		wt::CWndTreeBuilder builder( &m_treeCtrl, NULL /*app::GetLogger()*/ );

		builder.Build( ::GetDesktopWindow() );
	}
	else
	{	// loop windows
		app::GetLogger()->Log( _T("# Loop Windows (OLD)") );
		InsertWndItem_Old( TVI_ROOT, ::GetDesktopWindow(), 0, Image_Desktop );
	}

	app::GetLogger()->Log( _T("Elapsed %.3f seconds (found %d windows)"), timer.ElapsedSeconds(), m_treeCtrl.GetCount() );
}

HTREEITEM CTreeWndPage::FindItemWithWnd( HWND hWnd ) const
{
	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )		// is currently selected a match?
		if ( hWnd == m_treeCtrl.GetItemDataAs<HWND>( hSelItem ) )
			return hSelItem;

	return m_treeCtrl.FindItemWithData( hWnd );
}

HWND CTreeWndPage::FindValidParentItem( HTREEITEM hItem ) const
{
	HWND hValidWnd = NULL;

	// find first valid parent window if hItem refers to a stale window
	for ( ; hItem != NULL; hItem = m_treeCtrl.GetParentItem( hItem ) )
		if ( ui::IsValidWindow( hValidWnd = m_treeCtrl.GetItemDataAs<HWND>( hItem ) ) )
			break;

	return hValidWnd;
}

HTREEITEM CTreeWndPage::FindValidTargetItem( void ) const
{
	if ( CWndSpot* pTargetWnd = app::GetValidTargetWnd() )
		return m_treeCtrl.FindItemWithData( pTargetWnd->m_hWnd );

	return NULL;
}

HTREEITEM CTreeWndPage::InsertWndItem_Old( HTREEITEM hItemParent, HWND hWnd, int indent, int image /*= -1*/ )
{
//	wt::LogWindow( hWnd, indent );

	wt::CItemInfo info( hWnd, image );
	HTREEITEM hItem = m_treeCtrl.InsertItem( info.m_item.mask, info.m_item.pszText,
											 info.m_item.iImage, info.m_item.iSelectedImage, info.m_item.state, info.m_item.stateMask,
											 info.m_item.lParam, hItemParent, TVI_LAST );

	for ( HWND hChildWnd = ::GetWindow( hWnd, GW_CHILD ); hChildWnd != NULL; hChildWnd = ::GetWindow( hChildWnd, GW_HWNDNEXT ) )
		InsertWndItem_Old( hItem, hChildWnd, indent + 1 );

	return hItem;
}

void CTreeWndPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_treeCtrl.m_hWnd;
	DDX_Control( pDX, IDC_WINDOW_TREE, m_treeCtrl );

	if ( firstInit )
		RefreshTreeContents();

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		OutputTargetWnd();

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CTreeWndPage, CLayoutPropertyPage )
	ON_WM_DESTROY()
	ON_NOTIFY( TVN_SELCHANGED, IDC_WINDOW_TREE, OnTvnSelChanged_WndTree )
	ON_NOTIFY( NM_SETFOCUS, IDC_WINDOW_TREE, OnTvnSerFocus_WndTree )
	ON_NOTIFY( NM_CUSTOMDRAW, IDC_WINDOW_TREE, OnTvnCustomDraw_WndTree )
	ON_NOTIFY( tv::TVN_REFRESHITEM, IDC_WINDOW_TREE, OnTcnRefreshItem_WndTree )
	ON_COMMAND( CM_EXPAND_BRANCH, CmExpandBranch )
	ON_COMMAND( CM_COLAPSE_BRANCH, CmColapseBranch )
	ON_COMMAND( ID_COPY_TREE_ANCESTORS, OnCopyTreeAncestors )
	ON_COMMAND( ID_COPY_TREE_CHILDREN, OnCopyTreeChildren )
	ON_UPDATE_COMMAND_UI_RANGE( ID_COPY_TREE_ANCESTORS, ID_COPY_TREE_CHILDREN, OnUpdateCopy )
END_MESSAGE_MAP()

BOOL CTreeWndPage::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		GetParentOwner()->OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CTreeWndPage::OnDestroy( void )
{
	m_destroying = true;		// speed up destruction caused by CDDS_ITEMPOSTPAINT for slow windows
	__super::OnDestroy();
}

void CTreeWndPage::OnTvnSelChanged_WndTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMTREEVIEW* pTreeInfo = (NMTREEVIEW*)pNmHdr;
	*pResult = 0;

	ASSERT( !m_treeCtrl.IsInternalChange() );
	HWND hSelWnd = (HWND)pTreeInfo->itemNew.lParam;
	app::GetSvc().SetTargetWnd( hSelWnd );
}

void CTreeWndPage::OnTvnSerFocus_WndTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	pNmHdr;
	*pResult = 0;
	m_treeCtrl.Invalidate();		// redraw stale windows
}

void CTreeWndPage::OnTvnCustomDraw_WndTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMTVCUSTOMDRAW* pDraw = (NMTVCUSTOMDRAW*)pNmHdr;
	HWND hWnd = reinterpret_cast<HWND>( pDraw->nmcd.lItemlParam );

	*pResult = CDRF_DODEFAULT;
	switch ( pDraw->nmcd.dwDrawStage )
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			*pResult |= CDRF_NOTIFYPOSTPAINT;
			break;
		case CDDS_ITEMPOSTPAINT:
			if ( !m_destroying )											// speed up tree destruction for slow windows in Windows 10 due to UIPI
				if ( ui::IsValidWindow( hWnd ) )
					if ( HICON hIcon = wnd::GetWindowIcon( hWnd ) )
						m_treeCtrl.CustomDrawItemIcon( pDraw, hIcon );		// works with transparent item image
			break;
	}
}

void CTreeWndPage::OnTcnRefreshItem_WndTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	tv::CNmTreeItem* pTreeItemInfo = (tv::CNmTreeItem*)pNmHdr;
	*pResult = RefreshTreeItem( pTreeItemInfo->m_hItem ) ? 0 : 1;
}

void CTreeWndPage::CmExpandBranch( void )
{
	if ( HTREEITEM hItemSel = m_treeCtrl.GetSelectedItem() )
		m_treeCtrl.ExpandBranch( hItemSel );
}

void CTreeWndPage::CmColapseBranch( void )
{
	if ( HTREEITEM hItemSel = m_treeCtrl.GetSelectedItem() )
		m_treeCtrl.ExpandBranch( hItemSel, false );
}

void CTreeWndPage::OnCopyTreeAncestors( void )
{
	std::deque< HTREEITEM > ancestors;
	for ( HTREEITEM hItem = m_treeCtrl.GetSelectedItem(); hItem != NULL; hItem = m_treeCtrl.GetParentItem( hItem ) )
		ancestors.push_front( hItem );

	std::tstring itemsText; itemsText.reserve( ancestors.size() * 128 );
	for ( size_t i = 0; i != ancestors.size(); ++i )
	{
		std::tstring prefix( i * 2, _T(' ') );
		stream::Tag( itemsText, prefix + (const TCHAR*)m_treeCtrl.GetItemText( ancestors[ i ] ), _T("\r\n") );
	}
	if ( ancestors.size() > 1 )					// multi-line
		itemsText += _T("\r\n");

	CTextClipboard::CopyText( itemsText, m_hWnd );
}

void CTreeWndPage::OnCopyTreeChildren( void )
{
	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
	{
		std::tstring itemsText;
		size_t itemCount = wt::AddItemText( itemsText, &m_treeCtrl, hSelItem, 0 );

		if ( itemCount > 1 )					// multi-line
			itemsText += _T("\r\n");

		CTextClipboard::CopyText( itemsText, m_hWnd );
	}
}

void CTreeWndPage::OnUpdateCopy( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_treeCtrl.GetSelectedItem() != NULL );
}
