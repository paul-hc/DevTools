#ifndef ReportListControl_hxx
#define ReportListControl_hxx

#include "utl/ContainerUtilities.h"


// CReportListControl template code


template< typename ObjectT >
void CReportListControl::QueryObjectsByIndex( std::vector< ObjectT* >& rObjects, const std::vector< int >& itemIndexes ) const
{
	rObjects.reserve( rObjects.size() + itemIndexes.size() );

	for ( std::vector< int >::const_iterator itIndex = itemIndexes.begin(); itIndex != itemIndexes.end(); ++itIndex )
		if ( ObjectT* pObject = GetPtrAt< ObjectT >( *itIndex ) )
			rObjects.push_back( pObject );
}

template< typename ObjectT >
bool CReportListControl::QueryObjectsWithCheckedState( std::vector< ObjectT* >& rCheckedObjects, int checkState /*= BST_CHECKED*/ ) const
{
	for ( UINT i = 0, count = GetItemCount(); i != count; ++i )
		if ( GetCheckState( i ) == checkState )
			rCheckedObjects.push_back( GetPtrAt< ObjectT >( i ) );

	return !rCheckedObjects.empty();
}

template< typename ObjectT >
void CReportListControl::SetObjectsCheckedState( const std::vector< ObjectT* >* pObjects, int checkState /*= BST_CHECKED*/, bool uncheckOthers /*= true*/ )
{
	for ( UINT i = 0, count = GetItemCount(); i != count; ++i )
		if ( NULL == pObjects || utl::Contains( *pObjects, GetPtrAt< ObjectT >( i ) ) )
			ModifyCheckState( i, checkState );
		else if ( uncheckOthers )
			ModifyCheckState( i, BST_UNCHECKED );
}


template< typename ObjectT >
ObjectT* CReportListControl::GetSelected( void ) const
{
	ASSERT( !IsMultiSelectionList() );
	int selIndex = GetCurSel();
	return selIndex != -1 ? GetPtrAt< ObjectT >( selIndex ) : NULL;
}

template< typename ObjectT >
bool CReportListControl::QuerySelectionAs( std::vector< ObjectT* >& rSelObjects ) const
{
	std::vector< int > selIndexes;
	if ( !GetSelection( selIndexes ) )
		return false;

	rSelObjects.clear();
	QueryObjectsByIndex( rSelObjects, selIndexes );

	return !rSelObjects.empty() && rSelObjects.size() == selIndexes.size();		// homogenous selection?
}

template< typename ObjectT >
void CReportListControl::SelectObjects( const std::vector< ObjectT* >& objects )
{
	std::vector< int > selIndexes;
	for ( std::vector< ObjectT* >::const_iterator itObject = objects.begin(); itObject != objects.end(); ++itObject )
	{
		int foundIndex = FindItemIndex( *itObject );
		if ( foundIndex != -1 )
			selIndexes.push_back( foundIndex );
	}

	if ( !selIndexes.empty() )
		SetSelection( selIndexes, selIndexes.front() );
	else
		ClearSelection();
}

template< typename ObjectT >
void CReportListControl::QueryGroupItems( std::vector< ObjectT* >& rObjectItems, int groupId ) const
{
	REQUIRE( IsGroupViewEnabled() );

	typedef std::multimap< int, TRowKey >::const_iterator GroupMapIterator;
	std::pair< GroupMapIterator, GroupMapIterator > itPair = m_groupIdToItemsMap.equal_range( groupId );

	for ( GroupMapIterator it = itPair.first; it != itPair.second; ++it )
		rObjectItems.push_back( AsPtr< ObjectT >( it->second ) );
}

template< typename MatchFunc >
void CReportListControl::SetupDiffColumnPair( TColumn srcColumn, TColumn destColumn, MatchFunc getMatchFunc )
{
	m_diffColumnPairs.push_back( CDiffColumnPair( srcColumn, destColumn ) );

	stdext::hash_map< TRowKey, str::TMatchSequence >& rRowSequences = m_diffColumnPairs.back().m_rowSequences;

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


#endif // ReportListControl_hxx
