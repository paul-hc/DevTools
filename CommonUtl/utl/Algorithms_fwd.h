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


namespace func
{
	struct GetFirst
	{
		template< typename T, typename U >
		inline const T& operator()( const std::pair<T, U>& p ) const
		{
			return p.first;
		}
	};

	struct GetSecond
	{
		template< typename T, typename U >
		inline const U& operator()( const std::pair<T, U>& p ) const
		{
			return p.second;
		}
	};
}


namespace utl
{
	template< typename StructT >
	inline StructT* ZeroStruct( OUT StructT* pStruct )
	{
		ASSERT_PTR( pStruct );
		::memset( pStruct, 0, sizeof( StructT ) );
		return pStruct;
	}

	template< typename StructT >
	inline StructT* ZeroWinStruct( OUT StructT* pStruct )		// for Win32 structures with 'cbSize' data-member
	{
		ZeroStruct( pStruct );
		pStruct->cbSize = sizeof( StructT );
		return pStruct;
	}


	template< typename EnumT >
	inline EnumT MaxEnum( EnumT left, EnumT right )
	{
		return static_cast<EnumT>( std::max<int>( left, right ) );
	}

	template< typename EnumT >
	inline EnumT MinEnum( EnumT left, EnumT right )
	{
		return static_cast<EnumT>( std::min<int>( left, right ) );
	}


	// va_list algorithms

	template< typename ArgT >
	void QueryArgumentList( OUT std::vector<ArgT>& rArguments, va_list argList, size_t argCount = utl::npos /* NULL terminated */ )
	{
		if ( argCount != utl::npos )
			rArguments.reserve( argCount );

		for ( size_t i = 0; i != argCount; ++i )
		{
			ArgT argument = va_arg( argList, ArgT );

			if ( utl::npos == argCount && 0 == argument )
				break;			// reached the terminating NULL

			rArguments.push_back( argument );
		}
	}
}


namespace utl
{
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
		if ( itLeft == itLeftLast )
			return itRight == itRightLast;		// both ranges empty
		else if ( itRight == itRightLast )
			return itLeft == itLeftLast;		// both ranges empty

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
