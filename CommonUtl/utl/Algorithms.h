#ifndef Algorithms_h
#define Algorithms_h
#pragma once

#include <algorithm>
#include <functional>


namespace utl
{
	template< typename StructT >
	inline StructT* ZeroStruct( StructT* pStruct )
	{
		ASSERT_PTR( pStruct );
		::memset( pStruct, 0, sizeof( StructT ) );
		return pStruct;
	}

	template< typename StructT >
	inline StructT* ZeroWinStruct( StructT* pStruct )		// for Win32 structures with 'cbSize' data-member
	{
		ZeroStruct( pStruct );
		pStruct->cbSize = sizeof( StructT );
		return pStruct;
	}


	template< typename ContainerT >
	inline size_t ByteSize( const ContainerT& items ) { return items.size() * sizeof( ContainerT::value_type ); }	// for vector, deque

	template< typename InputIterator, class OutputIterT >
	OutputIterT Copy( InputIterator itFirst, InputIterator itLast, OutputIterT itDest )
	{
		// same as std::copy without the warning
		for ( ; itFirst != itLast; ++itDest, ++itFirst )
			*itDest = *itFirst;

		return itDest;
	}

	template< typename SrcT >
	inline void* WriteBuffer( void* pBuffer, const SrcT* pSrc, size_t size = 1 )
	{
		memcpy( pBuffer, pSrc, size * sizeof( SrcT ) );
		return reinterpret_cast<SrcT*>( pBuffer ) + size;			// pointer arithmetic: next insert position in the DEST buffer
	}

	template< typename DestT >
	inline const void* ReadBuffer( DestT* pDest, const void* pBuffer, size_t size = 1 )
	{
		memcpy( pDest, pBuffer, size * sizeof( DestT ) );
		return reinterpret_cast<const DestT*>( pBuffer ) + size;	// pointer arithmetic: next extract position in the SRC buffer
	}
}


namespace utl
{
	template< typename ContainerT, typename FuncT >
	inline FuncT for_each( ContainerT& rObjects, FuncT func )
	{
		return std::for_each( rObjects.begin(), rObjects.end(), func );
	}

	template< typename ContainerT, typename FuncT >
	inline FuncT for_each( const ContainerT& objects, FuncT func )
	{
		return std::for_each( objects.begin(), objects.end(), func );
	}


	template< typename ContainerT, typename UnaryPred >
	void QueryThat( std::vector< typename ContainerT::value_type >& rSubset, const ContainerT& objects, UnaryPred pred )
	{
		// additive
		for ( typename ContainerT::const_iterator itObject = objects.begin(); itObject != objects.end(); ++itObject )
			if ( pred( *itObject ) )
				rSubset.push_back( *itObject );
	}


	// container bounds: works with std::list (not random iterator)

	template< typename ContainerT >
	inline typename const ContainerT::value_type& Front( const ContainerT& rItems ) { ASSERT( !rItems.empty() ); return *rItems.begin(); }

	template< typename ContainerT >
	inline typename ContainerT::value_type& Front( ContainerT& rItems ) { ASSERT( !rItems.empty() ); return *rItems.begin(); }

	template< typename ContainerT >
	inline typename const ContainerT::value_type& Back( const ContainerT& rItems ) { ASSERT( !rItems.empty() ); return *--rItems.end(); }

	template< typename ContainerT >
	inline typename ContainerT::value_type& Back( ContainerT& rItems ) { ASSERT( !rItems.empty() ); return *--rItems.end(); }
}


namespace func
{
	template< typename NumericT >
	struct GenNumSeq : public std::unary_function< void, NumericT >
	{
		GenNumSeq( NumericT initialValue = NumericT(), NumericT step = 1 ) : m_value( initialValue ), m_step( step ) {}

		NumericT operator()( void ) { NumericT value = m_value; m_value += m_step; return value; }
	private:
		NumericT m_value;
		NumericT m_step;
	};
}


