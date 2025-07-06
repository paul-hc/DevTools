#ifndef TreeWndPage_h
#define TreeWndPage_h
#pragma once

#include <unordered_map>
#include "utl/UI/LayoutPropertyPage.h"
#include "utl/UI/TreeControl.h"
#include "utl/UI/TextEffect.h"
#include "wnd/WndEnumerator.h"
#include "Observers.h"


class CTreeWndPage : public CLayoutPropertyPage
	, public IWndObserver
	, public IEventObserver
	, private ui::ITextEffectCallback
{
public:
	CTreeWndPage( void );
	virtual ~CTreeWndPage();
private:
	// IWndObserver interface
	virtual void OnTargetWndChanged( const CWndSpot& targetWnd );

	// IEventObserver interface
	virtual void OnAppEvent( app::Event appEvent );

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;
private:
	void RefreshTreeContents( void );
	bool RefreshTreeItem( HTREEITEM hItem );

	// targeted refresh
	void RefreshTreeBranch( HTREEITEM hItem );
	void RefreshTreeParentBranch( HTREEITEM hItem );
	HTREEITEM RefreshBranchOf( HWND hWndTarget );		// refresh branch under desktop window down to the hWndTarget item (merge existing, insert missing)

	void SetupTreeItems( void );
	HTREEITEM InsertWndItem_Old( HTREEITEM hItemParent, HWND hWnd, int indent, int image = -1 );

	HTREEITEM FindItemWithWnd( HWND hWnd ) const;
	HWND FindValidParentItem( HTREEITEM hItem ) const;
	HTREEITEM FindValidTargetItem( void ) const;		// target refers to the current CWndSpot
private:
	bool m_destroying;

	// enum { IDD = IDD_TREE_WND_PAGE };
	CTreeControl m_treeCtrl;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnDestroy( void );
	afx_msg void OnTvnSelChanged_WndTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnTvnSerFocus_WndTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnTvnCustomDraw_WndTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnTcnRefreshItem_WndTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void CmExpandBranch( void );
	afx_msg void CmColapseBranch( void );
	afx_msg void OnCopyTreeAncestors( void );
	afx_msg void OnCopyTreeChildren( void );
	afx_msg void OnUpdateCopy( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


class CLogger;


namespace wt
{
	struct CItemInfo
	{
		CItemInfo( HWND hWnd, int image = -1 );
		CItemInfo( const CTreeControl* pTreeCtrl, HTREEITEM hItem );

		bool operator!=( const CItemInfo& right ) const
		{
			return
				m_text != right.m_text ||
				m_item.iImage != right.m_item.iImage ||
				m_item.state != right.m_item.state;
		}
	private:
		void Construct( void );
	public:
		TVITEM m_item;				// note: m_item.lParam stores the HWND window handle
		std::tstring m_text;
	};


	typedef std::pair<HTREEITEM, int> TTreeItemIndent;


	class CWndTreeBuilder : public CWndEnumBase
	{
	public:
		CWndTreeBuilder( CTreeControl* pTreeCtrl, CLogger* pLogger = nullptr );

		size_t GetCount( void ) const { return m_wndToItemMap.size(); }
		void SetInsertedEffect( const ui::CTextEffect& insertedEffect ) { m_insertedEffect = insertedEffect; }

		const TTreeItemIndent& MapParentItem( HTREEITEM hParentItem );			// map insert, starting point for top-down insert/merge

		template< typename IteratorT >
		HTREEITEM MergeBranchPath( IteratorT itStartWnd, IteratorT itEndWnd );	// iterate in top-down order
	protected:
		// base overrides
		virtual void AddWndItem( HWND hWnd );
	private:
		const TTreeItemIndent& InsertWndItem( HWND hWnd, const TTreeItemIndent& parentPair, HTREEITEM hInsertAfter = TVI_LAST );
		const TTreeItemIndent& MergeWndItem( HWND hWnd, const TTreeItemIndent& parentPair );
		HTREEITEM FindInsertAfter( HTREEITEM hParentItem, HWND hWnd ) const;

		const TTreeItemIndent* FindWndItem( HWND hWnd ) const;
		void LogWnd( HWND hWnd, int indent ) const;

		const TTreeItemIndent& RegisterParentWnd( HWND hWndParent );			// map lookup or insert
	private:
		CTreeControl* m_pTreeCtrl;
		CLogger* m_pLogger;
		ui::CTextEffect m_insertedEffect;
		std::unordered_map<HWND, TTreeItemIndent> m_wndToItemMap;		// HWND -> [item, indent]
	};
}


#endif // TreeWndPage_h
