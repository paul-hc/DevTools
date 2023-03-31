#ifndef CtrlUiState_h
#define CtrlUiState_h
#pragma once

#include "InternalChange.h"


class CEditUiState
{
public:
	CEditUiState( CEdit& rEditBox ) : m_rEditBox( rEditBox ) { m_rEditBox.GetSel( m_selStart, m_selEnd ); }
	~CEditUiState() { m_rEditBox.SetSel( m_selStart, m_selEnd ); }
public:
	CEdit& m_rEditBox;
	int m_selStart, m_selEnd;
};


class CListBoxUiState
{
public:
	CListBoxUiState( CListBox& rListBox ) : m_rListBox( rListBox ), m_caretIndex( m_rListBox.GetCaretIndex() ), m_topVisibleIndex( m_rListBox.GetTopIndex() ) {}

	void Restore( void );
public:
	CListBox& m_rListBox;
	int m_caretIndex;
	int m_topVisibleIndex;
};


class CTreeCtrlUiState
{
public:
	CTreeCtrlUiState( void );
	~CTreeCtrlUiState();

	void SaveVisualState( const CTreeCtrl& treeCtrl );
	void RestoreVisualState( CTreeCtrl& rTreeCtrl ) const;
public:
	void SaveExpandingState( const CTreeCtrl& treeCtrl );
	void RestoreExpandingState( CTreeCtrl& rTreeCtrl ) const;

	void SaveSelection( const CTreeCtrl& treeCtrl );

	bool RestoreTreeSelection( CTreeCtrl& rTreeCtrl ) const;

	HTREEITEM FindSelectedItem( CTreeCtrl& rTreeCtrl, bool* pIsFullMatch = nullptr ) const;

	void SaveTopVisibleItem( const CTreeCtrl& treeCtrl );
	bool RestoreTopVisibleItem( CTreeCtrl& rTreeCtrl ) const;

	bool SaveHorizontalScrollPosition( const CTreeCtrl& treeCtrl );

	bool RestoreHorizontalScrollPosition( CTreeCtrl& rTreeCtrl ) const;
public:
	static bool IsItemExpanded( HTREEITEM hItem, const CTreeCtrl& treeCtrl );
public:
	struct CTreeItem
	{
		CTreeItem( void );
		CTreeItem( const CString& text, int sameSiblingIndex );
		CTreeItem( const CTreeCtrl& treeCtrl, HTREEITEM hItem );
		~CTreeItem();

		HTREEITEM FindInParent( const CTreeCtrl& treeCtrl, HTREEITEM hParentItem ) const;
	public:
		CString m_text;
		int m_sameSiblingIndex; // provides item unicity for multiple siblings with same name (as in DG)
	};

	struct CTreeItemPath
	{
		CTreeItemPath( void );
		~CTreeItemPath();

		int BuildPath( const CTreeCtrl& treeCtrl, HTREEITEM hItem );
		HTREEITEM FindItem( const CTreeCtrl& treeCtrl, bool* pDestIsFullMatch = nullptr ) const;
	public:
		std::vector<CTreeItem> m_elements;
	};

	struct CTreeItemPathSet
	{
		CTreeItemPathSet( void ) {}
		~CTreeItemPathSet() { Clear(); }

		void Clear( void );
	public:
		std::vector<CTreeItemPath*> m_paths;
	};
private:
	struct CTreeNode
	{
		CTreeNode( void );
		explicit CTreeNode( const CTreeItem& item );
		~CTreeNode();

		void AddExpandedChildNodes( const CTreeCtrl& treeCtrl, HTREEITEM hParentItem );
		void ExpandTreeItem( CTreeCtrl& rTreeCtrl, HTREEITEM hTargetItem ) const;

		void Clear( void );

		void TraceNode( int nestingLevel = 0 );
	public:
		CTreeItem m_item;
		std::vector<CTreeNode*> m_children;
	};

	void ClearExpandedState( void );
private:
	std::vector<CTreeNode*> m_expandedRootNodes;
	CTreeItemPathSet m_selectedItems; // for multi-selection caret comes at end
	CTreeItemPath m_topVisibleItemPath;
	int m_horzScrollPos;
	int m_clientWidth;
};


#endif // CtrlUiState_h
