
#include "pch.h"
#include "CtrlUiState.h"
#include "utl/ContainerOwnership.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CListBoxUiState implementation

void CListBoxUiState::Restore( void )
{
	ASSERT( ::IsWindow( m_rListBox ) );

	if ( m_rListBox.GetCount() > 0 )
	{
		if ( m_caretIndex != LB_ERR )
			m_rListBox.SetCaretIndex( m_caretIndex, FALSE );

		if ( m_topVisibleIndex != LB_ERR )
			m_rListBox.SetTopIndex( m_topVisibleIndex );
	}
}


// CTreeCtrlUiState implementation

CTreeCtrlUiState::CTreeCtrlUiState( void )
	: m_horzScrollPos( -1 )
	, m_clientWidth( 0 )
{
}

CTreeCtrlUiState::~CTreeCtrlUiState()
{
	ClearExpandedState();
}

void CTreeCtrlUiState::SaveVisualState( const CTreeCtrl& treeCtrl )
{
	SaveExpandingState( treeCtrl );
	SaveSelection( treeCtrl );
	SaveTopVisibleItem( treeCtrl );
	SaveHorizontalScrollPosition( treeCtrl );
}

void CTreeCtrlUiState::RestoreVisualState( CTreeCtrl& rTreeCtrl ) const
{
	RestoreExpandingState( rTreeCtrl );
	RestoreTreeSelection( rTreeCtrl );
	RestoreTopVisibleItem( rTreeCtrl );
	RestoreHorizontalScrollPosition( rTreeCtrl );
}

void CTreeCtrlUiState::SaveExpandingState( const CTreeCtrl& treeCtrl )
{
	ClearExpandedState();

	for ( HTREEITEM hRootItem = treeCtrl.GetChildItem( TVGN_ROOT ); hRootItem != nullptr;
		  hRootItem = treeCtrl.GetNextSiblingItem( hRootItem ) )
		if ( IsItemExpanded( hRootItem, treeCtrl ) )
		{
			CTreeNode* pExpandedNode = new CTreeNode( CTreeItem( treeCtrl, hRootItem ) );

			pExpandedNode->AddExpandedChildNodes( treeCtrl, hRootItem );
			m_expandedRootNodes.push_back( pExpandedNode );

//			pExpandedNode->TraceNode();
		}
}

bool CTreeCtrlUiState::SaveHorizontalScrollPosition( const CTreeCtrl& treeCtrl )
{
	CRect clientRect;
	treeCtrl.GetClientRect( &clientRect );

	m_horzScrollPos = -1;
	m_clientWidth = clientRect.Width();
	if ( !( treeCtrl.GetStyle() & WS_HSCROLL ) )
		return false;

	m_horzScrollPos = treeCtrl.GetScrollPos( SB_HORZ );
	return true;
}

bool CTreeCtrlUiState::RestoreHorizontalScrollPosition( CTreeCtrl& rTreeCtrl ) const
{
	if ( m_horzScrollPos == -1 )
		return false; // no horizontal scroll position stored

	CRect clientRect;
	rTreeCtrl.GetClientRect( &clientRect );

	if ( m_clientWidth != clientRect.Width() )
		return false; // stored horizontal position incompatible with current width

	if ( !HasFlag( rTreeCtrl.GetStyle(), WS_HSCROLL ) )
		return false; // no scroll bar

	rTreeCtrl.SetScrollPos( SB_HORZ, m_horzScrollPos );
	return true;
}

void CTreeCtrlUiState::RestoreExpandingState( CTreeCtrl& rTreeCtrl ) const
{
	for ( std::vector<CTreeNode*>::const_iterator itRootNode = m_expandedRootNodes.begin();
		  itRootNode != m_expandedRootNodes.end(); ++itRootNode )
		if ( HTREEITEM hFoundRoot = ( *itRootNode )->m_item.FindInParent( rTreeCtrl, TVGN_ROOT ) )
			( *itRootNode )->ExpandTreeItem( rTreeCtrl, hFoundRoot ); // restore the expansion state
}

void CTreeCtrlUiState::ClearExpandedState( void )
{
	std::for_each( m_expandedRootNodes.begin(), m_expandedRootNodes.end(), func::Delete() );
	m_expandedRootNodes.clear();
}

