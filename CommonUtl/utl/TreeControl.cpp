
#include "stdafx.h"
#include "TreeControl.h"
#include "TreeControlCustomDraw.h"
#include "Clipboard.h"
#include "CustomDrawImager.h"
#include "Icon.h"
#include "ContainerUtilities.h"
#include "MenuUtilities.h"
#include "UtilitiesEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace tv
{
	bool IsTooltipDraw( const NMTVCUSTOMDRAW* pDraw )
	{
		ASSERT_PTR( pDraw );
		static const CRect s_emptyRect( 0, 0, 0, 0 );
		if ( s_emptyRect == pDraw->nmcd.rc )
			return true;						// tooltip custom draw
		return false;
	}
}


CTreeControl::CTreeControl( void )
	: CTreeCtrl()
	, CListLikeCtrlBase( this )
	, m_pImageList( NULL )
	, m_indentNoImages( 0 )
	, m_indentWithImages( 0 )
	, m_imageSize( 0, 0 )
{
}

CTreeControl::~CTreeControl()
{
}

void CTreeControl::ClearData( void )
{
	m_markedItems.clear();
}

bool CTreeControl::DeleteAllItems( void )
{
	ClearData();
	return __super::DeleteAllItems() != FALSE;
}

bool CTreeControl::Copy( void )
{
	HTREEITEM hSelItem = GetSelectedItem();
	return
		hSelItem != NULL &&
		CClipboard::CopyText( FormatCode( GetItemObject< utl::ISubject >( hSelItem ) ), this );
}

void CTreeControl::StoreImageList( CImageList* pImageList )
{
	m_pImageList = pImageList;

	if ( m_hWnd != NULL )
	{
		SetImageList( m_pImageList, TVSIL_NORMAL );

		if ( UINT indent = m_pImageList != NULL ? m_indentWithImages : m_indentNoImages )
			SetIndent( indent );

		UpdateCustomImagerBoundsSize();
		ui::RecalculateScrollbars( m_hWnd );
	}
}

CImageList* CTreeControl::SetImageList( CImageList* pImageList, int imageType )
{
	CImageList* pOldImageList = __super::SetImageList( pImageList, imageType );

	if ( pImageList != NULL && TVSIL_NORMAL == imageType )
		if ( 0 == m_indentWithImages )
			m_indentWithImages = GetIndent();

	return pOldImageList;
}

void CTreeControl::SetCustomFileGlyphDraw( bool showGlyphs /*= true*/ )
{
	CListLikeCtrlBase::SetCustomFileGlyphDraw( showGlyphs );

	if ( showGlyphs )
		StoreImageList( m_pCustomImager->GetImageList( ui::SmallGlyph ) );
	else
		StoreImageList( NULL );
}

void CTreeControl::SetCustomImageDraw( ui::ICustomImageDraw* pCustomImageDraw, const CSize& imageSize /*= CSize( 0, 0 )*/ )
{
	if ( pCustomImageDraw != NULL )
	{
		m_pCustomImager.reset( new CSingleCustomDrawImager( pCustomImageDraw, imageSize, imageSize ) );
		StoreImageList( m_pCustomImager->GetImageList( ui::SmallGlyph ) );
	}
	else
	{
		m_pCustomImager.reset();
		StoreImageList( NULL );
	}
}

HTREEITEM CTreeControl::InsertObjectItem( HTREEITEM hParent, const utl::ISubject* pObject, int imageIndex /*= ui::No_Image*/, UINT state /*= TVIS_EXPANDED*/,
										  HTREEITEM hInsertAfter /*= TVI_LAST*/, const TCHAR* pText /*= NULL*/ )
{
	std::tstring displayCode;
	if ( pObject != NULL )
		if ( NULL == pText )
		{
			displayCode = FormatCode( pObject );
			pText = displayCode.c_str();
		}

	if ( ui::Transparent_Image == imageIndex )
		imageIndex = m_pCustomImager.get() != NULL ? m_pCustomImager->GetTranspImageIndex() : 0;

	UINT mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
	if ( imageIndex != ui::No_Image )
		SetFlag( mask, TVIF_IMAGE );

	return InsertItem( mask, pText, imageIndex, imageIndex, state, state, reinterpret_cast< LPARAM >( pObject ),
		hParent, hInsertAfter );
}

