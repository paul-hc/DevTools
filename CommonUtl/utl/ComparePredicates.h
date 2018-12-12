#ifndef ComparePredicates_h
#define ComparePredicates_h
#pragma once

#include "Compare_fwd.h"


namespace func
{
	// adapters for comparison predicates

	template< typename SubType >
	struct As
	{
		template< typename ObjectT >
		const SubType* operator()( const ObjectT* pObject ) const
		{
			return checked_static_cast< const SubType* >( pObject );
		}
	};

	template< typename SubType >
	struct DynamicAs
	{
		template< typename ObjectT >
		const SubType* operator()( const ObjectT* pObject ) const
		{
			return dynamic_cast< const SubType* >( pObject );
		}
	};

	struct ToSelf
	{
		template< typename ValueT >
		const ValueT& operator()( const ValueT& value ) const { return value; }
	};
}


namespace pred
{
	template< typename CompareValues >
	struct CompareInOrder
	{
		CompareInOrder( CompareValues cmpValues = CompareValues(), bool ascendingOrder = true ) : m_cmpValues( cmpValues ), m_ascendingOrder( ascendingOrder ) {}

		template< typename ValueT >
		CompareResult operator()( const ValueT& left, const ValueT& right ) const
		{
			return GetResultInOrder( m_cmpValues( left, right ), m_ascendingOrder );
		}
	private:
		CompareValues m_cmpValues;
		bool m_ascendingOrder;
	};


	template< typename CompareValues >
	inline CompareInOrder< CompareValues > MakeCompareInOrder( CompareValues cmpValues, bool ascendingOrder = true )
	{
		return CompareInOrder< CompareValues >( cmpValues, ascendingOrder );
	}
}


namespace pred
{
	template< typename CompareValues >
	struct LessValue							// used for either containers of values or pointers, based on a values comparator
	{
		LessValue( CompareValues cmpValues = CompareValues() ) : m_cmpValues( cmpValues ) {}

		template< typename ValueT >
		bool operator()( const ValueT& left, const ValueT& right ) const			// containers of values
		{
			return Less == m_cmpValues( left, right );
		}

		template< typename ObjectT >
		bool operator()( const ObjectT* pLeft, const ObjectT* pRight ) const		// containers of pointers (non-const pointers to disambiguate)
		{
			return Less == m_cmpValues( *pLeft, *pRight );							// compare-by-value dereferenced pointers
		}
	private:
		CompareValues m_cmpValues;
	};

	template< typename ComparePtrs >
	struct LessPtr
	{
		LessPtr( ComparePtrs cmpPtrs = ComparePtrs() ) : m_cmpPtrs( cmpPtrs ) {}

		template< typename ObjectT >
		bool operator()( const ObjectT* pLeft, const ObjectT* pRight ) const
		{
			return Less == m_cmpPtrs( pLeft, pRight );
		}
	private:
		ComparePtrs m_cmpPtrs;
	};

	template< typename ToKeyFunc >
	struct LessKey					// order by keys; ToKeyFunc must provide all conversions to key
	{
		LessKey( ToKeyFunc toKey = ToKeyFunc() ) : m_toKey( toKey ) {}

		template< typename ValueT1, typename ValueT2 >
		bool operator()( const ValueT1& left, const ValueT2& right ) const
		{
			return m_toKey( left ) < m_toKey( right );
		}
	private:
		ToKeyFunc m_toKey;
	};


	template< typename CompareValues >
	struct OrderByValue
	{
		OrderByValue( bool ascendingOrder = true ) : m_ascendingOrder( ascendingOrder ) {}
		OrderByValue( CompareValues cmpValues, bool ascendingOrder = true ) : m_cmpValues( cmpValues ), m_ascendingOrder( ascendingOrder ) {}

		template< typename ValueT >
		bool operator()( const ValueT& left, const ValueT& right ) const		// containers of values
		{
			return ( m_ascendingOrder ? Less : Greater ) == m_cmpValues( left, right );
		}

