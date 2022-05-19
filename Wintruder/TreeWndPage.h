#ifndef TreeWndPage_h
#define TreeWndPage_h
#pragma once

#include <hash_map>
#include "utl/UI/LayoutPropertyPage.h"
#include "utl/UI/TreeControl.h"
#include "wnd/WndEnumerator.h"
#include "Observers.h"


class CTreeWndPage
	: public CLayoutPropertyPage
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

	void SetupTreeItems( void );
	HTREEITEM InsertWndItem( HTREEITEM hItemParent, HWND hWnd, int indent, int image = -1 );
	HTREEITEM FindItemWithWnd( HWND hWnd ) const;
	HWND FindValidParentItem( HTREEITEM hItem ) const;
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
		TVITEM m_item;
		std::tstring m_text;
	};


	class CWndTreeBuilder : public CWndEnumBase
	{
	public:
		CWndTreeBuilder( CTreeControl* pTreeCtrl, CLogger* pLogger = NULL );

		size_t GetCount( void ) const { return m_wndToItemMap.size(); }
	protected:
		// base overrides
		virtual void AddWndItem( HWND hWnd );
	private:
		const std::pair<HTREEITEM, int>* FindWndItem( HWND hWnd ) const;
		void LogWnd( HWND hWnd, int indent ) const;
	private:
		CTreeControl* m_pTreeCtrl;
		CLogger* m_pLogger;
		stdext::hash_map< HWND, std::pair<HTREEITEM, int> > m_wndToItemMap;		// HWND -> [item, indent]
	};
}


#endif // TreeWndPage_h