void CTreeCtrlUiState::SaveSelection( const CTreeCtrl& treeCtrl )
{	// single selection
	m_selectedItems.Clear();

	if ( HTREEITEM hSelected = treeCtrl.GetSelectedItem() )
	{
		CTreeItemPath* pSelItemPath = new CTreeItemPath();

		pSelItemPath->BuildPath( treeCtrl, hSelected );
		m_selectedItems.m_paths.push_back( pSelItemPath );
	}
}

bool CTreeCtrlUiState::RestoreTreeSelection( CTreeCtrl& rTreeCtrl ) const
{
	bool isFullMatch = false;

	if ( !m_selectedItems.m_paths.empty() )
	{
		const CTreeItemPath* pSelItemPath = m_selectedItems.m_paths.front();

		if ( HTREEITEM hSelected = pSelItemPath->FindItem( rTreeCtrl, &isFullMatch ) )
			rTreeCtrl.SelectItem( hSelected ); // restore the selection
	}

	return isFullMatch;
}

bool CTreeCtrlUiState::IsItemExpanded( HTREEITEM hItem, const CTreeCtrl& treeCtrl )
{
	ASSERT_PTR( hItem );
	return ( treeCtrl.GetItemState( hItem, TVIS_EXPANDED ) & TVIS_EXPANDED ) != 0;
}


HTREEITEM CTreeCtrlUiState::FindSelectedItem( CTreeCtrl& rTreeCtrl, bool* pIsFullMatch /*= nullptr*/ ) const
{
	if ( m_selectedItems.m_paths.empty() )
		return nullptr;
	return m_selectedItems.m_paths.back()->FindItem( rTreeCtrl, pIsFullMatch );
}

void CTreeCtrlUiState::SaveTopVisibleItem( const CTreeCtrl& treeCtrl )
{
	m_topVisibleItemPath.BuildPath( treeCtrl, treeCtrl.GetFirstVisibleItem() );
}

bool CTreeCtrlUiState::RestoreTopVisibleItem( CTreeCtrl& rTreeCtrl ) const
{
	bool isFullMatch;
	if ( HTREEITEM hTopVisibleItem = m_topVisibleItemPath.FindItem( rTreeCtrl, &isFullMatch ) )
		rTreeCtrl.Select( hTopVisibleItem, TVGN_FIRSTVISIBLE ); // restore the selection

	return isFullMatch;
}


// CTreeCtrlUiState::CTreeItem class

CTreeCtrlUiState::CTreeItem::CTreeItem( void )
	: m_text()
	, m_sameSiblingIndex( 0 )
{
}

CTreeCtrlUiState::CTreeItem::CTreeItem( const CString& text, int sameSiblingIndex )
	: m_text( text )
	, m_sameSiblingIndex( sameSiblingIndex )
{
}

CTreeCtrlUiState::CTreeItem::CTreeItem( const CTreeCtrl& treeCtrl, HTREEITEM hItem )
	: m_text( treeCtrl.GetItemText( hItem ) )
	, m_sameSiblingIndex( 0 )
{
	HTREEITEM hSiblingItem = hItem;

	while ( ( hSiblingItem = treeCtrl.GetPrevSiblingItem( hSiblingItem ) ) != nullptr &&
			treeCtrl.GetItemText( hSiblingItem ) == m_text )
		++m_sameSiblingIndex;
}

CTreeCtrlUiState::CTreeItem::~CTreeItem()
{
}

HTREEITEM CTreeCtrlUiState::CTreeItem::FindInParent( const CTreeCtrl& treeCtrl, HTREEITEM hParentItem ) const
{
	HTREEITEM hLastMatchItem = nullptr;
	int sameSiblingIndex = 0;

	for ( HTREEITEM hItem = treeCtrl.GetChildItem( hParentItem ); hItem != nullptr;
		  hItem = treeCtrl.GetNextSiblingItem( hItem ) )
		if ( treeCtrl.GetItemText( hItem ) == m_text )
			if ( sameSiblingIndex++ == m_sameSiblingIndex )
				return hItem;				// Text & sibling index match
			else if ( sameSiblingIndex > m_sameSiblingIndex )
				return hLastMatchItem;			// Last text match but sibling index mismatch
			else
				hLastMatchItem = hItem;	// Make sure we return at least one partial match

	return hLastMatchItem;
}


// CTreeCtrlUiState::CTreeItemPath class

CTreeCtrlUiState::CTreeItemPath::CTreeItemPath( void )
{
}

CTreeCtrlUiState::CTreeItemPath::~CTreeItemPath()
{
}

