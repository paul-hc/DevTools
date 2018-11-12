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
		template< typename ObjectType >
		const SubType* operator()( const ObjectType* pObject ) const
		{
			return checked_static_cast< const SubType* >( pObject );
		}
	};

	template< typename SubType >
	struct DynamicAs
	{
		template< typename ObjectType >
		const SubType* operator()( const ObjectType* pObject ) const
		{
			return dynamic_cast< const SubType* >( pObject );
		}
	};

	struct ToSelf
	{
		template< typename ValueType >
		const ValueType& operator()( const ValueType& value ) const { return value; }
	};
}


namespace pred
{
	template< typename Compare >
	struct CompareInOrder
	{
		CompareInOrder( Compare compare = Compare(), bool ascendingOrder = true ) : m_compare( compare ), m_ascendingOrder( ascendingOrder ) {}

		template< typename T >
		CompareResult operator()( const T& left, const T& right ) const
		{
			return GetResultInOrder( m_compare( left, right ), m_ascendingOrder );
		}
	private:
		Compare m_compare;
		bool m_ascendingOrder;
	};


	template< typename Compare >
	inline CompareInOrder< Compare > MakeCompareInOrder( Compare compare, bool ascendingOrder = true )
	{
		return CompareInOrder< Compare >( compare, ascendingOrder );
	}
}


namespace pred
{
	template< typename Compare >
	struct LessBy
	{
		LessBy( Compare compare = Compare() ) : m_compare( compare ) {}

		template< typename T >
		bool operator()( const T& left, const T& right ) const		// containers of values
		{
			return Less == m_compare( left, right );
		}

		template< typename T >
		bool operator()( const T* pLeft, const T* pRight ) const				// containers of pointers (non-const pointers to disambiguate)
		{
			return Less == m_compare( *pLeft, *pRight );
		}
	private:
		Compare m_compare;
	};

	template< typename ComparePtr >
	struct LessPtr
	{
		LessPtr( ComparePtr comparePtr = ComparePtr() ) : m_comparePtr( comparePtr ) {}

		template< typename ObjectType >
		bool operator()( const ObjectType* pLeft, const ObjectType* pRight ) const
		{
			return Less == m_comparePtr( pLeft, pRight );
		}
	private:
		ComparePtr m_comparePtr;
	};

	template< typename ToKeyFunc >
	struct LessKey					// order by keys; ToKeyFunc must provide all conversions to key
	{
		LessKey( ToKeyFunc toKey = ToKeyFunc() ) : m_toKey( toKey ) {}

		template< typename T1, typename T2 >
		bool operator()( const T1& left, const T2& right ) const
		{
			return m_toKey( left ) < m_toKey( right );
		}
	private:
		ToKeyFunc m_toKey;
	};


	template< typename Compare >
	struct OrderBy
	{
		OrderBy( bool ascendingOrder = true ) : m_ascendingOrder( ascendingOrder ) {}
		OrderBy( Compare compare, bool ascendingOrder = true ) : m_compare( compare ), m_ascendingOrder( ascendingOrder ) {}

		template< typename T >
		bool operator()( const T& left, const T& right ) const		// containers of values
		{
			return ( m_ascendingOrder ? Less : Greater ) == m_compare( left, right );
		}

		template< typename T >
		bool operator()( T* pLeft, T* pRight ) const				// containers of pointers (non-const pointers to disambiguate)
		{
			return ( m_ascendingOrder ? Less : Greater ) == m_compare( *pLeft, *pRight );
		}
	private:
		Compare m_compare;
		bool m_ascendingOrder;
	};

	template< typename ComparePtr >
	struct OrderByPtr
	{
		OrderByPtr( bool ascendingOrder = true ) : m_ascendingOrder( ascendingOrder ) {}
		OrderByPtr( ComparePtr compare, bool ascendingOrder = true ) : m_compare( compare ), m_ascendingOrder( ascendingOrder ) {}

		template< typename ObjectType >
		bool operator()( const ObjectType* pLeft, const ObjectType* pRight ) const
		{
			return ( m_ascendingOrder ? Less : Greater ) == m_compare( pLeft, pRight );
		}
	private:
		ComparePtr m_compare;
		bool m_ascendingOrder;
	};


	template< typename Compare >
	struct IsEqual
	{
		IsEqual( Compare compare = Compare() ) : m_compare( compare ) {}

		template< typename T >
		bool operator()( const T& left, const T& right ) const		// containers of values
		{
			return Equal == m_compare( left, right );
		}
	private:
		Compare m_compare;
	};


