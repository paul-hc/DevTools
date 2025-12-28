#ifndef SelectionData_hxx
#define SelectionData_hxx
#pragma once

#include "SelectionData.h"
#include "ReportListControl.h"
#include "TextListEditor.h"
#include "utl/Algorithms.h"


namespace ui
{
	template< typename ObjectT >
	ObjectT* CSelectionData<ObjectT>::GetCurSelItem( void ) const
	{
		if ( m_pCaretItem != nullptr )
			return m_pCaretItem;
		else if ( !m_selItems.empty() )
			return m_selItems.front();

		return nullptr;
	}

	template< typename ObjectT >
	bool CSelectionData<ObjectT>::SetCurSelItem( ObjectT* pCurSelItem, bool exclusive /*= true*/ )
	{
		CSelectionData<ObjectT> oldSelData = *this;

		SetCaretItem( pCurSelItem );

		if ( exclusive )
			m_selItems.clear();

		if ( pCurSelItem != nullptr )
			m_selItems.push_back( pCurSelItem );

		return *this != oldSelData;
	}

	template< typename ObjectT >
	bool CSelectionData<ObjectT>::ReadList( const CReportListControl* pListCtrl )
	{
		ASSERT_PTR( pListCtrl->GetSafeHwnd() );

		const CSelectionData oldSelItems = *this;

		pListCtrl->QuerySelectionAs( m_selItems );
		m_pCaretItem = pListCtrl->GetCaretAs<ObjectT>();
		m_pTopVisibleItem = pListCtrl->GetTopObjectAs<ObjectT>();

		return *this != oldSelItems;		// true if changed
	}

	template< typename ObjectT >
	bool CSelectionData<ObjectT>::UpdateList( CReportListControl* pListCtrl ) const
	{
		ASSERT_PTR( pListCtrl->GetSafeHwnd() );

		if ( 0 == pListCtrl->GetItemCount() )
			return false;

		CSelectionData currSelItems;
		currSelItems.ReadList( pListCtrl );

		if ( *this == currSelItems )
			return false;		// same state, nothing to change

		if ( pListCtrl->IsMultiSelectionList() )
			pListCtrl->SelectObjects( m_selItems );
		else
			if ( !m_selItems.empty() )
				pListCtrl->Select( m_selItems.front() );
			else
				pListCtrl->ClearSelection();

		if ( m_pCaretItem != pListCtrl->GetCaretAs<ObjectT>() )
			pListCtrl->SetCaretObject( m_pCaretItem );

		if ( m_useTopItem && m_pTopVisibleItem != nullptr )
			pListCtrl->SetTopObject( m_pTopVisibleItem );

		return true;	// selection/caret changed
	}


	template< typename ObjectT >
	bool CSelectionData<ObjectT>::ReadEdit( const CTextListEditor* pEdit )
	{
		ASSERT( pEdit->IsMultiLine() );

		const CSelectionData oldSelItems = *this;

		pEdit->QuerySelItems( m_selItems );
		m_pCaretItem = pEdit->GetCaretItem<ObjectT>();
		m_pTopVisibleItem = pEdit->GetTopItem<ObjectT>();

		return *this != oldSelItems;		// true if changed
	}

	template< typename ObjectT >
	bool CSelectionData<ObjectT>::UpdateEdit( CTextListEditor* pEdit ) const
	{
		ASSERT( pEdit->IsMultiLine() );

		if ( 0 == pEdit->GetItemCount() )
			return false;

		bool changed = false;

		if ( !m_selItems.empty() )
			changed |= pEdit->SetSelItems( m_selItems );
		else if ( m_pCaretItem != nullptr )
			changed |= pEdit->SetCaretItem( m_pCaretItem, true );
		else
			changed |= pEdit->ClearSelection();

		if ( m_useTopItem && m_pTopVisibleItem != nullptr )
			changed |= pEdit->SetTopItem( m_pTopVisibleItem );

		return changed;
	}

#ifdef IS_CPP_11
	template< typename ObjectT >
	bool CSelectionData<ObjectT>::SortListSelItems( const CReportListControl* pListCtrl )
	{	// reorder selected items in list index order - not used, just for illustration (since pListCtrl->QuerySelectionAs() returns a sorted selection)
		ASSERT_PTR( pListCtrl->GetSafeHwnd() );

		typedef std::pair<ObjectT*, size_t> TItemIndexPair;
		std::vector<TItemIndexPair> selPairs;

		utl::Assign( selPairs, m_selItems, [pListCtrl]( ObjectT* pObject ) { return std::make_pair( pObject, pListCtrl->FindItemIndex( pObject ) ); } );
		std::sort( selPairs.begin(), selPairs.end(), []( const TItemIndexPair& leftPair, const TItemIndexPair& rightPair ) { return leftPair.second < rightPair.second; } );	// sort by item index

		std::vector<ObjectT*> newSelItems;
		utl::Assign( newSelItems, selPairs, []( const TItemIndexPair& itemPair ) { return itemPair.first; } );

		return utl::ModifyValue( m_selItems, newSelItems );
	}
#endif //IS_CPP_11

}


#endif // SelectionData_hxx