namespace utl
{
	// linear search

	template< typename ContainerT, typename UnaryPred >
	inline typename ContainerT::value_type Find( const ContainerT& objects, UnaryPred pred )
	{
		typename ContainerT::const_iterator itFound = std::find_if( objects.begin(), objects.end(), pred );
		if ( itFound == objects.end() )
			return typename ContainerT::value_type( 0 );
		return *itFound;
	}


	template< typename IteratorT, typename UnaryPred >
	IteratorT FindIfNot( IteratorT itFirst, IteratorT itEnd, UnaryPred pred )
	{	// std::find_if_not() is missing on most Unix platforms
		for ( ; itFirst != itEnd; ++itFirst )
			if ( !pred( *itFirst ) )
				break;
		return itFirst;
	}


	template< typename ContainerT, typename UnaryPred >
	inline bool Any( const ContainerT& objects, UnaryPred pred )
	{
		return std::find_if( objects.begin(), objects.end(), pred ) != objects.end();
	}

	template< typename ContainerT, typename UnaryPred >
	inline bool All( const ContainerT& objects, UnaryPred pred )
	{
		return !objects.empty() && FindIfNot( objects.begin(), objects.end(), pred ) == objects.end();
	}


	template< typename ContainerT, typename ValueT >
	inline bool Contains( const ContainerT& container, const ValueT& value )
	{
		return std::find( container.begin(), container.end(), value ) != container.end();
	}

	template< typename IteratorT, typename ValueT >
	inline bool Contains( IteratorT itStart, IteratorT itEnd, const ValueT& value )
	{
		IteratorT itFound = std::find( itStart, itEnd, value );
		return itFound != itEnd;
	}

	template< typename DiffT, typename IteratorT >
	inline DiffT Distance( IteratorT itFirst, IteratorT itLast )
	{
		return static_cast<DiffT>( std::distance( itFirst, itLast ) );
	}

	template< typename IteratorT, typename ValueT >
	inline size_t FindPos( IteratorT itStart, IteratorT itEnd, const ValueT& value )
	{
		IteratorT itFound = std::find( itStart, itEnd, value );
		return itFound != itEnd ? std::distance( itStart, itFound ) : size_t( -1 );
	}

	template< typename ContainerT, typename ValueT >
	inline size_t FindPos( const ContainerT& container, const ValueT& value )
	{
		return FindPos( container.begin(), container.end(), value );
	}

	template< typename IteratorT, typename ValueT >
	inline size_t LookupPos( IteratorT itStart, IteratorT itEnd, const ValueT& value )
	{
		IteratorT itFound = std::find( itStart, itEnd, value );

		ASSERT( itFound != itEnd );
		return std::distance( itStart, itFound );
	}

	template< typename ContainerT, typename ValueT >
	inline size_t LookupPos( const ContainerT& container, const ValueT& value )
	{
		return LookupPos( container.begin(), container.end(), value );
	}

	template< typename IteratorT >
	size_t GetMatchingLength( IteratorT itStart, IteratorT itEnd )
	{
		if ( itStart != itEnd )
			for ( IteratorT itMismatch = itStart + 1; ; ++itMismatch )
				if ( itMismatch == itEnd || *itMismatch != *itStart )
					return std::distance( itStart, itMismatch );

		return 0;
	}


	// vector of pairs

	template< typename IteratorT, typename KeyT >
	IteratorT FindPair( IteratorT itStart, IteratorT itEnd, const KeyT& key )
	{
		for ( ; itStart != itEnd; ++itStart )
			if ( key == itStart->first )
				break;

		return itStart;
	}


	// find first satisfying binary predicate equalPred
	template< typename IteratorT, typename ValueT, typename BinaryPred >
	inline IteratorT FindIfEqual( IteratorT iter, IteratorT itEnd, const ValueT& value, BinaryPred equalPred )
	{
		for ( ; iter != itEnd; ++iter )
			if ( equalPred( *iter, value ) )
				break;

		return iter;
	}
}


