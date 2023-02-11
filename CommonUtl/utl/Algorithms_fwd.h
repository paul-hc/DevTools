#ifndef Algorithms_fwd_h
#define Algorithms_fwd_h
#pragma once


// forward declarations - required for C++ 14+ compilation (which is stricter)

namespace pred
{
	template< typename ToKeyFunc >
	struct LessKey;

	template< typename ToKeyFunc >
	LessKey<ToKeyFunc> MakeLessKey( ToKeyFunc toKeyFunc );
}


// general algorithms

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


	template< typename DiffT, typename IteratorT >
	inline DiffT Distance( IteratorT itFirst, IteratorT itLast )
	{
		return static_cast<DiffT>( std::distance( itFirst, itLast ) );
	}


	// container bounds: works with std::list (not random iterator)

	template< typename ContainerT >
	inline const typename ContainerT::value_type& Front( const ContainerT& rItems ) { ASSERT( !rItems.empty() ); return *rItems.begin(); }

	template< typename ContainerT >
	inline typename ContainerT::value_type& Front( ContainerT& rItems ) { ASSERT( !rItems.empty() ); return *rItems.begin(); }

	template< typename ContainerT >
	inline const typename ContainerT::value_type& Back( const ContainerT& rItems ) { ASSERT( !rItems.empty() ); return *--rItems.end(); }

	template< typename ContainerT >
	inline typename ContainerT::value_type& Back( ContainerT& rItems ) { ASSERT( !rItems.empty() ); return *--rItems.end(); }


	// reverse iterator based on forward position (mainly for unit testing)

	template< typename ContainerT >
	inline typename ContainerT::const_reverse_iterator RevIterAtFwdPos( const ContainerT& items, size_t forwardPos )
	{	// get a reverse iterator at a FORWARD position
		REQUIRE( forwardPos < items.size() );
		return items.rend() - forwardPos - 1;
	}

	template< typename ContainerT >
	inline typename ContainerT::const_reverse_iterator RevIterAtFwdFront( const ContainerT& items )
	{	// get a reverse iterator at a FORWARD front position
		return RevIterAtFwdPos( items, 0 );
	}

	template< typename ContainerT >
	inline typename ContainerT::const_reverse_iterator RevIterAtFwdBack( const ContainerT& items )
	{	// get a reverse iterator at a FORWARD back position
		return RevIterAtFwdPos( items, items.size() - 1 );
	}


	// FORWARD position corresponding to reverse iterator

	template< typename ContainerT >
	inline size_t FwdPosOfRevIter( typename ContainerT::const_reverse_iterator itRev, const ContainerT& items )
	{
		size_t fwdPos = std::distance( itRev, items.rend() ) - 1;
		ENSURE( fwdPos < items.size() );
		return fwdPos;
	}
}


namespace utl
{
	template< typename IterLeftT, typename IterRightT >
	bool Equals( IterLeftT itLeft, IterLeftT itLeftLast, IterRightT itRight, IterRightT itRightLast )
	{
		// Use this for sub-sequence matching (typically the sub-sequence is on the right-hand-side).
		// The left/right sequences can be of different length, checking is done on the common shortest length.
		//	Note:
		//	In <algorithm>, none of the safe std::equal() has the same effect as this (not equal if different sequence lengths),
		//	except the unsafe std::equal(itLeft, itLeftLast, itRight)!
		//
		if ( itLeft == itLeftLast || itRight == itRightLast )
			return false;		// one of the ranges empty

		for ( ; itLeft != itLeftLast && itRight != itRightLast; ++itLeft, ++itRight )
			if ( !( *itLeft == *itRight ) )
				return false;

		return itLeft == itLeftLast || itRight == itRightLast;		// one of the sequences was consumed
	}

	template< typename ContainerT, typename UnaryPred >
	void QueryThat( std::vector<typename ContainerT::value_type>& rSubset, const ContainerT& objects, UnaryPred pred )
	{
		// additive
		for ( typename ContainerT::const_iterator itObject = objects.begin(); itObject != objects.end(); ++itObject )
			if ( pred( *itObject ) )
				rSubset.push_back( *itObject );
	}
}


#endif // Algorithms_fwd_h
