#ifndef ContainerUtilities_h
#define ContainerUtilities_h
#pragma once

#include <xhash>


namespace utl
{
	template< typename Container >
	inline size_t ByteSize( const Container& items ) { return items.size() * sizeof( Container::value_type ); }		// for vector, deque

	template< typename InputIterator, class OutputIterT >
	OutputIterT Copy( InputIterator itFirst, InputIterator itLast, OutputIterT itDest )
	{
		// same as std::copy without the warning
		for ( ; itFirst != itLast; ++itDest, ++itFirst )
			*itDest = *itFirst;

		return itDest;
	}

	template< typename SrcType >
	inline void* WriteBuffer( void* pBuffer, const SrcType* pSrc, size_t size = 1 )
	{
		memcpy( pBuffer, pSrc, size * sizeof( SrcType ) );
		return reinterpret_cast< SrcType* >( pBuffer ) + size;				// pointer arithmetic: next insert position in the DEST buffer
	}

	template< typename DestType >
	inline const void* ReadBuffer( DestType* pDest, const void* pBuffer, size_t size = 1 )
	{
		memcpy( pDest, pBuffer, size * sizeof( DestType ) );
		return reinterpret_cast< const DestType* >( pBuffer ) + size;		// pointer arithmetic: next extract position in the SRC buffer
	}
}


namespace utl
{
	template< typename T >
	void hash_combine( size_t& rSeed, const T& value )
	{
		rSeed ^= stdext::hash_value( value ) + 0x9e3779b9 + ( rSeed << 6 ) + ( rSeed >> 2 );
	}
}

namespace stdext
{
	template< typename T, typename U >
	inline size_t hash_value( const std::pair< T, U >& p )
	{
		size_t value = stdext::hash_value( p.first );
		utl::hash_combine( value, p.second );
		return value;
	}
}


#include <functional>
#include <algorithm>


namespace func
{
	/** usage:
		std::vector< Element* > elements;
		std::for_each( elements.begin(), elements.end(), func::Delete() );
	*/
	struct Delete
	{
		template< typename Type >
		void operator()( Type* pObject ) const
		{
			delete pObject;
		}
	};

	struct DeleteFirst
	{
		template< typename PairType >
		void operator()( const PairType& rPair ) const
		{
			delete const_cast< typename PairType::first_type >( rPair.first );
		}
	};


	struct DeleteSecond
	{
		template< typename PairType >
		void operator()( const PairType& rPair ) const
		{
			delete rPair.second;
		}
	};

	// std::map aliases
	typedef DeleteFirst DeleteKey;
	typedef DeleteSecond DeleteValue;
}


namespace utl
{
	// generic algoritms

	template< typename Container, typename Predicate >
	void QueryThat( std::vector< typename Container::value_type >& rSubset, const Container& objects, Predicate pred )
	{
		// additive
		for ( typename Container::const_iterator itObject = objects.begin(); itObject != objects.end(); ++itObject )
			if ( pred( *itObject ) )
				rSubset.push_back( *itObject );
	}

	template< typename Container, typename Predicate >
	inline typename Container::value_type Find( const Container& objects, Predicate pred )
	{
		typename Container::const_iterator itFound = std::find_if( objects.begin(), objects.end(), pred );
		if ( itFound == objects.end() )
			return typename Container::value_type( 0 );
		return *itFound;
	}


	template< typename Container, typename Predicate >
	inline bool Any( const Container& objects, Predicate pred )
	{
		return std::find_if( objects.begin(), objects.end(), pred ) != objects.end();
	}


	template< typename Iterator, typename Predicate >
	Iterator FindIfNot( Iterator itFirst, Iterator itEnd, Predicate pred )
	{	// std::find_if_not() is missing on most Unix platforms
		for ( ; itFirst != itEnd; ++itFirst )
			if ( !pred( *itFirst ) )
				break;
		return itFirst;
	}


	template< typename Container, typename Predicate >
	inline bool All( const Container& objects, Predicate pred )
	{
		return !objects.empty() && FindIfNot( objects.begin(), objects.end(), pred ) == objects.end();
	}


	template< typename Type, typename ItemType >
	inline void AddSorted( std::vector< Type >& rDest, ItemType item )
	{
		rDest.insert( std::upper_bound( rDest.begin(), rDest.end(), item ), item );
	}

	template< typename Type, typename ItemType, typename OrderPredicate >
	inline void AddSorted( std::vector< Type >& rDest, ItemType item, OrderPredicate orderPred ) // predicate version
	{
		rDest.insert( std::upper_bound( rDest.begin(), rDest.end(), item, orderPred ), item );
	}