namespace utl
{
	// binary search for ordered containers

	template< typename IteratorT, typename KeyT, typename ToKeyFunc >
	IteratorT BinaryFind( IteratorT itStart, IteratorT itEnd, const KeyT& key, ToKeyFunc toKeyFunc )
	{
		IteratorT itFound = std::lower_bound( itStart, itEnd, key, pred::MakeLessKey( toKeyFunc ) );
		if ( itFound != itEnd )						// loose match?
			if ( toKeyFunc( *itFound ) == key )		// exact match?
				return itFound;

		return itEnd;
	}

	template< typename ContainerT, typename KeyT, typename ToKeyFunc >
	inline typename ContainerT::const_iterator BinaryFind( const ContainerT& container, const KeyT& key, ToKeyFunc toKeyFunc )
	{
		return BinaryFind( container.begin(), container.end(), key, toKeyFunc );
	}

	template< typename ContainerT, typename KeyT, typename ToKeyFunc >
	inline typename ContainerT::iterator BinaryFind( ContainerT& container, const KeyT& key, ToKeyFunc toKeyFunc )
	{
		return BinaryFind( container.begin(), container.end(), key, toKeyFunc );
	}

	template< typename ContainerT, typename KeyT, typename ToKeyFunc >
	inline typename size_t BinaryFindPos( const ContainerT& container, const KeyT& key, ToKeyFunc toKeyFunc )
	{
		typename ContainerT::const_iterator itFound = BinaryFind( container.begin(), container.end(), key, toKeyFunc );
		return itFound != container.end() ? std::distance( container.begin(), itFound ) : std::tstring::npos;
	}

	template< typename ContainerT, typename KeyT, typename ToKeyFunc >
	inline typename bool BinaryContains( const ContainerT& container, const KeyT& key, ToKeyFunc toKeyFunc )
	{
		return BinaryFind( container.begin(), container.end(), key, toKeyFunc ) != container.end();
	}


	// map find

	template< typename MapType >
	inline typename MapType::mapped_type FindValue( const MapType& objectMap, const typename MapType::key_type& key )
	{
		typename MapType::const_iterator itFound = objectMap.find( key );
		if ( itFound == objectMap.end() )
			return typename MapType::mapped_type( 0 );
		return itFound->second;
	}

	template< typename MapType >
	inline const typename MapType::mapped_type* FindValuePtr( const MapType& objectMap, const typename MapType::key_type& key )
	{
		typename MapType::const_iterator itFound = objectMap.find( key );
		return itFound != objectMap.end() ? &itFound->second : NULL;
	}

	template< typename MapType >
	inline typename MapType::mapped_type* FindValuePtr( MapType& rObjectMap, const typename MapType::key_type& key )
	{
		typename MapType::iterator itFound = rObjectMap.find( key );
		return itFound != rObjectMap.end() ? &itFound->second : NULL;
	}
}


namespace utl
{
	// check containers are in order

	template< typename IteratorT, typename CompareValues >
	bool IsOrdered( IteratorT itStart, IteratorT itEnd, CompareValues comparator )
	{
		if ( std::distance( itStart, itEnd ) > 2 )		// enough values for conflicting order?
		{
			IteratorT itNext = itStart;
			++itNext;						// so that it works with std::list (not a random iterator)
			pred::CompareResult cmpFirst = comparator( *itStart, *itNext );

			for ( itStart = itNext; ++itNext != itEnd; itStart = itNext )
				if ( comparator( *itStart, *itNext ) != cmpFirst )
					return false;
		}

		return true;
	}

	template< typename ContainerT, typename CompareValues >
	inline bool IsOrdered( const ContainerT& values, CompareValues comparator )
	{
		return IsOrdered( values.begin(), values.end(), comparator );
	}


	template< typename IteratorT >
	inline bool IsOrdered( IteratorT itStart, IteratorT itEnd )
	{
		return IsOrdered( itStart, itEnd, pred::CompareValue() );
	}

