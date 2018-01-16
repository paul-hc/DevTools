
#include "stdafx.h"
#include "TreeWndPage.h"
#include "Application.h"
#include "AppService.h"
#include "resource.h"
#include "wnd/WndImageRepository.h"
#include "wnd/WndUtils.h"
#include "utl/AccelTable.h"
#include "utl/Clipboard.h"
#include "utl/Icon.h"
#include "utl/Logger.h"
#include "utl/MenuUtilities.h"
#include "utl/Timer.h"
#include "utl/StringUtilities.h"
#include "utl/UtilitiesEx.h"

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
		m_item.pszText = const_cast< TCHAR* >( m_text.c_str() );
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

		HWND hParentWnd = ::GetAncestor( hWnd, GA_PARENT );			// real parent, NULL if desktop - GetParent( hWnd ) may return the owner
		const std::pair< HTREEITEM, int >* pParentPair = NULL;
		if ( hParentWnd != NULL )
			pParentPair = FindWndItem( hParentWnd );

		CItemInfo info( hWnd, NULL == hParentWnd ? Image_Desktop : -1 );
		HTREEITEM hItemParent = pParentPair != NULL ? pParentPair->first : TVI_ROOT;

		std::pair< HTREEITEM, int >& rPair = m_wndToItemMap[ hWnd ];
		rPair.first = m_pTreeCtrl->InsertItem( info.m_item.mask, info.m_item.pszText,
											   info.m_item.iImage, info.m_item.iSelectedImage, info.m_item.state, info.m_item.stateMask,
											   info.m_item.lParam, hItemParent, TVI_LAST );
		rPair.second = pParentPair != NULL ? ( pParentPair->second + 1 ) : 0;

		LogWnd( hWnd, rPair.second );
	}

	const std::pair< HTREEITEM, int >* CWndTreeBuilder::FindWndItem( HWND hWnd ) const
	{
		stdext::hash_map< HWND, std::pair< HTREEITEM, int > >::const_iterator itFound = m_wndToItemMap.find( hWnd );
		return itFound != m_wndToItemMap.end() ? &itFound->second : NULL;
	}

	void CWndTreeBuilder::LogWnd( HWND hWnd, int indent ) const
	{
		if ( m_pLogger != NULL )
			app::GetLogger().Log( _T("%swnd: %s"), std::tstring( indent * 2, _T(' ') ).c_str(), wnd::FormatBriefWndInfo( hWnd ).c_str() );
	}
}


// CTreeWndPage implementation

namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_WINDOW_TREE, Stretch }
	};
}


CTreeWndPage::CTreeWndPage( void )
	: CLayoutPropertyPage( IDD_TREE_WND_PAGE )
	, m_destroying( false )
{
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	app::GetSvc().AddObserver( this );

	m_accelPool.AddAccelTable( new CAccelTable( IDD_TREE_WND_PAGE ) );

	ui::LoadPopupMenu( m_treeCtrl.GetContextMenu(), IDR_CONTEXT_MENU, app::TreePopup, ui::CheckedMenuImages );
	::SetMenuDefaultItem( m_treeCtrl.GetContextMenu(), CM_HIGHLIGHT_WINDOW, FALSE );
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

		bool stale = hTargetItem != NULL && !ui::IsValidWindow( m_treeCtrl.GetItemDataAs< HWND >( hTargetItem ) );

		if ( ( NULL == hTargetItem && !wnd::HasSlowWindows() ) ||
			 ( stale && app::GetOptions()->m_autoUpdateRefresh ) )
		{
			app::GetSvc().PublishEvent( app::RefreshWndTree );		// content refresh and target selection
			hTargetItem = m_treeCtrl.GetSelectedItem();				// should be the currently selected
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
	}
}