	// adapts Compare defined for an object to a new object via Adapter functor; example CompareAdapter< CompareRequestDate, func::ToWorkOrder >

	template< typename Compare, typename Adapter >
	struct CompareAdapter
	{
		CompareAdapter( Compare compare = Compare(), Adapter adapter = Adapter() ) : m_compare( compare ), m_adapter( adapter ) {}

		template< typename T >
		CompareResult operator()( const T& left, const T& right ) const
		{
			return m_compare( m_adapter( left ), m_adapter( right ) );
		}
	private:
		Compare m_compare;
		Adapter m_adapter;
	};

	template< typename ComparePtr, typename AdapterPtr >
	struct CompareAdapterPtr
	{
		CompareAdapterPtr( ComparePtr compare = ComparePtr(), AdapterPtr adapter = AdapterPtr() ) : m_compare( compare ), m_adapter( adapter ) {}

		template< typename ObjectType >
		CompareResult operator()( const ObjectType* pLeft, const ObjectType* pRight ) const
		{
			return m_compare( m_adapter( pLeft ), m_adapter( pRight ) );
		}
	private:
		ComparePtr m_compare;
		AdapterPtr m_adapter;
	};


	// compares scalars via the Adapter functor (some Unix compilers can't define template typedefs with CompareAdapterPtr?)

	template< typename Adapter >
	struct CompareScalarAdapterPtr
	{
		CompareScalarAdapterPtr( Adapter adapter = Adapter() ) : m_adapter( adapter ) {}

		template< typename ObjectType >
		CompareResult operator()( const ObjectType* pLeft, const ObjectType* pRight ) const
		{
			return Compare_Scalar( m_adapter( pLeft ), m_adapter( pRight ) );
		}
	private:
		Adapter m_adapter;
	};


	// creates a descending criteria by flipping Less and Greater of the comparator; example Descending< CompareEndTime >

	template< typename Compare >
	struct Descending
	{
		Descending( Compare compare = Compare() ) : m_compare( compare ) {}

		template< typename T >
		CompareResult operator()( const T& left, const T& right ) const
		{
			return GetInvertedResult( m_compare( left, right ) );
		}
	private:
		Compare m_compare;
	};

	template< typename ComparePtr >
	struct DescendingPtr
	{
		DescendingPtr( ComparePtr compare = ComparePtr() ) : m_compare( compare ) {}

		template< typename ObjectType >
		CompareResult operator()( const ObjectType* pLeft, const ObjectType* pRight ) const
		{
			return GetInvertedResult( m_compare( pLeft, pRight ) );
		}
	private:
		ComparePtr m_compare;
	};


	/**
		combines two comparison criteria:
			- a main comparison criteria
			- a secondary comparison criteria, used if the operands are EQUAL by the main comparison criteria
	*/

	template< typename Compare1, typename Compare2 >
	struct JoinCompare
	{
		template< typename T >
		CompareResult operator()( const T& left, const T& right ) const
		{
			CompareResult result = Compare1()( left, right );
			if ( Equal == result )
				result = Compare2()( left, right );
			return result;
		}
	};

	template< typename ComparePtr1, typename ComparePtr2 >
	struct JoinComparePtr
	{
		template< typename ObjectType >
		CompareResult operator()( const ObjectType* pLeft, const ObjectType* pRight ) const
		{
			CompareResult result = ComparePtr1()( pLeft, pRight );
			if ( Equal == result )
				result = ComparePtr2()( pLeft, pRight );
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

	template< typename Compare >
	inline LessBy< Compare > MakeLessBy( Compare compare )
	{
		return LessBy< Compare >( compare );
	}

	template< typename Compare >
	inline LessPtr< Compare > MakeLessPtr( Compare compare )
	{
		return LessPtr< Compare >( compare );
	}

	template< typename ToKeyFunc >
	inline LessKey< ToKeyFunc > MakeLessKey( ToKeyFunc toKeyFunc ) { return LessKey< ToKeyFunc >( toKeyFunc ); }

	template< typename Compare >
	inline OrderBy< Compare > MakeOrderBy( Compare compare, bool ascendingOrder = true )
	{
		return OrderBy< Compare >( compare, ascendingOrder );
	}

	template< typename ComparePtr >
	inline OrderByPtr< ComparePtr > MakeOrderByPtr( ComparePtr compare, bool ascendingOrder = true )
	{
		return OrderByPtr< ComparePtr >( compare, ascendingOrder );
	}
}


#endif // ComparePredicates_h