	template< typename ContainerT >
	inline bool IsOrdered( const ContainerT& values )
	{
		return IsOrdered( values.begin(), values.end() );
	}
}


namespace utl
{
	// copy items between containers using a conversion functor (unary)

	template< typename ContainerT, typename UnaryFunc >
	void GenerateN( ContainerT& rItems, size_t count, UnaryFunc genFunc, size_t atPos = utl::npos )
	{
		if ( utl::npos == atPos )
			atPos = rItems.size();

		rItems.insert( rItems.begin() + atPos, count, typename ContainerT::value_type() );		// append SRC count
		std::generate( rItems.begin() + atPos, rItems.begin() + atPos + count, genFunc );
	}


	template< typename InsertIteratorT, typename SrcContainerT, typename ConvertUnaryFunc >
	inline void InsertFrom( InsertIteratorT destInserter, const SrcContainerT& srcItems, ConvertUnaryFunc cvtFunc )
	{
		std::transform( srcItems.begin(), srcItems.end(), destInserter, cvtFunc );
	}


	template< typename DestContainerT, typename SrcContainerT, typename ConvertUnaryFunc >
	inline void Assign( DestContainerT& rDestItems, const SrcContainerT& srcItems, ConvertUnaryFunc cvtFunc )
	{
		rDestItems.resize( srcItems.size() );
		std::transform( srcItems.begin(), srcItems.end(), rDestItems.begin(), cvtFunc );
	}

	template< typename DestContainerT, typename SrcContainerT, typename ConvertUnaryFunc >
	inline void Append( DestContainerT& rDestItems, const SrcContainerT& srcItems, ConvertUnaryFunc cvtFunc )
	{
		size_t origDestCount = rDestItems.size();
		rDestItems.insert( rDestItems.end(), srcItems.size(), typename DestContainerT::value_type() );		// append SRC count
		std::transform( srcItems.begin(), srcItems.end(), rDestItems.begin() + origDestCount, cvtFunc );
	}

	template< typename DestContainerT, typename SrcContainerT, typename ConvertUnaryFunc >
	void Prepend( DestContainerT& rDestItems, const SrcContainerT& srcItems, ConvertUnaryFunc cvtFunc )
	{
		rDestItems.insert( rDestItems.begin(), srcItems.size(), typename DestContainerT::value_type() );		// prepend SRC count
		std::transform( srcItems.begin(), srcItems.end(), rDestItems.begin(), cvtFunc );
	}


	template< typename Type, typename ItemT >
	inline void AddSorted( std::vector< Type >& rDest, ItemT item )
	{
		rDest.insert( std::upper_bound( rDest.begin(), rDest.end(), item ), item );
	}

	template< typename Type, typename ItemT, typename OrderBinaryPred >
	inline void AddSorted( std::vector< Type >& rDest, ItemT item, OrderBinaryPred orderPred )		// predicate version
	{
		rDest.insert( std::upper_bound( rDest.begin(), rDest.end(), item, orderPred ), item );
	}

	template< typename Type, typename IteratorT, typename OrderBinaryPred >
	void AddSorted( std::vector< Type >& rDest, IteratorT itFirst, IteratorT itEnd, OrderBinaryPred orderPred )		// sequence with predicate version
	{
		for ( ; itFirst != itEnd; ++itFirst )
			AddSorted( rDest, *itFirst, orderPred );
	}

	template< typename ContainerT, typename ItemT >
	inline bool AddUnique( ContainerT& rDest, ItemT item )
	{
		if ( std::find( rDest.begin(), rDest.end(), item ) != rDest.end() )
			return false;

		rDest.push_back( item );
		return true;
	}

	template< typename ContainerT, typename IteratorT >
	size_t JoinUnique( ContainerT& rDest, IteratorT itStart, IteratorT itEnd )
	{
		size_t oldCount = rDest.size();

		for ( ; itStart != itEnd; ++itStart )
			AddUnique( rDest, *itStart );

		return rDest.size() - oldCount;		// added count
	}