		template< typename ObjectT >
		bool operator()( ObjectT* pLeft, ObjectT* pRight ) const				// containers of pointers (non-const pointers to disambiguate)
		{
			return ( m_ascendingOrder ? Less : Greater ) == m_cmpValues( *pLeft, *pRight );		// compare-by-value dereferenced pointers
		}
	private:
		CompareValues m_cmpValues;
		bool m_ascendingOrder;
	};

	template< typename ComparePtrs >
	struct OrderByPtr
	{
		OrderByPtr( bool ascendingOrder = true ) : m_ascendingOrder( ascendingOrder ) {}
		OrderByPtr( ComparePtrs cmpPtrs, bool ascendingOrder = true ) : m_cmpPtrs( cmpPtrs ), m_ascendingOrder( ascendingOrder ) {}

		template< typename ObjectT >
		bool operator()( const ObjectT* pLeft, const ObjectT* pRight ) const
		{
			return ( m_ascendingOrder ? Less : Greater ) == m_cmpPtrs( pLeft, pRight );
		}
	private:
		ComparePtrs m_cmpPtrs;
		bool m_ascendingOrder;
	};


	template< typename CompareValues >
	struct IsEqual
	{
		IsEqual( CompareValues cmpValues = CompareValues() ) : m_cmpValues( cmpValues ) {}

		template< typename ValueT >
		bool operator()( const ValueT& left, const ValueT& right ) const		// containers of values
		{
			return Equal == m_cmpValues( left, right );
		}
	private:
		CompareValues m_cmpValues;
	};


	// adapts Compare defined for an object to a new object via Adapter functor; example CompareAdapter< CompareRequestDate, func::ToWorkOrder >

	template< typename CompareValues, typename Adapter >
	struct CompareAdapter
	{
		CompareAdapter( CompareValues cmpValues = CompareValues(), Adapter adapter = Adapter() ) : m_cmpValues( cmpValues ), m_adapter( adapter ) {}

		template< typename ValueT >
		CompareResult operator()( const ValueT& left, const ValueT& right ) const
		{
			return m_cmpValues( m_adapter( left ), m_adapter( right ) );
		}
	private:
		CompareValues m_cmpValues;
		Adapter m_adapter;
	};

	template< typename ComparePtrs, typename AdapterPtr >
	struct CompareAdapterPtr
	{
		CompareAdapterPtr( ComparePtrs cmpPtrs = ComparePtrs(), AdapterPtr adapter = AdapterPtr() ) : m_cmpPtrs( cmpPtrs ), m_adapter( adapter ) {}

		template< typename ObjectT >
		CompareResult operator()( const ObjectT* pLeft, const ObjectT* pRight ) const
		{
			return m_cmpPtrs( m_adapter( pLeft ), m_adapter( pRight ) );
		}
	private:
		ComparePtrs m_cmpPtrs;
		AdapterPtr m_adapter;
	};


	// compares scalars via the Adapter functor (some Unix compilers can't define template typedefs with CompareAdapterPtr?)

	template< typename Adapter >
	struct CompareScalarAdapterPtr
	{
		CompareScalarAdapterPtr( Adapter adapter = Adapter() ) : m_adapter( adapter ) {}

		template< typename ObjectT >
		CompareResult operator()( const ObjectT* pLeft, const ObjectT* pRight ) const
		{
			return Compare_Scalar( m_adapter( pLeft ), m_adapter( pRight ) );
		}
	private:
		Adapter m_adapter;
	};


	// creates a descending criteria by flipping Less and Greater of the comparator; example Descending< CompareEndTime >

	template< typename CompareValues >
	struct Descending
	{
		Descending( CompareValues cmpValues = CompareValues() ) : m_cmpValues( cmpValues ) {}

		template< typename ValueT >
		CompareResult operator()( const ValueT& left, const ValueT& right ) const
		{
			return GetInvertedResult( m_cmpValues( left, right ) );
		}
	private:
		CompareValues m_cmpValues;
	};