void CTreeWndPage::RefreshTreeContents( void )
{
	CScopedInternalChange scopedChange( &m_treeCtrl );
	CScopedLockRedraw freeze( &m_treeCtrl, new CScopedWindowBorder( &m_treeCtrl, color::Salmon ) );

	m_treeCtrl.DeleteAllItems();
	SetupTreeItems();		// insert all windows in the system starting from the root desktop window (all others windows are desktop's children)

	if ( HTREEITEM hRootItem = m_treeCtrl.GetRootItem() )
		m_treeCtrl.Expand( hRootItem, TVE_EXPAND );

	HTREEITEM hTargetItem = NULL;
	if ( CWndSpot* pTargetWnd = app::GetValidTargetWnd() )
		hTargetItem = m_treeCtrl.FindItemWithData( pTargetWnd->m_hWnd );

	m_treeCtrl.SelectItem( hTargetItem );
}

void CTreeWndPage::SetupTreeItems( void )
{
	enum BuildMethod { EnumWindows, LoopWindowsOld } buildMethod = EnumWindows;
	CTimer timer;
	CWaitCursor wait;

	if ( EnumWindows == buildMethod )
	{	// enum windows
		app::GetLogger().Log( _T("# Enum Windows") );
		wt::CWndTreeBuilder builder( &m_treeCtrl );
		builder.Build( ::GetDesktopWindow() );
	}
	else
	{	// loop windows
		app::GetLogger().Log( _T("# Loop Windows (OLD)") );
		InsertWndItem( TVI_ROOT, ::GetDesktopWindow(), 0, Image_Desktop );
	}

	app::GetLogger().Log( _T("Elapsed %.3f seconds (found %d windows)"), timer.ElapsedSeconds(), m_treeCtrl.GetCount() );
}

HTREEITEM CTreeWndPage::FindItemWithWnd( HWND hWnd ) const
{
	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )		// is currently selected a match?
		if ( hWnd == m_treeCtrl.GetItemDataAs< HWND >( hSelItem ) )
			return hSelItem;

	return m_treeCtrl.FindItemWithData( hWnd );
}

HWND CTreeWndPage::FindValidParentItem( HTREEITEM hItem ) const
{
	HWND hValidWnd = NULL;

	// find first valid parent window if hItem refers to a stale window
	for ( ; hItem != NULL; hItem = m_treeCtrl.GetParentItem( hItem ) )
		if ( ui::IsValidWindow( hValidWnd = m_treeCtrl.GetItemDataAs< HWND >( hItem ) ) )
			break;

	return hValidWnd;
}

HTREEITEM CTreeWndPage::InsertWndItem( HTREEITEM hItemParent, HWND hWnd, int indent, int image /*= -1*/ )
{
//	app::GetLogger().Log( _T("%swnd: %s"), std::tstring( indent * 2, _T(' ') ).c_str(), wnd::FormatBriefWndInfo( hWnd ).c_str() );

	wt::CItemInfo info( hWnd, image );
	HTREEITEM hItem = m_treeCtrl.InsertItem( info.m_item.mask, info.m_item.pszText,
											 info.m_item.iImage, info.m_item.iSelectedImage, info.m_item.state, info.m_item.stateMask,
											 info.m_item.lParam, hItemParent, TVI_LAST );

	for ( HWND hChildWnd = ::GetWindow( hWnd, GW_CHILD ); hChildWnd != NULL; hChildWnd = ::GetWindow( hChildWnd, GW_HWNDNEXT ) )
		InsertWndItem( hItem, hChildWnd, indent + 1 );

	return hItem;
}

bool CTreeWndPage::RefreshTreeItem( HTREEITEM hItem )
{	// updates item contents if changed
	ASSERT_PTR( hItem );

	HWND hWnd = m_treeCtrl.GetItemDataAs< HWND >( hItem );
	if ( !ui::IsValidWindow( hWnd ) )
		return false;

	wt::CItemInfo oldInfo( &m_treeCtrl, hItem );
	wt::CItemInfo newInfo( hWnd );

	if ( newInfo != oldInfo )
	{
		CScopedInternalChange scopedChange( &m_treeCtrl );
		newInfo.m_item.hItem = hItem;
		m_treeCtrl.SetItem( &newInfo.m_item );				// refresh item
	}
	return true;
}

void CTreeWndPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_treeCtrl.m_hWnd;
	DDX_Control( pDX, IDC_WINDOW_TREE, m_treeCtrl );

	if ( firstInit )
	{
		CImageList* pImageList = CWndImageRepository::Instance().GetImageList();
		m_treeCtrl.SetImageList( pImageList, TVSIL_NORMAL );
		m_treeCtrl.SetImageList( pImageList, TVSIL_STATE );
		ui::MakeStandardControlFont( m_italicFont, ui::CFontInfo( false, true ) );

		RefreshTreeContents();				// first init
	}

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		OutputTargetWnd();

	CLayoutPropertyPage::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CTreeWndPage, CLayoutPropertyPage )
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY( TVN_SELCHANGED, IDC_WINDOW_TREE, OnTvnSelChanged_WndTree )
	ON_NOTIFY( NM_SETFOCUS, IDC_WINDOW_TREE, OnTvnSerFocus_WndTree )
	ON_NOTIFY( NM_CUSTOMDRAW, IDC_WINDOW_TREE, OnTvnCustomDraw_WndTree )
	ON_NOTIFY( CTreeControl::TCN_REFRESHITEM, IDC_WINDOW_TREE, OnTcnRefreshItem_WndTree )
	ON_COMMAND( CM_EXPAND_BRANCH, CmExpandBranch )
	ON_COMMAND( CM_COLAPSE_BRANCH, CmColapseBranch )
	ON_COMMAND( ID_COPY_TREE_ANCESTORS, OnCopyTreeAncestors )
	ON_COMMAND( ID_COPY_TREE_CHILDREN, OnCopyTreeChildren )
	ON_UPDATE_COMMAND_UI_RANGE( ID_COPY_TREE_ANCESTORS, ID_COPY_TREE_CHILDREN, OnUpdateCopy )
END_MESSAGE_MAP()

BOOL CTreeWndPage::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		CLayoutPropertyPage::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		GetParentOwner()->OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CTreeWndPage::OnDestroy( void )
{
	m_destroying = true;		// speed up destruction caused by CDDS_ITEMPOSTPAINT for slow windows
	CLayoutPropertyPage::OnDestroy();
}

void CTreeWndPage::OnContextMenu( CWnd* pWnd, CPoint point )
{
	if ( &m_treeCtrl == pWnd )
		if ( m_treeCtrl.GetContextMenu().GetSafeHmenu() != NULL )
			m_treeCtrl.GetContextMenu().TrackPopupMenu( TPM_RIGHTBUTTON, point.x, point.y, this );
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
	HWND hWnd = reinterpret_cast< HWND >( pDraw->nmcd.lItemlParam );

	*pResult = CDRF_DODEFAULT;
	switch ( pDraw->nmcd.dwDrawStage )
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			if ( !ui::IsValidWindow( hWnd ) )
			{
				pDraw->clrText = StaleWndColor;
				*pResult = CDRF_NEWFONT;
			}
			else
			{
				if ( !ui::IsVisible( hWnd ) )
				{
					::SelectObject( pDraw->nmcd.hdc, m_italicFont );
					*pResult = CDRF_NEWFONT;
				}
				if ( ui::IsDisabled( hWnd ) )
				{
					pDraw->clrText = color::Grey60;
					*pResult = CDRF_NEWFONT;
				}
				if ( wnd::IsSlowWindow( hWnd ) )
				{
					pDraw->clrText = SlowWndColor;
					*pResult = CDRF_NEWFONT;
				}
			}
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
	CTreeControl::NMTREEITEM* pTreeItemInfo = (CTreeControl::NMTREEITEM*)pNmHdr;
	*pResult = RefreshTreeItem( pTreeItemInfo->hItem ) ? 0 : 1;
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

	CClipboard::CopyText( itemsText, this );
}

void CTreeWndPage::OnCopyTreeChildren( void )
{
	if ( HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem() )
	{
		std::tstring itemsText;
		size_t itemCount = wt::AddItemText( itemsText, &m_treeCtrl, hSelItem, 0 );

		if ( itemCount > 1 )					// multi-line
			itemsText += _T("\r\n");

		CClipboard::CopyText( itemsText, this );
	}
}

void CTreeWndPage::OnUpdateCopy( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_treeCtrl.GetSelectedItem() != NULL );
}