	template< typename ContainerT, typename ItemT >
	inline void PushUnique( ContainerT& rContainer, ItemT item, size_t pos = std::tstring::npos )
	{
		ASSERT( !Contains( rContainer, item ) );
		rContainer.insert( std::tstring::npos == pos ? rContainer.end() : ( rContainer.begin() + pos ), item );
	}


	template< typename ContainerT, typename ValueT >
	size_t Remove( ContainerT& rItems, const ValueT& value )
	{
		typename ContainerT::iterator itRemove = std::remove( rItems.begin(), rItems.end(), value );	// doesn't actually remove, just move items to be removed at the end
		size_t removedCount = std::distance( itRemove, rItems.end() );
		rItems.erase( itRemove, rItems.end() );
		return removedCount;
	}

	template< typename ContainerT, typename UnaryPred >
	size_t RemoveIf( ContainerT& rItems, UnaryPred pred )
	{
		typename ContainerT::iterator itRemove = std::remove_if( rItems.begin(), rItems.end(), pred );		// doesn't actually remove, just move items to be removed at the end
		size_t removedCount = std::distance( itRemove, rItems.end() );
		rItems.erase( itRemove, rItems.end() );
		return removedCount;
	}


	template< typename ContainerT >
	size_t Uniquify( ContainerT& rItems )
	{
		size_t removedCount = 0;

		for ( typename ContainerT::iterator itItem = rItems.begin(), itNext = itItem, itEnd = rItems.end(); itItem != itEnd; itNext = ++itItem )
			if ( itNext++ != itEnd )
			{
				typename ContainerT::iterator itRemove = std::remove( itNext, itEnd, *itItem );		// doesn't actually remove, just move items to be removed at the end

				if ( itRemove != itEnd )
				{
					removedCount += std::distance( itRemove, itEnd );
					itEnd = rItems.erase( itRemove, itEnd );
				}
			}

		return removedCount;
	}


	template< typename PredT, typename ContainerT >
	size_t Uniquify( ContainerT& rItems, ContainerT* pRemovedDups = static_cast<ContainerT*>( NULL ) )
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


	template< typename ContainerT >
	inline void RemoveExisting( ContainerT& rContainer, const typename ContainerT::value_type& rItem )
	{
		typename ContainerT::iterator itFound = std::find( rContainer.begin(), rContainer.end(), rItem );
		ASSERT( itFound != rContainer.end() );
		rContainer.erase( itFound );
	}

	template< typename ContainerT >
	inline bool RemoveValue( ContainerT& rContainer, const typename ContainerT::value_type& value )
	{
		typename ContainerT::iterator itFound = std::find( rContainer.begin(), rContainer.end(), value );

		if ( itFound == rContainer.end() )
			return false;

		rContainer.erase( itFound );
		return true;
	}
}


namespace utl
{
	// specific type

	template< typename DesiredT, typename SourceT >
	void QueryWithType( std::vector< DesiredT* >& rOutObjects, const std::vector< SourceT* >& rSource )
	{
		rOutObjects.reserve( rOutObjects.size() + rSource.size() );

		for ( typename std::vector< SourceT* >::const_iterator itSource = rSource.begin(); itSource != rSource.end(); ++itSource )
			if ( DesiredT* pDesired = dynamic_cast<DesiredT*>( *itSource ) )
				rOutObjects.push_back( pDesired );
	}

	template< typename TargetT, typename DestT, typename SourceT >
	void AddWithType( std::vector< DestT* >& rDestObjects, const std::vector< SourceT* >& rSourceObjects )
	{
		rDestObjects.reserve( rDestObjects.size() + rSourceObjects.size() );

		for ( typename std::vector< SourceT* >::const_iterator itObject = rSourceObjects.begin(); itObject != rSourceObjects.end(); ++itObject )
			if ( is_a<TargetT>( *itObject ) )
				rDestObjects.push_back( checked_static_cast<DestT*>( *itObject ) );
	}