	template< typename Type, typename Iterator, typename OrderPredicate >
	void AddSorted( std::vector< Type >& rDest, Iterator itFirst, Iterator itEnd, OrderPredicate orderPred ) // sequence with predicate version
	{
		for ( ; itFirst != itEnd; ++itFirst )
			AddSorted( rDest, *itFirst, orderPred );
	}

	template< typename Container, typename ItemType >
	inline bool AddUnique( Container& rDest, ItemType item )
	{
		if ( std::find( rDest.begin(), rDest.end(), item ) != rDest.end() )
			return false;

		rDest.push_back( item );
		return true;
	}

	template< typename Type, typename Iterator >
	inline void JoinUnique( std::vector< Type >& rDest, Iterator itStart, Iterator itEnd )
	{
		for ( ; itStart != itEnd; ++itStart )
			AddUnique( rDest, *itStart );
	}

	template< typename Container, typename ItemType >
	inline bool Contains( const Container& container, ItemType item )
	{
		return std::find( container.begin(), container.end(), item ) != container.end();
	}

	template< typename Container, typename ItemType >
	inline void PushUnique( Container& rContainer, ItemType item, size_t pos = std::tstring::npos )
	{
		ASSERT( !Contains( rContainer, item ) );
		rContainer.insert( std::tstring::npos == pos ? rContainer.end() : ( rContainer.begin() + pos ), item );
	}

	template< typename Container >
	inline void RemoveExisting( Container& rContainer, const typename Container::value_type& rItem )
	{
		typename Container::iterator itFound = std::find( rContainer.begin(), rContainer.end(), rItem );
		ASSERT( itFound != rContainer.end() );
		rContainer.erase( itFound );
	}

	template< typename Container >
	inline bool RemoveValue( Container& rContainer, const typename Container::value_type& rValue )
	{
		typename Container::iterator itFound = std::find( rContainer.begin(), rContainer.end(), rValue );

		if ( itFound == rContainer.end() )
			return false;

		rContainer.erase( itFound );
		return true;
	}

	template< typename DesiredType, typename SourceType >
	void QueryWithType( std::vector< DesiredType* >& rOutObjects, const std::vector< SourceType* >& rSource )
	{
		rOutObjects.reserve( rOutObjects.size() + rSource.size() );

		for ( typename std::vector< SourceType* >::const_iterator itSource = rSource.begin(); itSource != rSource.end(); ++itSource )
			if ( DesiredType* pDesired = dynamic_cast< DesiredType* >( *itSource ) )
				rOutObjects.push_back( pDesired );
	}

	template< typename TargetType, typename DestType, typename SourceType >
	void AddWithType( std::vector< DestType* >& rDestObjects, const std::vector< SourceType* >& rSourceObjects )
	{
		rDestObjects.reserve( rDestObjects.size() + rSourceObjects.size() );

		for ( typename std::vector< SourceType* >::const_iterator itObject = rSourceObjects.begin();
			  itObject != rSourceObjects.end(); ++itObject )
			if ( is_a< TargetType >( *itObject ) )
				rDestObjects.push_back( checked_static_cast< DestType* >( *itObject ) );
	}

	template< typename TargetType, typename DestType, typename SourceType >
	void AddWithoutType( std::vector< DestType* >& rDestObjects, const std::vector< SourceType* >& rSourceObjects )
	{
		rDestObjects.reserve( rDestObjects.size() + rSourceObjects.size() );

		for ( typename std::vector< SourceType* >::const_iterator itObject = rSourceObjects.begin();
			  itObject != rSourceObjects.end(); ++itObject )
			if ( !is_a< TargetType >( *itObject ) )
				rDestObjects.push_back( checked_static_cast< DestType* >( *itObject ) );
	}

	template< typename ToRemoveType, typename ObjectType >
	size_t RemoveWithType( std::vector< ObjectType* >& rObjects )
	{
		size_t removedCount = 0;

		for ( typename std::vector< ObjectType* >::iterator it = rObjects.begin(); it != rObjects.end(); )
			if ( is_a< ToRemoveType >( *it ) )
			{
				it = rObjects.erase( it );
				++removedCount;
			}
			else
				++it;

		return removedCount;
	}

	template< typename PreserveType, typename ObjectType >
	size_t RemoveNotType( std::vector< ObjectType* >& rObjects )
	{
		size_t removedCount = 0;

		for ( typename std::vector< ObjectType* >::iterator it = rObjects.begin(); it != rObjects.end(); )
			if ( is_a< PreserveType >( *it ) )
				++it;
			else
			{
				it = rObjects.erase( it );
				++removedCount;
			}

		return removedCount;
	}

