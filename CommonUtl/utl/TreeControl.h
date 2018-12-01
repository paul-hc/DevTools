#ifndef TreeControl_h
#define TreeControl_h
#pragma once

#include "ListLikeCtrlBase.h"
#include "ui_fwd.h"				// ui::CNmHdr
#include <hash_map>


class CTreeControlCustomDraw;


namespace tv
{
	// custom notifications: handled the standard way with ON_NOTIFY( NotifyCode, id, memberFxn )
	enum NotifyCode
	{
		TVN_REFRESHITEM = TVN_FIRST - 77		// pass tv::CNmTreeItem; client could return TRUE if refresh was rejected
	};


	struct CNmTreeItem
	{
		CNmTreeItem( const CTreeCtrl* pTreeCtrl, NotifyCode notifyCode, HTREEITEM hItem )
			: m_nmHdr( pTreeCtrl, notifyCode ), m_hItem( hItem ) {}
	public:
		ui::CNmHdr m_nmHdr;
		HTREEITEM m_hItem;
	};
}


class CTreeControl
	: public CTreeCtrl
	, public CListLikeCtrlBase
{
	friend class CTreeControlCustomDraw;
public:
	CTreeControl( void );
	virtual ~CTreeControl();

	CMenu& GetContextMenu( void ) { return m_contextMenu; }
	void StoreImageList( CImageList* pImageList );
	CImageList* SetImageList( CImageList* pImageList, int imageType );		// pseudo-override

	bool DeleteAllItems( void );
	bool Copy( void );

	// custom imager
	virtual void SetCustomFileGlyphDraw( bool showGlyphs = true );		// ICustomDrawControl base override
	void SetCustomImageDraw( ui::ICustomImageDraw* pCustomImageDraw, const CSize& imageSize = CSize( 0, 0 ) );

	// item image
	const CSize& GetImageSize( void ) const;
	bool GetIconItemRect( CRect* pItemImageRect, HTREEITEM hItem ) const;
	bool CustomDrawItemIcon( const NMTVCUSTOMDRAW* pDraw, HICON hIcon, int diFlags = DI_NORMAL | DI_COMPAT );		// works with transparent item image

	// custom item marking: color, bold, italic, underline
	void MarkItem( HTREEITEM hItem, const ui::CTextEffect& textEfect );
	void UnmarkItem( HTREEITEM hItem );
	void ClearMarkedItems( void );

	const ui::CTextEffect* FindTextEffect( HTREEITEM hItem ) const;

	// item interface
	virtual HTREEITEM InsertObjectItem( HTREEITEM hParent, const utl::ISubject* pObject, int imageIndex = ui::No_Image, UINT state = TVIS_EXPANDED,
										HTREEITEM hInsertAfter = TVI_LAST, const TCHAR* pText = NULL );		// pText could be LPSTR_TEXTCALLBACK

	template< typename ObjectT >
	ObjectT* GetItemObject( HTREEITEM hItem ) const { return AsPtr< ObjectT >( GetItemData( hItem ) ); }

	template< typename ObjectT >
	void SetItemObject( HTREEITEM hItem, ObjectT* pObject ) { VERIFY( SetItemData( hItem, (DWORD_PTR)pObject ) != FALSE ); }

	template< typename Type >
	Type GetItemDataAs( HTREEITEM hItem ) const { return (Type)GetItemData( hItem ); }

	template< typename Type >
	void SetItemDataAs( HTREEITEM hItem, Type data ) { VERIFY( SetItemData( hItem, (DWORD_PTR)data ) != FALSE ); }

	bool IsRealItem( HTREEITEM hItem ) const;
	bool IsExpandLazyChildren( const NMTREEVIEW* pNmTreeView ) const;

	bool HasItemState( HTREEITEM hItem, UINT stateMask ) const { return ( GetItemState( hItem, stateMask ) & stateMask ) != 0; }

	BOOL EnsureVisible( HTREEITEM hItem );				// including item icon
	bool SelectItem( HTREEITEM hItem );					// select item and make it visible
	bool RefreshItem( HTREEITEM hItem );				// notifies parent to refresh the item

	// tree algorithms
	template< typename Pred >
	HTREEITEM FirstThat( Pred pred, HTREEITEM hItem = TVI_ROOT ) const;

	template< typename Func >
	void ForEach( Func func, HTREEITEM hItem = TVI_ROOT );

	void ExpandBranch( HTREEITEM hItem, bool expand = true );

	template< typename Type >
	HTREEITEM FindItemWithData( Type data, HTREEITEM hItem = TVI_ROOT ) const;
protected:
	void ClearData( void );

	// base overrides
	virtual void SetupControl( void );
private:
	bool UpdateCustomImagerBoundsSize( void );
protected:
	CMenu m_contextMenu;
private:
	CImageList* m_pImageList;			// for TVSIL_NORMAL type
	stdext::hash_map< HTREEITEM, ui::CTextEffect > m_markedItems;

	UINT m_indentNoImages, m_indentWithImages;
	mutable CSize m_imageSize;			// self-encapsulated

	// generated overrides
public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg void OnNcLButtonDown( UINT hitTest, CPoint point );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg BOOL OnTvnSelChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnTvnRClick_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnTvnDblClk_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnNmCustomDraw_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnEditCopy( void );
	afx_msg void OnUpdateEditCopy( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


namespace func
{
	struct ExpandItem
	{
		ExpandItem( UINT code = TVE_EXPAND ) : m_code( code ) {}

		void operator()( CTreeCtrl* pTreeCtrl, HTREEITEM hItem ) const
		{
			pTreeCtrl->Expand( hItem, TVE_EXPAND );
		}
	private:
		UINT m_code;		// TVE_EXPAND/TVE_COLLAPSE
	};

	struct HasItemData
	{
		template< typename Type >
		HasItemData( Type data ) : m_data( (DWORD_PTR)data ) { ASSERT( sizeof( data ) <= sizeof( m_data ) ); }

		bool operator()( const CTreeCtrl* pTreeCtrl, HTREEITEM hItem ) const
		{
			return m_data == pTreeCtrl->GetItemData( hItem );
		}
	private:
		DWORD_PTR m_data;
	};
}


// template code

template< typename Func >
void CTreeControl::ForEach( Func func, HTREEITEM hItem /*= TVI_ROOT*/ )
{
	if ( hItem != TVI_ROOT )			// could be TVI_ROOT to iterate multiple root items
		func( this, hItem );

	for ( HTREEITEM hChild = GetChildItem( hItem ); hChild != NULL; hChild = GetNextSiblingItem( hChild ) )
		ForEach( func, hChild );
}

template< typename Pred >
HTREEITEM CTreeControl::FirstThat( Pred pred, HTREEITEM hItem /*= TVI_ROOT*/ ) const
{
	if ( hItem != TVI_ROOT )			// could be TVI_ROOT to iterate multiple root items
		if ( pred( this, hItem ) )
			return hItem;

	for ( HTREEITEM hChild = GetChildItem( hItem ); hChild != NULL; hChild = GetNextSiblingItem( hChild ) )
		if ( HTREEITEM hFound = FirstThat( pred, hChild ) )
			return hFound;

	return NULL;
}

template< typename Type >
inline HTREEITEM CTreeControl::FindItemWithData( Type data, HTREEITEM hItem /*= TVI_ROOT*/ ) const
{
	if ( NULL == hItem )
		return NULL;

	return FirstThat( func::HasItemData( data ), hItem );
}


#endif // TreeControl_h