	template< typename TargetT, typename DestT, typename SourceT >
	void AddWithoutType( std::vector< DestT* >& rDestObjects, const std::vector< SourceT* >& rSourceObjects )
	{
		rDestObjects.reserve( rDestObjects.size() + rSourceObjects.size() );

		for ( typename std::vector< SourceT* >::const_iterator itObject = rSourceObjects.begin(); itObject != rSourceObjects.end(); ++itObject )
			if ( !is_a<TargetT>( *itObject ) )
				rDestObjects.push_back( checked_static_cast<DestT*>( *itObject ) );
	}

	template< typename ToRemoveT, typename ObjectT >
	size_t RemoveWithType( std::vector< ObjectT* >& rObjects )
	{
		size_t removedCount = 0;

		for ( typename std::vector< ObjectT* >::iterator it = rObjects.begin(); it != rObjects.end(); )
			if ( is_a<ToRemoveT>( *it ) )
			{
				it = rObjects.erase( it );
				++removedCount;
			}
			else
				++it;

		return removedCount;
	}

	template< typename PreserveT, typename ObjectT >
	size_t RemoveWithoutType( std::vector< ObjectT* >& rObjects )
	{
		size_t removedCount = 0;

		for ( typename std::vector< ObjectT* >::iterator it = rObjects.begin(); it != rObjects.end(); )
			if ( is_a<PreserveT>( *it ) )
				++it;
			else
			{
				it = rObjects.erase( it );
				++removedCount;
			}

		return removedCount;
	}
}


namespace utl
{
	// content algorithms

	// returns true if containers have the same elements, eventually in different order
	template< typename LeftIterator, typename RightIterator >
	bool SameContents( LeftIterator itLeftStart, LeftIterator itLeftEnd, RightIterator itRightStart, RightIterator itRightEnd )
	{
		if ( std::distance( itLeftStart, itLeftEnd ) != std::distance( itRightStart, itRightEnd ) )
			return false;

		for ( RightIterator itRight = itRightStart; itLeftStart != itLeftEnd; ++itLeftStart, ++itRight )
			if ( *itLeftStart != *itRight ) // try by index
				if ( std::find( itRightStart, itRightEnd, *itLeftStart ) == itRightEnd )
					return false;

		return true;
	}

	template< typename ContainerT >
	inline bool SameContents( const ContainerT& left, const ContainerT& right )
	{
		return SameContents( left.begin(), left.end(), right.begin(), right.end() );
	}


	// returns true if containers have the same items, eventually in different order (predicate version)
	template< typename Type, typename BinaryPred >
	bool SameContents( const std::vector< Type >& left, const std::vector< Type >& right, BinaryPred equalPred )
	{
		if ( left.size() != right.size() )
			return false;

		for ( size_t i = 0; i != left.size(); ++i )
			if ( !equalPred( left[ i ], right[ i ] ) ) // try by index
				if ( utl::FindIfEqual( right.begin(), right.end(), left[ i ], equalPred ) == right.end() )
					return false;

		return true;
	}


	template< typename IndexT, typename Type >
	void QuerySubSequenceFromIndexes( std::vector< Type >& rSubSequence, const std::vector< Type >& source, const std::vector< IndexT >& selIndexes )
	{
		REQUIRE( selIndexes.size() <= source.size() );

		rSubSequence.clear();
		rSubSequence.reserve( selIndexes.size() );

		for ( typename std::vector< IndexT >::const_iterator itSelIndex = selIndexes.begin(); itSelIndex != selIndexes.end(); ++itSelIndex )
			rSubSequence.push_back( source[ *itSelIndex ] );
	}