const CSize& CTreeControl::GetImageSize( void ) const
{
	if ( 0 == m_imageSize.cx && 0 == m_imageSize.cy )
		if ( CImageList* pImageList = GetImageList( TVSIL_NORMAL ) )
			m_imageSize = gdi::GetImageSize( *pImageList );

	return m_imageSize;
}

bool CTreeControl::GetIconItemRect( CRect* pItemImageRect, HTREEITEM hItem ) const
{
	ASSERT_PTR( pItemImageRect );
	ASSERT_PTR( hItem );
	CRect itemTextRect;
	if ( !GetItemRect( hItem, &itemTextRect, TRUE ) )
		return false;			// item is not visible

	pItemImageRect->SetRect( 0, 0, GetImageSize().cx, GetImageSize().cy );
	ui::AlignRect( *pItemImageRect, itemTextRect, H_AlignLeft | V_AlignCenter );

	enum { IconTextSpacing = 3 };
	pItemImageRect->OffsetRect( -( IconTextSpacing + pItemImageRect->Width() ), 0 );
	return true;
}

bool CTreeControl::CustomDrawItemIcon( const NMTVCUSTOMDRAW* pDraw, HICON hIcon, int diFlags /*= DI_NORMAL | DI_COMPAT*/ )
{
	ASSERT_PTR( pDraw );
	ASSERT_PTR( hIcon );

	HTREEITEM hItem = (HTREEITEM)pDraw->nmcd.dwItemSpec;
	CRect itemImageRect;
	if ( !GetIconItemRect( &itemImageRect, hItem ) )
		return false;

	::DrawIconEx( pDraw->nmcd.hdc, itemImageRect.left, itemImageRect.top, hIcon, itemImageRect.Width(), itemImageRect.Height(), 0, NULL, diFlags );
	return true;
}

void CTreeControl::MarkItem( HTREEITEM hItem, const ui::CTextEffect& textEfect )
{
	m_markedItems[ hItem ] = textEfect;
}

void CTreeControl::UnmarkItem( HTREEITEM hItem )
{
	stdext::hash_map< HTREEITEM, ui::CTextEffect >::iterator itFound = m_markedItems.find( hItem );
	if ( itFound != m_markedItems.end() )
		m_markedItems.erase( itFound );
}

void CTreeControl::ClearMarkedItems( void )
{
	m_markedItems.clear();
	Invalidate();
}

const ui::CTextEffect* CTreeControl::FindTextEffect( HTREEITEM hItem ) const
{
	return utl::FindValuePtr( m_markedItems, hItem );
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

	return
		( HasFlag( pNmTreeView->action, TVE_EXPAND ) && !HasFlag( pNmTreeView->itemNew.state, TVIS_EXPANDEDONCE ) ) ||
		( HasFlag( pNmTreeView->action, TVE_TOGGLE ) && !HasFlag( pNmTreeView->itemNew.state, TVIS_EXPANDED ) );
}

BOOL CTreeControl::EnsureVisible( HTREEITEM hItem )
{
	BOOL result = CTreeCtrl::EnsureVisible( hItem );

	CRect itemImageRect;
	if ( GetIconItemRect( &itemImageRect, hItem ) )
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
	tv::CNmTreeItem treeItemInfo( this, tv::TVN_REFRESHITEM, hItem );

	// notify parent to refresh the item
	return 0 == treeItemInfo.m_nmHdr.NotifyParent();		// false if refresh was rejected
}

void CTreeControl::ExpandBranch( HTREEITEM hItem, bool expand /*= true*/ )
{
	{
		CScopedLockRedraw freeze( this );
		ForEach( func::ExpandItem( expand ? TVE_EXPAND : TVE_COLLAPSE ), hItem );
	}
	EnsureVisible( hItem );
}

bool CTreeControl::UpdateCustomImagerBoundsSize( void )
{
	if ( m_pCustomImager.get() != NULL )
		return m_pCustomImager->SetCurrGlyphGauge( ui::SmallGlyph );

	return false;			// no change
}

void CTreeControl::SetupControl( void )
{
	CListLikeCtrlBase::SetupControl();

	m_indentNoImages = GetIndent();
	if ( m_pImageList != NULL )
		SetImageList( m_pImageList, TVSIL_NORMAL );

	UpdateCustomImagerBoundsSize();
}

void CTreeControl::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	SetupControl();
}

