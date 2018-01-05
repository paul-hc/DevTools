
#include "stdafx.h"
#include "TreeControl.h"
#include "Icon.h"
#include "UtilitiesEx.h"
#include "VisualTheme.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTreeControl::CTreeControl( void )
	: CTreeCtrl()
	, m_imageSize( 0, 0 )
{
}

CTreeControl::~CTreeControl()
{
}

bool CTreeControl::IsRealItem( HTREEITEM hItem ) const
{
	ASSERT_PTR( hItem );
	switch ( reinterpret_cast< size_t >( hItem ) )
	{
		case TVI_ROOT:
		case TVI_FIRST:
		case TVI_LAST:
		case TVI_SORT:
			return false;
	}
	return true;
}

bool CTreeControl::IsExpandLazyChildren( const NMTREEVIEW* pNmTreeView ) const
{
	ASSERT_PTR( pNmTreeView );
	return ( HasFlag( pNmTreeView->action, TVE_EXPAND ) && !HasFlag( pNmTreeView->itemNew.state, TVIS_EXPANDEDONCE ) ) ||
		   ( HasFlag( pNmTreeView->action, TVE_TOGGLE ) && !HasFlag( pNmTreeView->itemNew.state, TVIS_EXPANDED ) );
}

BOOL CTreeControl::EnsureVisible( HTREEITEM hItem )
{
	BOOL result = CTreeCtrl::EnsureVisible( hItem );

	CRect itemImageRect;
	if ( GetItemImageRect( itemImageRect, hItem ) )
	{
		CRect clientRect;
		GetClientRect( &clientRect );
		if ( itemImageRect.left < clientRect.left )		// item icon not visible
		{	// make the image and leading part visible
			int horizPos = GetScrollPos( SB_HORZ );
			horizPos -= ( clientRect.left - itemImageRect.left + GetIndent() );
			horizPos = std::max< int >( horizPos, 0 );
			SendMessage( WM_HSCROLL, MAKEWPARAM( SB_THUMBPOSITION, horizPos ), 0 );
		}
	}
	return result;
}

bool CTreeControl::SelectItem( HTREEITEM hItem )
{
	if ( !CTreeCtrl::SelectItem( hItem ) )
		return false;

	if ( hItem != NULL )
		EnsureVisible( hItem );
	return true;
}

bool CTreeControl::RefreshItem( HTREEITEM hItem )
{
	ASSERT_PTR( hItem );
	NMTREEITEM treeItemInfo;
	treeItemInfo.hItem = hItem;
	// notify parent to refresh the item
	return 0 == ui::SendNotifyToParent( m_hWnd, TCN_REFRESHITEM, &treeItemInfo.hdr );		// false if an error occured
}

void CTreeControl::ExpandBranch( HTREEITEM hItem, bool expand /*= true*/ )
{
	{
		CScopedLockRedraw freeze( this );
		ForEach( func::ExpandItem( expand ? TVE_EXPAND : TVE_COLLAPSE ), hItem );
	}
	EnsureVisible( hItem );
}

const CSize& CTreeControl::GetImageSize( void ) const
{
	if ( 0 == m_imageSize.cx && 0 == m_imageSize.cy )
		if ( CImageList* pImageList = GetImageList( TVSIL_NORMAL ) )
			m_imageSize = gdi::GetImageSize( *pImageList );

	return m_imageSize;
}

bool CTreeControl::GetItemImageRect( CRect& rItemImageRect, HTREEITEM hItem ) const
{
	ASSERT_PTR( hItem );
	CRect itemTextRect;
	if ( !GetItemRect( hItem, &itemTextRect, TRUE ) )
		return false;			// item is not visible

	rItemImageRect.SetRect( 0, 0, GetImageSize().cx, GetImageSize().cy );
	ui::AlignRect( rItemImageRect, itemTextRect, H_AlignLeft | V_AlignCenter );

	enum { IconTextSpacing = 3 };
	rItemImageRect.OffsetRect( -( IconTextSpacing + rItemImageRect.Width() ), 0 );
	return true;
}

bool CTreeControl::CustomDrawItemIcon( const NMTVCUSTOMDRAW* pDraw, HICON hIcon, int diFlags /*= DI_NORMAL | DI_COMPAT*/ )
{
	ASSERT_PTR( pDraw );
	ASSERT_PTR( hIcon );

	HTREEITEM hItem = (HTREEITEM)pDraw->nmcd.dwItemSpec;
	CRect itemImageRect;
	if ( !GetItemImageRect( itemImageRect, hItem ) )
		return false;

	::DrawIconEx( pDraw->nmcd.hdc, itemImageRect.left, itemImageRect.top, hIcon, itemImageRect.Width(), itemImageRect.Height(), 0, NULL, diFlags );
	return true;
}

void CTreeControl::PreSubclassWindow( void )
{
	CTreeCtrl::PreSubclassWindow();

	CVisualTheme::SetWindowTheme( m_hWnd, L"Explorer", NULL );		// enable Explorer theme
}


// message handlers

BEGIN_MESSAGE_MAP( CTreeControl, CTreeCtrl )
	ON_WM_NCLBUTTONDOWN()
	ON_NOTIFY_REFLECT_EX( TVN_SELCHANGED, OnTvnSelChanged_Reflect )
	ON_NOTIFY_REFLECT_EX( NM_RCLICK, OnTvnRClick_Reflect )
	ON_NOTIFY_REFLECT_EX( NM_DBLCLK, OnTvnDblClk_Reflect )
END_MESSAGE_MAP()

void CTreeControl::OnNcLButtonDown( UINT hitTest, CPoint point )
{
	ui::TakeFocus( m_hWnd );
	CTreeCtrl::OnNcLButtonDown( hitTest, point );
}

BOOL CTreeControl::OnTvnSelChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMTREEVIEW* pTreeInfo = (NMTREEVIEW*)pNmHdr;
	pTreeInfo, pResult;
	return IsInternalChange();		// don't raise the notification to list's parent during an internal change
}

BOOL CTreeControl::OnTvnRClick_Reflect( NMHDR* /*pNmHdr*/, LRESULT* pResult )
{
	if ( HTREEITEM hHitItem = GetDropHilightItem() )		// item right clicked (temporary highlighted)
		if ( RefreshItem( hHitItem ) )
			if ( hHitItem != GetSelectedItem() )
				CTreeCtrl::SelectItem( hHitItem );

	*pResult = 0;			// will send WM_CONTEXTMENU
	return 0;				// continue routing
}

BOOL CTreeControl::OnTvnDblClk_Reflect( NMHDR* /*pNmHdr*/, LRESULT* pResult )
{
	if ( HTREEITEM hSelItem = GetSelectedItem() )
		if ( m_contextMenu.GetSafeHmenu() != NULL )
			if ( UINT defCmdId = ::GetMenuDefaultItem( m_contextMenu, FALSE, 0 ) )
				ui::SendCommand( ::GetParent( m_hWnd ), defCmdId );

	*pResult = 0;
	return 0;				// continue routing
}
