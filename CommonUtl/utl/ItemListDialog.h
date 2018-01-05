#ifndef ItemListDialog_h
#define ItemListDialog_h
#pragma once

#include "AccelTable.h"
#include "DialogToolBar.h"
#include "ItemContent.h"
#include "LayoutDialog.h"
#include "ReportListControl.h"


class CItemListDialog : public CLayoutDialog
{
public:
	CItemListDialog( CWnd* pParent, const ui::CItemContent& content, const TCHAR* pTitle = NULL );
public:
	std::vector< std::tstring > m_items;
	bool m_readOnly;
private:
	ui::CItemContent m_content;
	const TCHAR* m_pTitle;

	// enum { IDD = IDD_ITEM_LIST_DIALOG };
	CReportListControl m_sepListCtrl;
	CDialogToolBar m_toolbar;
	CAccelTable m_accel;
	enum Column { Item };
private:
	bool m_addingItem;
private:
	bool EditItem( int itemIndex );
public:
	// generated overrides
	public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnCommand( WPARAM wParam, LPARAM lParam );
	virtual BOOL OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult );
protected:
	// generated message map
	afx_msg void OnDestroy( void );
	afx_msg void OnLvnItemChanged_Items( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnBeginLabelEdit_Items( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnEndLabelEdit_Items( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnAddItem( void );
	afx_msg void OnUpdateAddItem( CCmdUI* pCmdUI );
	afx_msg void OnRemoveItem( void );
	afx_msg void OnUpdateRemoveItem( CCmdUI* pCmdUI );
	afx_msg void OnRemoveAll( void );
	afx_msg void OnUpdateRemoveAll( CCmdUI* pCmdUI );
	afx_msg void OnEditItem( void );
	afx_msg void OnUpdateEditItem( CCmdUI* pCmdUI );
	afx_msg void OnMoveUpItem( void );
	afx_msg void OnUpdateMoveUpItem( CCmdUI* pCmdUI );
	afx_msg void OnMoveDownItem( void );
	afx_msg void OnUpdateMoveDownItem( CCmdUI* pCmdUI );
	afx_msg void OnCopyItems( void );
	afx_msg void OnUpdateCopyItems( CCmdUI* pCmdUI );
	afx_msg void OnPasteItems( void );
	afx_msg void OnUpdatePasteItems( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // ItemListDialog_h