BOOL CTreeControl::PreTranslateMessage( MSG* pMsg )
{
	return
		TranslateMessage( pMsg ) ||
		__super::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CTreeControl, CTreeCtrl )
	ON_WM_NCLBUTTONDOWN()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT_EX( TVN_SELCHANGED, OnTvnSelChanged_Reflect )
	ON_NOTIFY_REFLECT_EX( NM_RCLICK, OnTvnRClick_Reflect )
	ON_NOTIFY_REFLECT_EX( NM_DBLCLK, OnTvnDblClk_Reflect )
	ON_NOTIFY_REFLECT_EX( NM_CUSTOMDRAW, OnNmCustomDraw_Reflect )
	ON_COMMAND( ID_EDIT_COPY, OnEditCopy )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateEditCopy )
END_MESSAGE_MAP()

void CTreeControl::OnNcLButtonDown( UINT hitTest, CPoint point )
{
	ui::TakeFocus( m_hWnd );
	CTreeCtrl::OnNcLButtonDown( hitTest, point );
}

void CTreeControl::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	UINT flags;
	HTREEITEM hitIndex = HitTest( ui::ScreenToClient( m_hWnd, screenPos ), &flags );
	hitIndex, pWnd;

	if ( m_contextMenu.GetSafeHmenu() != NULL )
	{
		ui::TrackPopupMenu( m_contextMenu, m_pTrackMenuTarget, screenPos );
		return;						// supress rising WM_CONTEXTMENU to the parent
	}

	Default();
}

BOOL CTreeControl::OnTvnSelChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMTREEVIEW* pTreeInfo = (NMTREEVIEW*)pNmHdr;
	pTreeInfo, pResult;
	return IsInternalChange();		// don't raise the notification to list's parent during an internal change
}

BOOL CTreeControl::OnTvnRClick_Reflect( NMHDR* /*pNmHdr*/, LRESULT* pResult )
{
	if ( HTREEITEM hHitItem = GetDropHilightItem() )					// item right clicked (temporary highlighted)
		if ( RefreshItem( hHitItem ) )
			if ( hHitItem != GetSelectedItem() )
				CTreeCtrl::SelectItem( hHitItem );

	SendMessage( WM_CONTEXTMENU, (WPARAM)m_hWnd, GetMessagePos() );		// send WM_CONTEXTMENU to self
  	*pResult = 1;			// mark message as handled and suppress default handling
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

BOOL CTreeControl::OnNmCustomDraw_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMTVCUSTOMDRAW* pDraw = (NMTVCUSTOMDRAW*)pNmHdr;
	if ( CListLikeCustomDrawBase::IsTooltipDraw( &pDraw->nmcd ) )
		return TRUE;		// IMP: avoid custom drawing for tooltips

	//TRACE( _T(" CTreeControl::DrawStage: %s\n"), dbg::FormatDrawStage( pDraw->nmcd.dwDrawStage ) );

	CTreeControlCustomDraw draw( pDraw, this );

	*pResult = CDRF_DODEFAULT;
	switch ( pDraw->nmcd.dwDrawStage )
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:
			*pResult = CDRF_NEWFONT | CDRF_NOTIFYITEMDRAW;

			if ( draw.ApplyItemTextEffect() )
				*pResult |= CDRF_NEWFONT;

			if ( m_pCustomImager.get() != NULL )
				*pResult |= CDRF_NOTIFYPOSTPAINT;				// will superimpose the thumbnail on top of transparent image (on CDDS_ITEMPOSTPAINT drawing stage)

			break;

		case CDDS_ITEMPOSTPAINT:
			if ( m_pCustomImager.get() != NULL )
			{
				CRect itemImageRect;
				if ( GetIconItemRect( &itemImageRect, draw.m_hItem ) )		// visible item?
					if ( m_pCustomImager->DrawItemGlyph( &pDraw->nmcd, itemImageRect ) )
						return TRUE;				// handled
			}

			break;
	}

	if ( !ParentHandles( PN_CustomDraw ) )
		return TRUE;			// mark as handled so changes are applied

	return FALSE;				// continue handling by parent, even if changed (additive logic)
}

void CTreeControl::OnEditCopy( void )
{
	Copy();
}

void CTreeControl::OnUpdateEditCopy( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( GetSelectedItem() != NULL );
}
