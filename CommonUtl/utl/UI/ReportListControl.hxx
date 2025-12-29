#ifndef ReportListControl_hxx
#define ReportListControl_hxx

#include "utl/Algorithms.h"
#include "ReportListControl.h"


// CReportListControl template code

template< typename ObjectT >
void CReportListControl::QueryObjectsSequence( std::vector<ObjectT*>& rObjects ) const
{
	UINT count = GetItemCount();
	rObjects.clear();
	rObjects.reserve( count );

	for ( UINT i = 0; i != count; ++i )
		rObjects.push_back( GetPtrAt<ObjectT>( i ) );
}

template< typename PosT, typename ObjectT >
void CReportListControl::QueryItemIndexes( std::vector<PosT>& rIndexes, const std::vector<ObjectT*>& objects ) const
{
#ifdef IS_CPP_11
	std::transform( objects.begin(), objects.end(), std::back_inserter( rIndexes ), [this]( const ObjectT* pObject ) { return FindItemPos<PosT>( pObject ); } );
#else
	for ( typename std::vector<ObjectT*>::const_iterator itObject = objects.begin(); itObject != objects.end(); ++itObject )
		rIndexes.push_back( FindItemPos<PosT>( *itObject ) );
#endif
}

template< typename ObjectT, typename PosT >
void CReportListControl::QueryObjectsByIndex( std::vector<ObjectT*>& rObjects, const std::vector<PosT>& itemIndexes ) const
{
#ifdef IS_CPP_11
	std::transform( itemIndexes.begin(), itemIndexes.end(), std::back_inserter( rObjects ), [this]( PosT index ) { return GetPtrAt<ObjectT>( index ); } );
#else
	for ( typename std::vector<PosT>::const_iterator itIndex = itemIndexes.begin(); itIndex != itemIndexes.end(); ++itIndex )
		rObjects.push_back( GetPtrAt<ObjectT>( *itIndex ) );
#endif
}

template< typename ObjectT >
bool CReportListControl::QueryObjectsWithCheckedState( std::vector<ObjectT*>& rCheckedObjects, int checkState /*= BST_CHECKED*/ ) const
{
	for ( UINT i = 0, count = GetItemCount(); i != count; ++i )
		if ( GetCheckState( i ) == checkState )
			rCheckedObjects.push_back( GetPtrAt<ObjectT>( i ) );

	return !rCheckedObjects.empty();
}

template< typename ObjectT >
void CReportListControl::SetObjectsCheckedState( const std::vector<ObjectT*>* pObjects, int checkState /*= BST_CHECKED*/, bool uncheckOthers /*= true*/ )
{
	for ( UINT i = 0, count = GetItemCount(); i != count; ++i )
		if ( nullptr == pObjects || utl::Contains( *pObjects, GetPtrAt<ObjectT>( i ) ) )
			ModifyCheckState( i, checkState );
		else if ( uncheckOthers )
			ModifyCheckState( i, BST_UNCHECKED );
}

template< typename ObjectT >
bool CReportListControl::QuerySelectionAs( OUT std::vector<ObjectT*>& rSelObjects,
										   OUT OPTIONAL ObjectT** ppCaretItem /*= nullptr*/, OUT OPTIONAL ObjectT** ppTopVisibleItem /*= nullptr*/ ) const
{
	std::vector<int> selIndexes;
	int caretIndex = -1, topIndex = -1;

	rSelObjects.clear();
	if ( !GetSelIndexes( selIndexes, &caretIndex, &topIndex ) )
		return false;

	QueryObjectsByIndex( rSelObjects, selIndexes );

	if ( ppCaretItem != nullptr )
		*ppCaretItem = caretIndex != -1 ? GetPtrAt<ObjectT>( caretIndex ) : nullptr;

	if ( ppCaretItem != nullptr )
		*ppTopVisibleItem = topIndex != -1 ? GetPtrAt<ObjectT>( topIndex ) : nullptr;

	return !rSelObjects.empty() && rSelObjects.size() == selIndexes.size();		// homogenous selection?
}

template< typename ObjectT >
void CReportListControl::SelectObjects( const std::vector<ObjectT*>& objects )
{
	std::vector<int> selIndexes;
	for ( typename std::vector<ObjectT*>::const_iterator itObject = objects.begin(); itObject != objects.end(); ++itObject )
	{
		int foundIndex = FindItemIndex( *itObject );
		if ( foundIndex != -1 )
			selIndexes.push_back( foundIndex );
	}

	if ( !selIndexes.empty() )
		SetSelIndexes( selIndexes, selIndexes.front() );
	else
		ClearSelection();
}

template< typename ObjectT >
void CReportListControl::QueryGroupItems( std::vector<ObjectT*>& rObjectItems, int groupId ) const
{
	REQUIRE( IsGroupViewEnabled() );

	typedef std::multimap<int, TRowKey>::const_iterator TGroupMapIterator;
	std::pair<TGroupMapIterator, TGroupMapIterator> itPair = m_groupIdToItemsMap.equal_range( groupId );

	for ( TGroupMapIterator it = itPair.first; it != itPair.second; ++it )
		rObjectItems.push_back( AsPtr<ObjectT>( it->second ) );
}

template< typename MatchFunc >
void CReportListControl::SetupDiffColumnPair( TColumn srcColumn, TColumn destColumn, MatchFunc getMatchFunc )
{
	m_diffColumnPairs.push_back( CDiffColumnPair( srcColumn, destColumn ) );

	std::unordered_map<TRowKey, str::TMatchSequence>& rRowSequences = m_diffColumnPairs.back().m_rowSequences;

	for ( int index = 0, itemCount = GetItemCount(); index != itemCount; ++index )
	{
		str::TMatchSequence& rMatchSequence = rRowSequences[ MakeRowKeyAt( index ) ];

		rMatchSequence.Init( GetItemText( index, srcColumn ).GetString(), GetItemText( index, destColumn ).GetString(), getMatchFunc );

		// Replace with LPSTR_TEXTCALLBACK to allow custom draw without default sub-item draw interference (on CDDS_ITEMPOSTPAINT | CDDS_SUBITEM).
		// Sub-item text is still accessible with GetItemText() method.
		//
		VERIFY( SetItemText( index, srcColumn, LPSTR_TEXTCALLBACK ) );
		VERIFY( SetItemText( index, destColumn, LPSTR_TEXTCALLBACK ) );
	}
}

template< typename MatchFunc >
void CReportListControl::UpdateItemDiffColumnDest( int index, TColumn destColumn, MatchFunc getMatchFunc )
{	// call after Dest column text was updated for a given item
	CDiffColumnPair* pDiffColumnPair = const_cast<CReportListControl::CDiffColumnPair*>( FindDiffColumnPair( destColumn ) );
	ASSERT_PTR( pDiffColumnPair );
	ASSERT( pDiffColumnPair->m_destColumn == destColumn );

	std::unordered_map<TRowKey, str::TMatchSequence>& rRowSequences = pDiffColumnPair->m_rowSequences;
	str::TMatchSequence& rMatchSequence = rRowSequences[ MakeRowKeyAt( index ) ];

	rMatchSequence.UpdateDestText( GetItemText( index, destColumn ).GetString(), getMatchFunc );

	// replace with LPSTR_TEXTCALLBACK to allow custom draw without default sub-item draw interference (on CDDS_ITEMPOSTPAINT | CDDS_SUBITEM)
	VERIFY( SetItemText( index, destColumn, LPSTR_TEXTCALLBACK ) );
}


#endif // ReportListControl_hxx
