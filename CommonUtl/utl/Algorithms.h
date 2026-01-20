#ifndef Algorithms_h
#define Algorithms_h
#pragma once

#include <algorithm>
#include <functional>
#include <iterator>			// std::inserter, std::back_inserter
#include "Algorithms_fwd.h"


namespace utl
{
	template< typename ContainerT >
	inline size_t ByteSize( const ContainerT& items ) { return items.size() * sizeof( ContainerT::value_type ); }	// for vector, deque

	template< typename InputIterator, class OutputIterT >
	OutputIterT Copy( InputIterator itFirst, InputIterator itLast, OUT OutputIterT itDest )
	{
		// same as std::copy without the warning
		for ( ; itFirst != itLast; ++itDest, ++itFirst )
			*itDest = *itFirst;

		return itDest;
	}

	template< typename SrcT >
	inline void* WriteBuffer( OUT void* pBuffer, const SrcT* pSrc, size_t size = 1 )
	{
		memcpy( pBuffer, pSrc, size * sizeof( SrcT ) );
		return reinterpret_cast<SrcT*>( pBuffer ) + size;			// pointer arithmetic: next insert position in the DEST buffer
	}

	template< typename DestT >
	inline const void* ReadBuffer( OUT DestT* pDest, const void* pBuffer, size_t size = 1 )
	{
		memcpy( pDest, pBuffer, size * sizeof( DestT ) );
		return reinterpret_cast<const DestT*>( pBuffer ) + size;	// pointer arithmetic: next extract position in the SRC buffer
	}
}


namespace func
{
	template< typename NumericT >
	struct GenNumSeq : public std::unary_function<void, NumericT>
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
	enum PredEval { TruePred, NegatePred };		// allow UTL algorithms to optionally negate the predicate


	// linear search

	template< typename IteratorT, typename UnaryPredT >
	IteratorT FindIf( IteratorT itFirst, IteratorT itEnd, UnaryPredT pred, PredEval predEval = utl::TruePred )
	{
		for ( ; itFirst != itEnd; ++itFirst )
			if ( pred( *itFirst ) != ( NegatePred == predEval ) )
				break;

		return itFirst;
	}

	template< typename ContainerT, typename UnaryPredT >
	inline bool ContainsIf( const ContainerT& items, UnaryPredT pred, PredEval predEval = utl::TruePred )
	{
		return utl::FindIf( items.begin(), items.end(), pred, predEval ) != items.end();
	}


	template< typename ContainerT, typename UnaryPredT >
	inline bool Any( const ContainerT& objects, UnaryPredT pred )
	{
		return std::find_if( objects.begin(), objects.end(), pred ) != objects.end();
	}