int CTreeCtrlUiState::CTreeItemPath::BuildPath( const CTreeCtrl& treeCtrl, HTREEITEM hItem )
{
	m_elements.clear();

	// save the selected item path (if any)
	for ( HTREEITEM hPathItem = hItem; hPathItem != nullptr; hPathItem = treeCtrl.GetParentItem( hPathItem ) )
		m_elements.insert( m_elements.begin(), CTreeItem( treeCtrl, hPathItem ) );

	return (int)m_elements.size();
}

HTREEITEM CTreeCtrlUiState::CTreeItemPath::FindItem( const CTreeCtrl& treeCtrl, bool* pDestIsFullMatch /*= nullptr*/ ) const
{
	HTREEITEM hPathItem = TVGN_ROOT;

	if ( pDestIsFullMatch != nullptr )
		*pDestIsFullMatch = true;

	for ( std::vector<CTreeItem>::const_iterator itElement = m_elements.begin();
		  itElement != m_elements.end(); ++itElement )
		if ( HTREEITEM hChildItem = ( *itElement ).FindInParent( treeCtrl, hPathItem ) )
			hPathItem = hChildItem; // recover as much as possible from original selected item path
		else
		{
			if ( pDestIsFullMatch != nullptr )
				*pDestIsFullMatch = false;
			break;
		}

	return hPathItem;
}


// CTreeCtrlUiState::CTreeItemPathSet class

void CTreeCtrlUiState::CTreeItemPathSet::Clear( void )
{
	std::for_each( m_paths.begin(), m_paths.end(), func::Delete() );
	m_paths.clear();
}


// CTreeCtrlUiState::CTreeNode class

CTreeCtrlUiState::CTreeNode::CTreeNode( void )
	: m_item()
{
}

CTreeCtrlUiState::CTreeNode::CTreeNode( const CTreeItem& item )
	: m_item( item )
{
}

CTreeCtrlUiState::CTreeNode::~CTreeNode()
{
	Clear();
}

void CTreeCtrlUiState::CTreeNode::Clear( void )
{
	utl::ClearOwningContainer( m_children );
}

void CTreeCtrlUiState::CTreeNode::AddExpandedChildNodes( const CTreeCtrl& treeCtrl, HTREEITEM hParentItem )
{
	for ( HTREEITEM hChildItem = treeCtrl.GetChildItem( hParentItem ); hChildItem != nullptr;
		  hChildItem = treeCtrl.GetNextSiblingItem( hChildItem ) )
		if ( IsItemExpanded( hChildItem, treeCtrl ) )
		{
			CTreeNode* pExpandedChild = new CTreeNode( CTreeItem( treeCtrl, hChildItem ) );

			m_children.push_back( pExpandedChild );
			pExpandedChild->AddExpandedChildNodes( treeCtrl, hChildItem );
		}
}

void CTreeCtrlUiState::CTreeNode::ExpandTreeItem( CTreeCtrl& rTreeCtrl, HTREEITEM hTargetItem ) const
{
	ASSERT_PTR( hTargetItem );

	rTreeCtrl.Expand( hTargetItem, TVE_EXPAND );

	for ( std::vector<CTreeNode*>::const_iterator itExpandedChild = m_children.begin(); itExpandedChild != m_children.end(); ++itExpandedChild )
	{
		HTREEITEM hMatchingChildItem = ( *itExpandedChild )->m_item.FindInParent( rTreeCtrl, hTargetItem );

		if ( hMatchingChildItem != nullptr )
			( *itExpandedChild )->ExpandTreeItem( rTreeCtrl, hMatchingChildItem );
		else
			TRACE( _T(" * Couldn't restore expanded state for child node: '%s':%d\n"),
				   ( *itExpandedChild )->m_item.m_text.GetString(),
				   ( *itExpandedChild )->m_item.m_sameSiblingIndex );
	}
}

void CTreeCtrlUiState::CTreeNode::TraceNode( int nestingLevel /*= 0*/ )
{
#ifdef _DEBUG
	if ( this == nullptr )
		return; // must be safe for NULL (root ptr)

	std::tstring leadingSpaces( size_t( nestingLevel * 2 ), _T(' ') );

	TRACE( _T("%s'%s':%d\n"), leadingSpaces.c_str(), (LPCTSTR)m_item.m_text, m_item.m_sameSiblingIndex );

	for ( std::vector<CTreeNode*>::const_iterator itNode = m_children.begin(); itNode != m_children.end(); ++itNode )
		( *itNode )->TraceNode( nestingLevel + 1 );

#else
	nestingLevel;
#endif // _DEBUG
}