	template< typename Type >
	bool MoveAtEnd( std::vector< Type >& rOutVector, const Type& value )
	{
		if ( rOutVector.empty() )
			return false;
		else if ( rOutVector.back() != value ) // not already at the end
		{
			typename std::vector< Type >::iterator itFound = std::find( rOutVector.begin(), rOutVector.end(), value );

			if ( itFound == rOutVector.end() )
				return false; // value not found

			rOutVector.erase( itFound );
			rOutVector.push_back( value );
		}

		return true;
	}

	/**
		algorithms
	*/

	// find first satisfying binary predicate equalPred
	template< typename Iterator, typename ValueType, typename EqualBinaryPred >
	inline Iterator FindIfEqual( Iterator iter, Iterator itEnd, const ValueType& value, EqualBinaryPred equalPred )
	{
		for ( ; iter != itEnd; ++iter )
			if ( equalPred( *iter, value ) )
				break;

		return iter;
	}


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

	template< typename Type >
	inline bool SameContents( const std::vector< Type >& left, const std::vector< Type >& right )
	{
		return SameContents( left.begin(), left.end(), right.begin(), right.end() );
	}


	// returns true if containers have the same items, eventually in different order (predicate version)
	template< typename Type, typename EqualBinaryPred >
	bool SameContents( const std::vector< Type >& left, const std::vector< Type >& right, EqualBinaryPred equalPred )
	{
		if ( left.size() != right.size() )
			return false;

		for ( size_t i = 0; i != left.size(); ++i )
			if ( !equalPred( left[ i ], right[ i ] ) ) // try by index
				if ( utl::FindIfEqual( right.begin(), right.end(), left[ i ], equalPred ) == right.end() )
					return false;

		return true;
	}


	template< typename DiffType, typename Iterator >
	inline DiffType Distance( Iterator itFirst, Iterator itLast )
	{
		return static_cast< DiffType >( std::distance( itFirst, itLast ) );
	}

	template< typename PosType, typename Iterator, typename ValueType >
	inline PosType FindPos( Iterator itStart, Iterator itEnd, const ValueType& rValue )
	{
		Iterator itFound = std::find( itStart, itEnd, rValue );
		return itFound != itEnd ? static_cast< PosType >( itFound - itStart ) : PosType( -1 );
	}

	template< typename PosType, typename Container, typename ValueType >
	inline PosType FindPos( const Container& container, const ValueType& rValue )
	{
		return FindPos< PosType >( container.begin(), container.end(), rValue );
	}


	template< typename PosType, typename Iterator, typename ValueType >
	inline PosType LookupPos( Iterator itStart, Iterator itEnd, const ValueType& rValue )
	{
		Iterator itFound = std::find( itStart, itEnd, rValue );

		ASSERT( itFound != itEnd );
		return static_cast< PosType >( itFound - itStart );
	}

	template< typename PosType, typename Container, typename ValueType >
	inline PosType LookupPos( const Container& container, const ValueType& rValue )
	{
		return LookupPos< PosType >( container.begin(), container.end(), rValue );
	}

	template< typename PosType >
	PosType CircularAdvance( PosType pos, PosType count, bool next = true )
	{
		ASSERT( pos < count );
		if ( next )
		{	// circular next
			if ( ++pos == count )
				pos = 0;
		}
		else
		{	// circular previous
			if ( --pos == PosType( -1 ) )
				pos = count - 1;
		}
		return pos;
	}

	template< typename Iterator, typename Value, typename Pred >
	Value CircularFind( Iterator itFirst, Iterator itLast, Value startValue, Pred pred )
	{
		Iterator itStart = std::find( itFirst, itLast, startValue );
		if ( itStart != itLast )
			++itStart;
		else
			itStart = itFirst;

		Iterator itMatch = std::find_if( itStart, itLast, pred );
		if ( itMatch == itLast && itStart != itFirst )
			itMatch = std::find_if( itFirst, itStart - 1, pred );

		return itMatch != itLast && pred( *itMatch ) ? *itMatch : Value( NULL );
	}


	template< typename PosType >
	bool AdvancePos( PosType& rPos, PosType count, bool circular, bool next, PosType step = 1 )
	{
		ASSERT( rPos < count );
		ASSERT( count > 0 );
		ASSERT( step > 0 );

		if ( next )
		{
			rPos += step;
			if ( rPos >= count )
				if ( circular )
					rPos = 0;
				else
				{
					rPos = count - 1;
					return false;					// overflow
				}
		}
		else
		{
			rPos -= step;
			if ( rPos < 0 || rPos >= count )		// underflow for signed/unsigned?
				if ( circular )
					rPos = count - 1;
				else
				{
					rPos = 0;
					return false;					// underflow
				}
		}
		return true;								// no overflow
	}