	template< typename ContainerT, typename UnaryPredT >
	inline bool All( const ContainerT& objects, UnaryPredT pred )
	{
		return !objects.empty() && !ContainsIf( objects, pred, utl::NegatePred );
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

	template< typename ContainerT, typename SubSetContainerT >
	bool ContainsSubSet( const ContainerT& items, const SubSetContainerT& subSetItems )
	{
		if ( subSetItems.size() > items.size() )
			return false;

	#ifdef IS_CPP_11
		return std::all_of( subSetItems.begin(), subSetItems.end(),
							[&]( const typename SubSetContainerT::value_type& subItem ) { return Contains( items, subItem ); }
		);
	#else
		for ( SubSetContainerT::const_iterator itSubItem = subSetItems.begin(); itSubItem != subSetItems.end(); ++itSubItem )
			if ( !Contains( items, subItem ) )
				return false;

		return true;
	#endif
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
	inline size_t BinaryFindPos( const ContainerT& container, const KeyT& key, ToKeyFunc toKeyFunc )
	{
		typename ContainerT::const_iterator itFound = BinaryFind( container.begin(), container.end(), key, toKeyFunc );
		return itFound != container.end() ? std::distance( container.begin(), itFound ) : std::tstring::npos;
	}

	template< typename ContainerT, typename KeyT, typename ToKeyFunc >
	inline bool BinaryContains( const ContainerT& container, const KeyT& key, ToKeyFunc toKeyFunc )
	{
		return BinaryFind( container.begin(), container.end(), key, toKeyFunc ) != container.end();
	}


	// map find

	template< typename MapType >
	inline typename MapType::mapped_type FindValue( const MapType& objectMap, const typename MapType::key_type& key )
	{
		typename MapType::const_iterator itFound = objectMap.find( key );
		if ( itFound == objectMap.end() )
			return typename MapType::mapped_type();

		return itFound->second;
	}

	template< typename MapType >
	inline const typename MapType::mapped_type* FindValuePtr( const MapType& objectMap, const typename MapType::key_type& key )
	{
		typename MapType::const_iterator itFound = objectMap.find( key );
		return itFound != objectMap.end() ? &itFound->second : nullptr;
	}

	template< typename MapType >
	inline typename MapType::mapped_type* FindValuePtr( MapType& rObjectMap, const typename MapType::key_type& key )
	{
		typename MapType::iterator itFound = rObjectMap.find( key );
		return itFound != rObjectMap.end() ? &itFound->second : nullptr;
	}

	template< typename MapType >
	inline bool ContainsValue( const MapType& objectMap, const typename MapType::key_type& key )
	{
		return objectMap.find( key ) != objectMap.end();
	}
}


namespace utl
{
	// check containers are in order

	template< typename IteratorT, typename CompareValues >
	bool IsOrdered( IteratorT itStart, IteratorT itEnd, CompareValues comparator )
	{
		if ( std::distance( itStart, itEnd ) > 1 )		// enough values for conflicting order?
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


	template< typename IteratorT >
	bool IsContiguous( IteratorT itStart, IteratorT itEnd, typename IteratorT::value_type step = 1 )
	{	// are numeric values consecutive?
		IteratorT itPrev = itStart;

		if ( itStart == itEnd || ++itStart == itEnd )
			return true;			// empty sequence or single element is always consecutive

		for ( ; itStart != itEnd; ++itStart )
			if ( *itStart == *itPrev + step )
				itPrev = itStart;
			else
				return false;		// found a gap or duplicate

		return true;
	}

	template< typename ContainerT >
	inline bool IsContiguous( const ContainerT& numValues, typename ContainerT::value_type step = 1 )
	{
		return IsContiguous( numValues.begin(), numValues.end(), step );
	}
}


namespace utl
{
	template< typename SrcContainerT, typename OutIteratorT, typename PredT >
	OutIteratorT CopyIfAs( const SrcContainerT& srcItems, OUT OutIteratorT itOutItems, PredT pred )
	{	// copy each satisfying pred cast as DesiredT
		typedef typename OutIteratorT::container_type::value_type TDestType;

		for ( typename SrcContainerT::const_iterator itSrc = srcItems.begin(), itEnd = srcItems.end(); itSrc != itEnd; ++itSrc )
			if ( pred( *itSrc ) )
			{
				*itOutItems = checked_static_cast<TDestType>( *itSrc );
				++itOutItems;
			}

		return itOutItems;
	}

	// copy items between containers using a conversion functor (unary)

	template< typename ContainerT, typename UnaryFuncT >
	void GenerateN( OUT ContainerT& rItems, size_t count, UnaryFuncT genFunc, size_t atPos = utl::npos )
	{
		if ( utl::npos == atPos )
			atPos = rItems.size();

		rItems.insert( rItems.begin() + atPos, count, typename ContainerT::value_type() );		// append SRC count
		std::generate( rItems.begin() + atPos, rItems.begin() + atPos + count, genFunc );
	}

	template< typename DestContainerT, typename SrcContainerT, typename CvtFuncT >
	inline void Assign( OUT DestContainerT& rDestItems, const SrcContainerT& srcItems, CvtFuncT cvtFunc )
	{
		rDestItems.resize( srcItems.size() );
		std::transform( srcItems.begin(), srcItems.end(), rDestItems.begin(), cvtFunc );
	}

	template< typename DestContainerT, typename SrcContainerT >
	inline size_t Append( IN OUT DestContainerT& rDestItems, const SrcContainerT& srcItems )
	{	// straight conversion from SRC to DEST items:
		size_t origCount = rDestItems.size();

		rDestItems.insert( rDestItems.end(), srcItems.begin(), srcItems.end() );
		return rDestItems.size() - origCount;
	}

	template< typename DestContainerT, typename SrcContainerT, typename CvtFuncT >
	inline void Append( IN OUT DestContainerT& rDestItems, const SrcContainerT& srcItems, CvtFuncT cvtFunc )
	{
		size_t origDestCount = rDestItems.size();
		rDestItems.insert( rDestItems.end(), srcItems.size(), typename DestContainerT::value_type() );		// append SRC count
		std::transform( srcItems.begin(), srcItems.end(), rDestItems.begin() + origDestCount, cvtFunc );
	}

	template< typename DestContainerT, typename SrcContainerT, typename CvtFuncT >
	void Prepend( IN OUT DestContainerT& rDestItems, const SrcContainerT& srcItems, CvtFuncT cvtFunc )
	{
		rDestItems.insert( rDestItems.begin(), srcItems.size(), typename DestContainerT::value_type() );	// prepend SRC count
		std::transform( srcItems.begin(), srcItems.end(), rDestItems.begin(), cvtFunc );
	}


	template< typename Type, typename ItemT >
	inline void AddSorted( IN OUT std::vector<Type>& rDest, ItemT item )
	{
		rDest.insert( std::upper_bound( rDest.begin(), rDest.end(), item ), item );
	}

	template< typename Type, typename ItemT, typename OrderBinaryPred >
	inline void AddSorted( IN OUT std::vector<Type>& rDest, ItemT item, OrderBinaryPred orderPred )		// predicate version
	{
		rDest.insert( std::upper_bound( rDest.begin(), rDest.end(), item, orderPred ), item );
	}

	template< typename Type, typename IteratorT, typename OrderBinaryPred >
	void AddSorted( IN OUT std::vector<Type>& rDest, IteratorT itFirst, IteratorT itEnd, OrderBinaryPred orderPred )		// sequence with predicate version
	{
		for ( ; itFirst != itEnd; ++itFirst )
			AddSorted( rDest, *itFirst, orderPred );
	}

	template< typename ContainerT, typename ItemT >
	inline bool AddUnique( IN OUT ContainerT& rDest, ItemT item )
	{
		if ( std::find( rDest.begin(), rDest.end(), item ) != rDest.end() )
			return false;

		rDest.push_back( item );
		return true;
	}

	template< typename ContainerT, typename IteratorT >
	size_t JoinUnique( IN OUT ContainerT& rDest, IteratorT itStart, IteratorT itEnd )
	{
		size_t oldCount = rDest.size();

		for ( ; itStart != itEnd; ++itStart )
			AddUnique( rDest, *itStart );

		return rDest.size() - oldCount;		// added count
	}

	template< typename ContainerT, typename ItemT >
	inline void PushUnique( IN OUT ContainerT& rContainer, ItemT item, size_t pos = std::tstring::npos )
	{
		ASSERT( !Contains( rContainer, item ) );
		rContainer.insert( std::tstring::npos == pos ? rContainer.end() : ( rContainer.begin() + pos ), item );
	}


	template< typename ContainerT, typename ValueT >
	size_t Remove( IN OUT ContainerT& rItems, const ValueT& value )
	{
		typename ContainerT::iterator itRemove = std::remove( rItems.begin(), rItems.end(), value );	// doesn't actually remove, just move items to be removed at the end
		size_t delCount = std::distance( itRemove, rItems.end() );

		rItems.erase( itRemove, rItems.end() );
		return delCount;
	}

	template< typename ContainerT, typename UnaryPredT >
	size_t RemoveIf( IN OUT ContainerT& rItems, UnaryPredT pred )
	{
		typename ContainerT::iterator itRemove = std::remove_if( rItems.begin(), rItems.end(), pred );	// doesn't actually remove, just move items to be removed at the end
		size_t delCount = std::distance( itRemove, rItems.end() );

		rItems.erase( itRemove, rItems.end() );
		return delCount;
	}


	template< typename ContainerT, typename UnaryPredT >
	std::pair<size_t, UnaryPredT> RemoveIfPred( IN OUT ContainerT& rItems, UnaryPredT pred, PredEval predEval = utl::TruePred )
	{	// returns a pair of:
		//	- count of deleted items
		//	- copy of the predicate, as in std::for_each().
		size_t delCount = 0;

		for ( typename ContainerT::iterator itItem = rItems.begin(); itItem != rItems.end(); )
			if ( pred( *itItem ) != ( NegatePred == predEval ) )
			{
				itItem = rItems.erase( itItem );
				++delCount;
			}
			else
				++itItem;

		return std::make_pair( delCount, pred );
	}


	template< typename ContainerT >
	bool RemoveValue( IN OUT ContainerT& rContainer, const typename ContainerT::value_type& value )		// remove once
	{
		typename ContainerT::iterator itFound = std::find( rContainer.begin(), rContainer.end(), value );

		if ( itFound == rContainer.end() )
			return false;

		rContainer.erase( itFound );
		return true;
	}

	template< typename ContainerT >
	void RemoveExisting( IN OUT ContainerT& rContainer, const typename ContainerT::value_type& rItem )	// remove once
	{
		typename ContainerT::iterator itFound = std::find( rContainer.begin(), rContainer.end(), rItem );
		ASSERT( itFound != rContainer.end() );
		rContainer.erase( itFound );
	}


	template< typename ContainerT >
	size_t UniquifyLinear( IN OUT ContainerT& rItems )		// slow, don't use - use the utl::Uniquify()
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
}


namespace utl
{
	// specific type

	template< typename DestContainerT, typename SrcContainerT >
	void QueryWithType( IN OUT DestContainerT& rOutObjects, const SrcContainerT& srcObjects )
	{
		typedef typename DestContainerT::value_type TDesired;

		for ( typename SrcContainerT::const_iterator itSrc = srcObjects.begin(); itSrc != srcObjects.end(); ++itSrc )
			if ( TDesired pDesired = dynamic_cast<TDesired>( *itSrc ) )
				rOutObjects.push_back( pDesired );
	}

	template< typename DestContainerT, typename SrcContainerT >
	size_t AddWithType( IN OUT DestContainerT& rDestObjects, const SrcContainerT& srcObjects )
	{
		typedef typename std::remove_pointer<typename DestContainerT::value_type>::type TDest;

		size_t oldCount = rDestObjects.size();
		CopyIfAs( srcObjects, std::inserter( rDestObjects, rDestObjects.end() ), pred::IsA<TDest>() );
		return rDestObjects.size() - oldCount;		// added count
	}

	template< typename TargetT, typename DestContainerT, typename SrcContainerT >
	size_t AddWithoutType( IN OUT DestContainerT& rDestObjects, const SrcContainerT& srcObjects )
	{
		size_t oldCount = rDestObjects.size();
		CopyIfAs( srcObjects, std::inserter( rDestObjects, rDestObjects.end() ), pred::IsNotA<TargetT>() );
		return rDestObjects.size() - oldCount;		// added count
	}

	template< typename ToRemoveT, typename ContainerT >
	inline size_t RemoveWithType( IN OUT ContainerT& rItems )
	{
		return RemoveIf( rItems, pred::IsA<ToRemoveT>() );
	}

	template< typename KeepT, typename ContainerT >
	inline size_t RemoveWithoutType( IN OUT ContainerT& rItems )
	{
		return RemoveIf( rItems, pred::IsNotA<KeepT>() );
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
	//
	template< typename Type >
	bool SameContents( const std::vector<Type>& left, const std::vector<Type>& right )
	{
		if ( left.size() != right.size() )
			return false;

		for ( size_t i = 0; i != left.size(); ++i )
			if ( !( left[ i ] == right[ i ] ) )				// try by index
				if ( !utl::Contains( right, left[ i ] ) )
					return false;

		return true;
	}

	template< typename Type, typename BinaryPred >
	bool SameContents( const std::vector<Type>& left, const std::vector<Type>& right, BinaryPred equalPred )
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
	void QuerySubSequenceFromIndexes( OUT std::vector<Type>& rSubSequence, const std::vector<Type>& source, const std::vector<IndexT>& selIndexes )
	{
		REQUIRE( selIndexes.size() <= source.size() );

		rSubSequence.clear();
		rSubSequence.reserve( selIndexes.size() );

		for ( typename std::vector<IndexT>::const_iterator itSelIndex = selIndexes.begin(); itSelIndex != selIndexes.end(); ++itSelIndex )
			rSubSequence.push_back( source[ *itSelIndex ] );
	}

	template< typename IndexT, typename ContainerT >
	void QuerySubSequenceIndexes( OUT std::vector<IndexT>& rIndexes, const ContainerT& source, const ContainerT& subSequence )
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
	std::pair<size_t, size_t> RemoveIntersection( IN OUT LeftContainerT& rLeft, IN OUT RightContainer2T& rRight )
	{
		std::pair<size_t, size_t> count( 0, 0 );

		for ( typename LeftContainerT::iterator itLeft = rLeft.begin(); itLeft != rLeft.end(); )
			if ( size_t rightCount = Remove( rRight, *itLeft ) )
			{
				itLeft = rLeft.erase( itLeft );

				++count.first;
				count.second += rightCount;
			}
			else
				++itLeft;

		return count;
	}

	template< typename LeftContainerT, typename RightContainerT >
	size_t RemoveLeftDuplicates( IN OUT LeftContainerT& rLeft, const RightContainerT& right )
	{
		size_t delCount = 0;

		for ( typename LeftContainerT::iterator itLeft = rLeft.begin(); itLeft != rLeft.end(); )
			if ( std::find( right.begin(), right.end(), *itLeft ) != right.end() )
			{
				itLeft = rLeft.erase( itLeft );
				++delCount;
			}
			else
				++itLeft;

		return delCount;
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

	template< typename IteratorT, typename Value, typename UnaryPredT >
	Value CircularFind( IteratorT itFirst, IteratorT itLast, Value startValue, UnaryPredT pred )
	{
		IteratorT itStart = std::find( itFirst, itLast, startValue );
		if ( itStart != itLast )
			++itStart;
		else
			itStart = itFirst;

		IteratorT itMatch = std::find_if( itStart, itLast, pred );
		if ( itMatch == itLast && itStart != itFirst )
			itMatch = std::find_if( itFirst, itStart - 1, pred );

		return itMatch != itLast && pred( *itMatch ) ? *itMatch : Value( nullptr );
	}


	template< typename PosT >
	bool AdvancePos( IN OUT PosT& rPos, PosT count, bool wrap, bool next, PosT step = 1 )
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
