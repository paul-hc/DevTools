
#include "Algorithms.h"
#include "StdHashValue.h"
#include <unordered_set>
#include <type_traits>		// for std::hash


namespace utl
{
	template< typename ContainerT, typename SetT >
	size_t Uniquify( IN OUT ContainerT& rItems, IN OUT SetT& rUniqueSet, OUT ContainerT* pRemovedDups /*= static_cast<ContainerT*>( nullptr )*/ )
	{
		typedef typename ContainerT::value_type TValue;

		// note: we need to have a "struct std::hash<TValue>" specialized for TValue
		ContainerT uniqueItems;
		size_t duplicateCount = 0;

		uniqueItems.reserve( rItems.size() );
		for ( typename ContainerT::const_iterator itItem = rItems.begin(), itEnd = rItems.end(); itItem != itEnd; ++itItem )
			if ( rUniqueSet.insert( *itItem ).second )		// item is unique?
				uniqueItems.push_back( *itItem );
			else
			{
				++duplicateCount;

				if ( pRemovedDups != nullptr )
					pRemovedDups->insert( pRemovedDups->end(), *itItem );	// for owning container of pointers: allow client to delete the removed duplicates
			}

		rItems.swap( uniqueItems );
		return duplicateCount;
	}

	template< typename ContainerT >
	size_t Uniquify( IN OUT ContainerT& rItems, OUT ContainerT* pRemovedDups /*= static_cast<ContainerT*>( nullptr )*/ )
	{
		typedef typename ContainerT::value_type TValue;

		std::unordered_set<TValue> uniqueSet;		// using a standard hash set
		return Uniquify( rItems, uniqueSet, pRemovedDups );
	}

	template< typename HasherT, typename KeyEqualT, typename ContainerT >
	size_t Uniquify( IN OUT ContainerT& rItems, OUT ContainerT* pRemovedDups /*= static_cast<ContainerT*>( nullptr )*/ )
	{
		typedef typename ContainerT::value_type TValue;

		std::unordered_set<TValue, HasherT, KeyEqualT> uniqueSet;		// using a custom hash set
		return Uniquify( rItems, uniqueSet, pRemovedDups );
	}
}
