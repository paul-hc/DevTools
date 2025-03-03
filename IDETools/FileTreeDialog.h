#ifndef FileTreeDialog_h
#define FileTreeDialog_h
#pragma once

#include <map>
#include "utl/Path.h"
#include "utl/UI/AccelTable.h"
#include "utl/UI/EnumComboBox.h"
#include "utl/UI/ItemContentEdit.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/TreeControl.h"
#include "FileAssoc.h"
#include "IncludeOptions.h"
#include "IncludeNode.h"


struct CIncludeNode;
typedef std::pair<HTREEITEM, CIncludeNode*> TTreeItemPair;


class CFileTreeDialog : public CLayoutDialog
{
public:
	typedef void (*TIterFunc)( CTreeControl* pTreeCtrl, HTREEITEM hItem, void* pArgs, int nestingLevel );

	CFileTreeDialog( const fs::CPath& rootPath, CWnd* pParent );
	virtual ~CFileTreeDialog();

	const fs::CPath& GetRootPath( void ) const { return m_rootPath; }
	void SetRootPath( const fs::CPath& rootPath );
	void OutputRootPath( void ) { SetRootPath( m_rootPath.Get() ); }

	int GetSourceLineNo( void ) const { return m_sourceLineNo; }

	bool SetViewMode( ViewMode viewMode );
	bool SetOrder( Ordering ordering );

	bool AnySelected( void ) const { return m_treeCtrl.GetSelectedItem() != nullptr; }
private:
	void Clear( void );
	void BuildIncludeTree( void );
	bool UpdateCurrentSelection( void );
	bool AddIncludedChildren( TTreeItemPair& rParentPair, bool doRecurse = true, bool avoidCircularDependency = false );

	TTreeItemPair AddTreeItem( HTREEITEM hParent, CIncludeNode* pItemInfo, int& rOrderIndex, bool& rOriginalItem );
	std::tstring BuildItemText( const CIncludeNode* pItemInfo, ViewMode viewMode = vmDefaultMode ) const;
	void RefreshItemsText( HTREEITEM hItem = TVI_ROOT );
	CIncludeNode* GetItemInfo( HTREEITEM hItem ) const { return m_treeCtrl.GetItemDataAs<CIncludeNode*>( hItem ); }

	void ForEach( TIterFunc pIterFunc, HTREEITEM hItem = TVI_ROOT, void* pArgs = nullptr, int nestingLevel = -1 );

	bool ReorderChildren( HTREEITEM hParent = TVI_ROOT );
	HTREEITEM FindNextItem( HTREEITEM hStartItem, bool forward = true );
	HTREEITEM FindOriginalItem( HTREEITEM hStartItem );

	friend void _RefreshItemText( CTreeControl* pTreeCtrl, HTREEITEM hItem, CFileTreeDialog* pDialog, int nestingLevel );
	friend void _SafeBindItem( CTreeControl* pTreeCtrl, HTREEITEM hItem, CFileTreeDialog* pDialog, int nestingLevel );

	friend int CALLBACK SortCallback( const CIncludeNode* pLeft, const CIncludeNode* pRight, CFileTreeDialog* pDialog );
private:
	bool IsCircularDependency( const TTreeItemPair& currItem ) const;
	bool SafeBindItem( HTREEITEM hItem, CIncludeNode* pTreeItem );
	void BindAllItems( void ) { ForEach( (TIterFunc)_SafeBindItem, TVI_ROOT, this ); }

	bool IsRootItem( HTREEITEM hItem ) const { return hItem != nullptr && nullptr == m_treeCtrl.GetParentItem( hItem ); }
	bool IsOriginalItem( HTREEITEM hItem ) const;
	bool IsFileExcluded( const fs::CPath& path ) const;
	bool HasValidComplementary( void ) const;
	TTreeItemPair GetSelectedItem( void ) const;

	void UpdateOptionCtrl( void );

	typedef std::map<fs::CPath, HTREEITEM> TPathToItemMap;
private:
	fs::CPath m_rootPath;
	CIncludeOptions& m_rOpt;

	std::vector<CIncludeNode*> m_treeItems;				// has ownership
	int m_sourceLineNo;

	TPathToItemMap m_originalItems;

	CFileAssoc m_fileAssoc;
	fs::CPath m_complemFilePath;
	std::tstring m_titlePrefix;

	int m_nestingLevel;
public:
	// enum { IDD = IDD_FILE_TREE_DIALOG };

	CEnumComboBox m_depthLevelCombo;
	CTreeControl m_treeCtrl;
	CItemContentEdit m_selFilePathEdit;
	CEnumComboBox m_viewModeCombo;
	CEnumComboBox m_orderCombo;

	CAccelTable m_accelTreeFocus;

	// generated stuff
public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual void OnOK( void );
	afx_msg void OnDestroy( void );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu );
	afx_msg void TVnSelChanged_FileTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void TVnItemExpanding_FileTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void TVnGetDispInfo_FileTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void TVnCustomDraw_FileTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void TVnRclick_FileTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void TVnDblclk_FileTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void TVnBeginLabelEdit_FileTree(NMHDR* pNmHdr, LRESULT* pResult);
	afx_msg void TVnGetInfoTip_FileTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void CBnSelChangeViewModeCombo( void );
	afx_msg void CBnSelChangeOrder( void );
	afx_msg void CmUpdateDialogTitle( void );
	afx_msg void CmOptions( void );
	afx_msg void CmFileOpen( void );
	afx_msg void CmFindNext( void );
	afx_msg void CmFindPrev( void );
	afx_msg void CmFindOriginal( void );
	afx_msg void CmExpandBranch( void );
	afx_msg void CmColapseBranch( void );
	afx_msg void CmRefresh( void );
	afx_msg void CmOpenComplementary( void );
	afx_msg void CmOpenIncludeLine( void );
	afx_msg void CmExploreFile( void );
	afx_msg void CmCopyFullPath( void );
	afx_msg void CmBrowseFile( void );
	afx_msg void CBnCloseupDepthLevel( void );
	afx_msg void OnToggle_LazyParsing( void );
	afx_msg void OnToggle_NoDuplicates( void );
	afx_msg void CmEditLabel( void );
	afx_msg void UUI_EditLabel( CCmdUI* pCmdUI );
	virtual void OnCancel( void );
	afx_msg void CmLocateFile( void );
	afx_msg void CmCopyToClipboard();
	afx_msg void CmViewFile( UINT cmdId );
	afx_msg void UUI_AnySelected( CCmdUI* pCmdUI );
	afx_msg void UUI_OpenIncludeLine( CCmdUI* pCmdUI );
	afx_msg void UUI_FindDupOccurences( CCmdUI* pCmdUI );
	afx_msg void UUI_OpenComplementary( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // FileTreeDialog_h
