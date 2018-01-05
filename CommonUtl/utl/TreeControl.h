#ifndef TreeControl_h
#define TreeControl_h
#pragma once

#include "InternalChange.h"


class CTreeControl : public CTreeCtrl
				   , public CInternalChange
{
public:
	CTreeControl( void );
	virtual ~CTreeControl();

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

	CMenu& GetContextMenu( void ) { return m_contextMenu; }

	// item image
	const CSize& GetImageSize( void ) const;
	bool GetItemImageRect( CRect& rItemImageRect, HTREEITEM hItem ) const;
	bool CustomDrawItemIcon( const NMTVCUSTOMDRAW* pDraw, HICON hIcon, int diFlags = DI_NORMAL | DI_COMPAT );		// works with transparent item image
public:
	// custom notifications: handled the standard way with ON_NOTIFY( NotifyCode, id, memberFxn )
	enum NotifCode { TCN_REFRESHITEM = TVN_FIRST - 77 };

	struct NMTREEITEM
	{
		NMHDR hdr;
		HTREEITEM hItem;
	};
protected:
	CMenu m_contextMenu;
private:
	mutable CSize m_imageSize;			// self-encapsulated
public:
	// generated overrides
	public:
	virtual void PreSubclassWindow( void );
protected:
	// message map functions
	afx_msg void OnNcLButtonDown( UINT hitTest, CPoint point );
	afx_msg BOOL OnTvnSelChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnTvnRClick_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnTvnDblClk_Reflect( NMHDR* pNmHdr, LRESULT* pResult );

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