	template< typename ComparePtrs >
	struct DescendingPtr
	{
		DescendingPtr( ComparePtrs cmpPtrs = ComparePtrs() ) : m_cmpPtrs( cmpPtrs ) {}

		template< typename ObjectT >
		CompareResult operator()( const ObjectT* pLeft, const ObjectT* pRight ) const
		{
			return GetInvertedResult( m_cmpPtrs( pLeft, pRight ) );
		}
	private:
		ComparePtrs m_cmpPtrs;
	};


	/**
		combines two comparison criteria:
			- a main comparison criteria
			- a secondary comparison criteria, used if the operands are EQUAL by the main comparison criteria
	*/

	template< typename Compare1, typename Compare2 >
	struct JoinCompare
	{
		template< typename ValueT >
		CompareResult operator()( const ValueT& left, const ValueT& right ) const
		{
			CompareResult result = Compare1()( left, right );
			if ( Equal == result )
				result = Compare2()( left, right );
			return result;
		}
	};

	template< typename ComparePtrs1, typename ComparePtrs2 >
	struct JoinComparePtr
	{
		template< typename ObjectT >
		CompareResult operator()( const ObjectT* pLeft, const ObjectT* pRight ) const
		{
			CompareResult result = ComparePtrs1()( pLeft, pRight );
			if ( Equal == result )
				result = ComparePtrs2()( pLeft, pRight );
			return result;
		}
	};


	template< typename CompareFirstT >
	struct CompareFirst
	{
		template< typename T, typename U >
		CompareResult operator()( const std::pair< T, U >& left, const std::pair< T, U >& right ) const
		{
			return CompareFirstT()( left.first, right.first );
		}
	};

	template< typename CompareSecondT >
	struct CompareSecond
	{
		template< typename T, typename U >
		CompareResult operator()( const std::pair< T, U >& left, const std::pair< T, U >& right ) const
		{
			return CompareSecondT()( left.second, right.second );
		}
	};


	template< typename CompareFirstT, typename CompareSecondT >
	struct CompareFirstSecond
	{
		template< typename T, typename U >
		CompareResult operator()( const std::pair< T, U >& left, const std::pair< T, U >& right ) const
		{
			CompareResult result = CompareFirstT()( left.first, right.first );
			if ( Equal == result )
				result = CompareSecondT()( left.second, right.second );
			return result;
		}
	};

	template< typename CompareFirstT, typename CompareSecondT >
	struct CompareSecondFirst
	{
		template< typename T, typename U >
		CompareResult operator()( const std::pair< T, U >& left, const std::pair< T, U >& right ) const
		{
			CompareResult result = CompareSecondT()( left.second, right.second );
			if ( Equal == result )
				result = CompareFirstT()( left.first, right.first );
			return result;
		}
	};
}


namespace pred
{
	// make template functions for compare primitives that work stateful functors

	template< typename CompareValues >
	inline LessValue< CompareValues > MakeLessValue( CompareValues cmpValues ) { return LessValue< CompareValues >( cmpValues ); }

	template< typename ComparePtrs >
	inline LessPtr< ComparePtrs > MakeLessPtr( ComparePtrs cmpPtrs ) { return LessPtr< ComparePtrs >( cmpPtrs ); }

	template< typename ToKeyFunc >
	inline LessKey< ToKeyFunc > MakeLessKey( ToKeyFunc toKeyFunc ) { return LessKey< ToKeyFunc >( toKeyFunc ); }

	template< typename CompareValues >
	inline OrderByValue< CompareValues > MakeOrderByValue( CompareValues cmpValues, bool ascendingOrder = true ) { return OrderByValue< CompareValues >( cmpValues, ascendingOrder ); }

	template< typename ComparePtrs >
	inline OrderByPtr< ComparePtrs > MakeOrderByPtr( ComparePtrs cmpPtrs, bool ascendingOrder = true ) { return OrderByPtr< ComparePtrs >( cmpPtrs, ascendingOrder ); }
}


#endif // ComparePredicates_h