	template< typename IndexT, typename ContainerT >
	void QuerySubSequenceIndexes( std::vector< IndexT >& rIndexes, const ContainerT& source, const ContainerT& subSequence )
	{	// note: N-squared complexity
		rIndexes.clear();
		rIndexes.reserve( subSequence.size() );

		for ( typename ContainerT::const_iterator itSubItem = subSequence.begin(); itSubItem != subSequence.end(); ++itSubItem )
			rIndexes.push_back( static_cast<IndexT>( utl::LookupPos( source.begin(), source.end(), *itSubItem ) ) );
	}


	template< typename LeftContainerT, typename RightContainer2T >
	bool EmptyIntersection( const LeftContainerT& left, const RightContainer2T& right )
	{
		for ( typename LeftContainerT::const_iterator itLeft = left.begin(); itLeft != left.end(); ++itLeft )
			if ( utl::Contains( right, *itLeft ) )
				return false;

		return true;
	}


	template< typename LeftContainerT, typename RightContainer2T >
	void RemoveIntersection( LeftContainerT& rLeft, RightContainer2T& rRight )
	{
		for ( typename LeftContainerT::iterator itLeft = rLeft.begin(); itLeft != rLeft.end(); )
			if ( Remove( rRight, *itLeft ) != 0 )
				itLeft = rLeft.erase( itLeft );
			else
				++itLeft;
	}

	template< typename LeftContainerT, typename RightContainerT >
	size_t RemoveLeftDuplicates( LeftContainerT& rLeft, const RightContainerT& right )
	{
		size_t removedCount = 0;

		for ( typename LeftContainerT::iterator itLeft = rLeft.begin(); itLeft != rLeft.end(); )
			if ( std::find( right.begin(), right.end(), *itLeft ) != right.end() )
			{
				itLeft = rLeft.erase( itLeft );
				++removedCount;
			}
			else
				++itLeft;

		return removedCount;
	}
}


namespace pred
{
	template< typename ContainerT >
	struct ContainsAny
	{
		ContainsAny( const ContainerT& items ) : m_items( items ) {}

		bool operator()( const typename ContainerT::value_type& item ) const
		{
			return utl::Contains( m_items, item );
		}
	private:
		const ContainerT& m_items;
	};
}


namespace utl
{
	// position navigation

	template< typename PosT >
	PosT CircularAdvance( PosT pos, PosT count, bool forward = true )
	{
		ASSERT( pos < count );
		if ( forward )
		{	// circular next
			if ( ++pos == count )
				pos = 0;
		}
		else
		{	// circular previous
			if ( --pos == PosT( -1 ) )
				pos = count - 1;
		}
		return pos;
	}

	template< typename IteratorT, typename Value, typename UnaryPred >
	Value CircularFind( IteratorT itFirst, IteratorT itLast, Value startValue, UnaryPred pred )
	{
		IteratorT itStart = std::find( itFirst, itLast, startValue );
		if ( itStart != itLast )
			++itStart;
		else
			itStart = itFirst;

		IteratorT itMatch = std::find_if( itStart, itLast, pred );
		if ( itMatch == itLast && itStart != itFirst )
			itMatch = std::find_if( itFirst, itStart - 1, pred );

		return itMatch != itLast && pred( *itMatch ) ? *itMatch : Value( NULL );
	}


	template< typename PosT >
	bool AdvancePos( PosT& rPos, PosT count, bool wrap, bool next, PosT step = 1 )
	{
		ASSERT( rPos < count );
		ASSERT( count > 0 );
		ASSERT( step > 0 );

		if ( next )
		{
			rPos += step;
			if ( rPos >= count )
				if ( wrap )
					rPos = 0;
				else
				{
					rPos = count - 1;
					return false;				// overflow
				}
		}
		else
		{
			rPos -= step;
			if ( rPos < 0 || rPos >= count )	// underflow for signed/unsigned?
				if ( wrap )
					rPos = count - 1;
				else
				{
					rPos = 0;
					return false;				// underflow
				}
		}
		return true;							// no overflow
	}
}


#endif // Algorithms_h
