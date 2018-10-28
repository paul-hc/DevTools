#ifndef ItemListDialog_h
#define ItemListDialog_h
#pragma once

#include "AccelTable.h"
#include "DialogToolBar.h"
#include "ItemContent.h"
#include "LayoutDialog.h"
#include "LayoutChildPropertySheet.h"
#include "LayoutPropertyPage.h"
#include "ReportListControl.h"
#include "TextEdit.h"


namespace detail
{
	interface IContentPage
	{
		virtual bool InEditMode( void ) const = 0;		// list control in edit mode?
		virtual bool EditSelItem( void ) = 0;
	};
}


class CItemListDialog : public CLayoutDialog
{
public:
	CItemListDialog( CWnd* pParent, const ui::CItemContent& content, const TCHAR* pTitle = NULL );

	const ui::CItemContent& GetContent( void ) const { return m_content; }

	size_t GetSelItemPos( void ) const { return m_selItemPos; }
	void SetSelItemPos( size_t selItemPos );

	int GetSelItemIndex( void ) const { return static_cast< int >( m_selItemPos ); }

	bool InputItem( size_t itemPos, const std::tstring& newItem );
	bool InputAllItems( const std::vector< std::tstring >& items );
private:
	detail::IContentPage* GetActivePage( void ) const { return dynamic_cast< detail::IContentPage* >( m_childSheet.GetActivePage() ); }
	bool InEditMode( void ) const;
	bool EditSelItem( void );

	static const TCHAR* FindSeparatorMostUsed( const std::tstring& text );
public:
	std::vector< std::tstring > m_items;
	bool m_readOnly;
private:
	ui::CItemContent m_content;
	const TCHAR* m_pTitle;
	size_t m_selItemPos;
	CInternalChange m_internal;

	// enum { IDD = IDD_ITEM_LIST_DIALOG };
	CDialogToolBar m_toolbar;
	CLayoutChildPropertySheet m_childSheet;
	enum Page { ListPage, EditPage };
public:
	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
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


class CItemsListPage : public CLayoutPropertyPage
					 , public detail::IContentPage
{
public:
	CItemsListPage( CItemListDialog* pDialog );
	virtual ~CItemsListPage();

	// detail::IContentPage interface
	virtual bool InEditMode( void ) const;
	virtual bool EditSelItem( void );
private:
	void QueryListItems( std::vector< std::tstring >& rItems ) const;
	void OutputList( void );
private:
	CItemListDialog* m_pDialog;
	const ui::CItemContent& m_rContent;
	bool m_inEdit;

	std::vector< ui::CPathItem* > m_pathItems;
private:
	enum Column { Item };

	CReportListControl m_listCtrl;
	CAccelTable m_accel;

	// generated stuff
public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnLvnItemChanged_Items( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


class CItemsEditPage : public CLayoutPropertyPage
					 , public detail::IContentPage
{
public:
	CItemsEditPage( CItemListDialog* pDialog );

	// detail::IContentPage interface
	virtual bool InEditMode( void ) const;
	virtual bool EditSelItem( void );
private:
	Range< int > GetLineRange( int linePos ) const;
	Range< int > SelectLine( int linePos );

	void QueryEditItems( std::vector< std::tstring >& rItems ) const;
	void OutputEdit( void );
	void OnSelectedLinesChanged( void );
private:
	CItemListDialog* m_pDialog;
	const ui::CItemContent& m_rContent;
	Range< int > m_selLineRange;
	static const TCHAR s_lineEnd[];
private:
	CTextEdit m_mlEdit;
	CAccelTable m_accel;

	// generated stuff
public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg LRESULT OnIdleUpdateCmdUI( WPARAM wParam, LPARAM lParam );
	afx_msg void OnEditSelectAll( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ItemListDialog_h
