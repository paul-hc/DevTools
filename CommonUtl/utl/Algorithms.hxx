#ifndef Algorithms_hxx
#define Algorithms_hxx
#pragma once

#include <set>
#include "Algorithms.h"
#include "ComparePredicates.h"


namespace utl
{
	template< typename IteratorT, typename KeyT, typename ToKeyFunc >
	IteratorT BinaryFind( IteratorT itStart, IteratorT itEnd, const KeyT& key, ToKeyFunc toKeyFunc )
	{
		IteratorT itFound = std::lower_bound( itStart, itEnd, key, pred::MakeLessKey( toKeyFunc ) );
		if ( itFound != itEnd )						// loose match?
			if ( toKeyFunc( *itFound ) == key )		// exact match?
				return itFound;

		return itEnd;
	}


	template< typename PredT, typename ContainerT >
	size_t Uniquify( ContainerT& rItems, ContainerT* pRemovedDups /*= static_cast<ContainerT*>( NULL )*/ )
	{
		REQUIRE( PredT::IsBoolPred() );

		typedef typename ContainerT::value_type TValue;

		std::set<TValue, PredT> uniqueIndex;
		ContainerT tempItems;
		size_t duplicateCount = 0;

		tempItems.reserve( rItems.size() );
		for ( typename ContainerT::const_iterator itItem = rItems.begin(), itEnd = rItems.end(); itItem != itEnd; ++itItem )
			if ( uniqueIndex.insert( *itItem ).second )		// item is unique?
				tempItems.push_back( *itItem );
			else
			{
				++duplicateCount;

				if ( pRemovedDups != NULL )
					pRemovedDups->insert( pRemovedDups->end(), *itItem );	// for owning container of pointers: allow client to delete the removed duplicates
			}

		rItems.swap( tempItems );
		return duplicateCount;
	}
}


#endif // Algorithms_hxx