	template< typename IndexType, typename ObjectType >
	void QuerySubsetIndexes( std::vector< IndexType >& rIndexes,
							 const std::vector< ObjectType* >& source,
							 const std::vector< ObjectType* >& subset )
	{
		rIndexes.resize( subset.size() );

		for ( size_t i = 0; i != subset.size(); ++i )
			rIndexes[ i ] = utl::LookupPos< IndexType >( source.begin(), source.end(), subset[ i ] );
	}


	template< typename UnaryPred, typename Container >
	void RemoveDuplicates( Container& rItems, Container* pDuplicates = NULL )
	{
		Container sourceItems;
		sourceItems.swap( rItems );
		rItems.reserve( sourceItems.size() );

		for ( Container::const_iterator itItem = sourceItems.begin(); itItem != sourceItems.end(); ++itItem )
			if ( std::find_if( rItems.begin(), rItems.end(), UnaryPred( *itItem ) ) == rItems.end() )
				rItems.push_back( *itItem );
			else if ( pDuplicates != NULL )
				pDuplicates->push_back( *itItem );		// store for deletion if items are owned
	}


	template< typename Container1, typename Container2 >
	bool EmptyIntersection( const Container1& left, const Container2& right )
	{
		for ( typename Container1::const_iterator itLeft = left.begin(); itLeft != left.end(); ++itLeft )
			if ( utl::Contains( right, *itLeft ) )
				return false;

		return true;
	}


	template< typename ObjectType >
	void RemoveIntersection( std::vector< ObjectType >& rOutLeft, std::vector< ObjectType >& rOutRight )
	{
		for ( typename std::vector< ObjectType >::iterator itLeft = rOutLeft.begin(); itLeft != rOutLeft.end(); )
		{
			typename std::vector< ObjectType >::iterator itRight = std::find( rOutRight.begin(), rOutRight.end(), *itLeft );

			if ( itRight != rOutRight.end() )
			{
				itLeft = rOutLeft.erase( itLeft );
				rOutRight.erase( itRight );
			}
			else
				++itLeft;
		}
	}

	template< typename ObjectType >
	void RemoveLeftDuplicates( std::vector< ObjectType >& rOutLeft, const std::vector< ObjectType >& right )
	{
		for ( typename std::vector< ObjectType >::iterator itLeft = rOutLeft.begin(); itLeft != rOutLeft.end(); )
			if ( std::find( right.begin(), right.end(), *itLeft ) != right.end() )
				itLeft = rOutLeft.erase( itLeft );
			else
				++itLeft;
	}
}


namespace utl
{
	// vector of pairs

	template< typename Iterator, typename KeyType >
	Iterator FindPair( Iterator itStart, Iterator itEnd, const KeyType& key )
	{
		for ( ; itStart != itEnd; ++itStart )
			if ( key == itStart->first )
				break;

		return itStart;
	}
}


namespace utl
{
	template< typename Type, typename SmartPtrType >
	inline bool ResetPtr( SmartPtrType& rPtr, Type* pObject ) { rPtr.reset( pObject ); return pObject != NULL; }

	template< typename SmartPtrType >
	inline bool ResetPtr( SmartPtrType& rPtr ) { rPtr.reset( NULL ); return false; }


	/**
		object ownership helpers
	*/

	template< typename PtrContainer >
	void ClearOwningContainer( PtrContainer& rContainer )
	{
		std::for_each( rContainer.begin(), rContainer.end(), func::Delete() );
		rContainer.clear();
	}

	template< typename PtrContainer, typename ClearFunctor >
	void ClearOwningContainer( PtrContainer& rContainer, ClearFunctor clearFunctor )
	{
		std::for_each( rContainer.begin(), rContainer.end(), clearFunctor );
		rContainer.clear();
	}

	template< typename MapType >
	void ClearOwningAssocContainer( MapType& rObjectToObjectMap )
	{
		for ( typename MapType::iterator it = rObjectToObjectMap.begin(); it != rObjectToObjectMap.end(); ++it )
		{
			delete it->first;
			delete it->second;
		}

		rObjectToObjectMap.clear();
	}

	template< typename MapType >
	void ClearOwningAssocContainerKeys( MapType& rObjectToValueMap )
	{
		for ( typename MapType::iterator it = rObjectToValueMap.begin(); it != rObjectToValueMap.end(); ++it )
			delete it->first;

		rObjectToValueMap.clear();
	}

	template< typename MapType >
	void ClearOwningAssocContainerValues( MapType& rKeyToObjectMap )
	{
		for ( typename MapType::iterator it = rKeyToObjectMap.begin(); it != rKeyToObjectMap.end(); ++it )
			delete it->second;

		rKeyToObjectMap.clear();
	}
}


#endif // ContainerUtilities_h
