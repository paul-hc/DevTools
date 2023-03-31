#ifndef TreeControl_h
#define TreeControl_h
#pragma once

#include "ListLikeCtrlBase.h"
#include "BaseTrackMenuWnd.h"
#include "ui_fwd.h"				// ui::CNmHdr
#include <unordered_map>


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


class CTreeControl : public CBaseTrackMenuWnd<CTreeCtrl>
	, public CListLikeCtrlBase
{
	friend class CTreeControlCustomDraw;

	using CBaseTrackMenuWnd<CTreeCtrl>::DeleteItem;
public:
	CTreeControl( void );
	virtual ~CTreeControl();

	CMenu& GetContextMenu( void ) { return m_contextMenu; }

	void StoreImageList( CImageList* pImageList );
	CImageList* SetImageList( CImageList* pImageList, int imageType );		// pseudo-override

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

	COLORREF GetActualTextColor( void ) const;
	const ui::CTextEffect* FindTextEffect( HTREEITEM hItem ) const;

	// item interface
	virtual HTREEITEM InsertObjectItem( HTREEITEM hParent, const utl::ISubject* pObject, int imageIndex = ui::No_Image, UINT state = TVIS_EXPANDED,
										HTREEITEM hInsertAfter = TVI_LAST, const TCHAR* pText = nullptr );		// pText could be LPSTR_TEXTCALLBACK

	bool DeleteAllItems( void );
	bool DeleteItem( HTREEITEM hItem );
	void DeleteChildren( HTREEITEM hItem );

	template< typename ObjectT >
	ObjectT* GetItemObject( HTREEITEM hItem ) const { return AsPtr<ObjectT>( GetItemData( hItem ) ); }

	template< typename ObjectT >
	void SetItemObject( HTREEITEM hItem, ObjectT* pObject ) { VERIFY( SetItemData( hItem, (DWORD_PTR)pObject ) != FALSE ); }

	template< typename Type >
	Type GetItemDataAs( HTREEITEM hItem ) const { return AsValue<Type>( GetItemData( hItem ) ); }

	template< typename Type >
	void SetItemDataAs( HTREEITEM hItem, Type data ) { VERIFY( SetItemData( hItem, (DWORD_PTR)data ) != FALSE ); }

	bool IsRealItem( HTREEITEM hItem ) const;
	bool IsExpandLazyChildren( const NMTREEVIEW* pNmTreeView ) const;

	bool HasItemState( HTREEITEM hItem, UINT stateMask ) const { return ( GetItemState( hItem, stateMask ) & stateMask ) != 0; }

	int GetItemIndentLevel( HTREEITEM hItem ) const;		// 0 for root item, etc

	BOOL EnsureVisible( HTREEITEM hItem );				// including item icon
	bool SelectItem( HTREEITEM hItem );					// select item and make it visible
	bool RefreshItem( HTREEITEM hItem );				// notifies parent to refresh the item

	// selection
	template< typename ObjectT >
	ObjectT* GetSelected( void ) const;

	bool SetSelected( const utl::ISubject* pObject );

	// tree algorithms
	template< typename Pred >
	HTREEITEM FirstThat( Pred pred, HTREEITEM hStart = TVI_ROOT ) const;

	template< typename Func >
	void ForEach( Func func, HTREEITEM hStart = TVI_ROOT );

	void ExpandBranch( HTREEITEM hItem, bool expand = true );

	template< typename Type >
	HTREEITEM FindItemWithData( Type data, HTREEITEM hStart = TVI_ROOT, RecursionDepth depth = Deep ) const;

	HTREEITEM FindItemWithObject( const utl::ISubject* pObject, HTREEITEM hStart = TVI_ROOT, RecursionDepth depth = Deep ) const { return FindItemWithData( pObject, hStart, depth ); }
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
	std::unordered_map<HTREEITEM, ui::CTextEffect> m_markedItems;

	UINT m_indentNoImages, m_indentWithImages;
	mutable CSize m_imageSize;			// self-encapsulated

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
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
}


namespace pred
{
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

template< typename ObjectT >
ObjectT* CTreeControl::GetSelected( void ) const
{
	HTREEITEM hSelItem = GetSelectedItem();
	return hSelItem != nullptr ? GetItemObject<ObjectT>( hSelItem ) : nullptr;
}

template< typename Func >
void CTreeControl::ForEach( Func func, HTREEITEM hStart /*= TVI_ROOT*/ )
{
	if ( hStart != TVI_ROOT )			// could be TVI_ROOT to iterate multiple root items
		func( this, hStart );

	for ( HTREEITEM hChild = GetChildItem( hStart ); hChild != nullptr; hChild = GetNextSiblingItem( hChild ) )
		ForEach( func, hChild );
}

template< typename Pred >
HTREEITEM CTreeControl::FirstThat( Pred pred, HTREEITEM hStart /*= TVI_ROOT*/ ) const
{
	if ( hStart != TVI_ROOT )			// could be TVI_ROOT to iterate multiple root items
		if ( pred( this, hStart ) )
			return hStart;

	for ( HTREEITEM hChild = GetChildItem( hStart ); hChild != nullptr; hChild = GetNextSiblingItem( hChild ) )
		if ( HTREEITEM hFound = FirstThat( pred, hChild ) )
			return hFound;

	return nullptr;
}

template< typename Type >
inline HTREEITEM CTreeControl::FindItemWithData( Type data, HTREEITEM hStart /*= TVI_ROOT*/, RecursionDepth depth /*= Deep*/ ) const
{
	if ( nullptr == hStart )
		return nullptr;

	pred::HasItemData hasDataPred( data );

	if ( Deep == depth )
		return FirstThat( hasDataPred, hStart );

	// children shallow search
	for ( HTREEITEM hChild = GetChildItem( hStart ); hChild != nullptr; hChild = GetNextSiblingItem( hChild ) )
		if ( hasDataPred( this, hChild ) )
			return hChild;

	return nullptr;
}


#endif // TreeControl_h
