#ifndef ComparePredicates_h
#define ComparePredicates_h
#pragma once

#include "Compare_fwd.h"


namespace func
{
	// adapters for comparison predicates

	struct PtrToReference
	{
		template< typename ObjectT >
		const ObjectT& operator()( const ObjectT* pObject ) const
		{
			ASSERT_PTR( pObject );
			return *pObject;
		}

		template< typename ObjectT >
		ObjectT& operator()( ObjectT* pObject ) const
		{
			ASSERT_PTR( pObject );
			return *pObject;
		}
	};


	struct ReferenceToPtr
	{
		template< typename ObjectT >
		const ObjectT* operator()( const ObjectT& object ) const
		{
			return &object;
		}

		template< typename ObjectT >
		ObjectT* operator()( ObjectT& object ) const
		{
			return &object;
		}
	};


	template< typename SubType >
	struct As
	{
		template< typename ObjectT >
		const SubType* operator()( const ObjectT* pObject ) const
		{
			return checked_static_cast<const SubType*>( pObject );
		}
	};


	template< typename SubType >
	struct DynamicAs
	{
		template< typename ObjectT >
		const SubType* operator()( const ObjectT* pObject ) const
		{
			return dynamic_cast<const SubType*>( pObject );
		}
	};


	template< typename Functor, typename Adapter, typename FunctorReturnT = typename Functor::TReturn >
	struct ValueAdapter
	{
		ValueAdapter( Functor functor = Functor(), Adapter adapter = Adapter() ) : m_functor( functor ), m_adapter( adapter ) {}

		template< typename ValueT >
		FunctorReturnT operator()( const ValueT& rValue ) const
		{
			return m_functor( m_adapter( rValue ) );
		}
	private:
		Functor m_functor;
		Adapter m_adapter;
	};
}


namespace pred
{
	template< typename CmpValuesT >
	struct CompareInOrder : public BaseComparator
	{
		CompareInOrder( CmpValuesT cmpValues = CmpValuesT(), bool ascendingOrder = true ) : m_cmpValues( cmpValues ), m_ascendingOrder( ascendingOrder ) {}

		template< typename ValueT >
		CompareResult operator()( const ValueT& left, const ValueT& right ) const
		{
			return GetResultInOrder( m_cmpValues( left, right ), m_ascendingOrder );
		}
	private:
		CmpValuesT m_cmpValues;
		bool m_ascendingOrder;
	};


	template< typename CmpValuesT >
	inline CompareInOrder<CmpValuesT> MakeCompareInOrder( CmpValuesT cmpValues, bool ascendingOrder = true )
	{
		return CompareInOrder<CmpValuesT>( cmpValues, ascendingOrder );
	}
}


namespace pred
{
	template< typename CmpValuesT >
	struct LessValue : public BasePredicate		// used for either containers of values or pointers, based on a values comparator
	{
		LessValue( CmpValuesT cmpValues = CmpValuesT() ) : m_cmpValues( cmpValues ) {}

		template< typename ValueT >
		bool operator()( const ValueT& left, const ValueT& right ) const		// containers of values OR pointers - for pointers it generates 'const ValueT*& left'
		{
			return Less == m_cmpValues( left, right );
		}
	private:
		CmpValuesT m_cmpValues;
	};


	template< typename CmpPtrT >
	struct LessPtr : public BasePredicate
	{
		LessPtr( CmpPtrT cmpPtrs = CmpPtrT() ) : m_cmpPtrs( cmpPtrs ) {}

		template< typename ObjectT >
		bool operator()( const ObjectT* pLeft, const ObjectT* pRight ) const
		{
			return Less == m_cmpPtrs( pLeft, pRight );
		}
	private:
		CmpPtrT m_cmpPtrs;
	};


	template< typename ToKeyFunc >
	struct LessKey : public BasePredicate			// order by keys; ToKeyFunc must provide all conversions to key
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


	template< typename CmpValuesT >
	struct OrderByValue : public BasePredicate
	{
		OrderByValue( bool ascendingOrder = true ) : m_ascendingOrder( ascendingOrder ) {}
		OrderByValue( CmpValuesT cmpValues, bool ascendingOrder = true ) : m_cmpValues( cmpValues ), m_ascendingOrder( ascendingOrder ) {}

		template< typename ValueT >
		bool operator()( const ValueT& left, const ValueT& right ) const		// containers of values OR pointers - for pointers it generates 'const ValueT*& left'
		{
			return ( m_ascendingOrder ? Less : Greater ) == m_cmpValues( left, right );
		}
	private:
		CmpValuesT m_cmpValues;
		bool m_ascendingOrder;
	};


	template< typename CmpPtrT >
	struct OrderByPtr : public BasePredicate
	{
		OrderByPtr( bool ascendingOrder = true ) : m_ascendingOrder( ascendingOrder ) {}
		OrderByPtr( CmpPtrT cmpPtrs, bool ascendingOrder = true ) : m_cmpPtrs( cmpPtrs ), m_ascendingOrder( ascendingOrder ) {}

		template< typename ObjectT >
		bool operator()( const ObjectT* pLeft, const ObjectT* pRight ) const
		{
			return ( m_ascendingOrder ? Less : Greater ) == m_cmpPtrs( pLeft, pRight );
		}
	private:
		CmpPtrT m_cmpPtrs;
		bool m_ascendingOrder;
	};


	template< typename CmpValuesT >
	struct IsEqual : public BasePredicate
	{
		IsEqual( CmpValuesT cmpValues = CmpValuesT() ) : m_cmpValues( cmpValues ) {}

		template< typename ValueT >
		bool operator()( const ValueT& left, const ValueT& right ) const		// containers of values
		{
			return Equal == m_cmpValues( left, right );
		}
	private:
		CmpValuesT m_cmpValues;
	};

	template< typename CmpValuesT >
	inline IsEqual<CmpValuesT> MakeIsEqual( CmpValuesT cmpValues )
	{
		return IsEqual<CmpValuesT>( cmpValues );
	}


	// adapts Compare defined for an object to a new object via Adapter functor; example CompareAdapter<CompareRequestDate, func::ToWorkOrder>

	template< typename CmpValuesT, typename Adapter >
	struct CompareAdapter : public BaseComparator
	{
		CompareAdapter( CmpValuesT cmpValues = CmpValuesT(), Adapter adapter = Adapter() ) : m_cmpValues( cmpValues ), m_adapter( adapter ) {}

		template< typename ValueT >
		CompareResult operator()( const ValueT& left, const ValueT& right ) const
		{
			return m_cmpValues( m_adapter( left ), m_adapter( right ) );
		}
	private:
		CmpValuesT m_cmpValues;
		Adapter m_adapter;
	};


	template< typename CmpPtrT, typename AdapterPtr >
	struct CompareAdapterPtr : public BaseComparator
	{
		CompareAdapterPtr( CmpPtrT cmpPtrs = CmpPtrT(), AdapterPtr adapter = AdapterPtr() ) : m_cmpPtrs( cmpPtrs ), m_adapter( adapter ) {}

		template< typename ObjectT >
		CompareResult operator()( const ObjectT* pLeft, const ObjectT* pRight ) const
		{
			return m_cmpPtrs( m_adapter( pLeft ), m_adapter( pRight ) );
		}
	private:
		CmpPtrT m_cmpPtrs;
		AdapterPtr m_adapter;
	};


	// compares scalars via the Adapter functor (some Unix compilers can't define template typedefs with CompareAdapterPtr?)

	template< typename Adapter >
	struct CompareScalarAdapterPtr : public BaseComparator
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


	// creates a descending criteria by flipping Less and Greater of the comparator; example Descending<CompareEndTime>

	template< typename CmpValuesT >
	struct Descending : public BaseComparator
	{
		Descending( CmpValuesT cmpValues = CmpValuesT() ) : m_cmpValues( cmpValues ) {}

		template< typename ValueT >
		CompareResult operator()( const ValueT& left, const ValueT& right ) const
		{
			return GetInvertedResult( m_cmpValues( left, right ) );
		}
	private:
		CmpValuesT m_cmpValues;
	};

	template< typename CmpPtrT >
	struct DescendingPtr : public BaseComparator
	{
		DescendingPtr( CmpPtrT cmpPtrs = CmpPtrT() ) : m_cmpPtrs( cmpPtrs ) {}

		template< typename ObjectT >
		CompareResult operator()( const ObjectT* pLeft, const ObjectT* pRight ) const
		{
			return GetInvertedResult( m_cmpPtrs( pLeft, pRight ) );
		}
	private:
		CmpPtrT m_cmpPtrs;
	};


	/**
		combines two comparison criteria:
			- a main comparison criteria
			- a secondary comparison criteria, used if the operands are EQUAL by the main comparison criteria
	*/

	template< typename CmpMainT, typename CmpExtraT >
	struct JoinCompare : public BaseComparator
	{
		template< typename ValueT >
		CompareResult operator()( const ValueT& left, const ValueT& right ) const
		{
			CompareResult result = CmpMainT()( left, right );
			if ( Equal == result )
				result = CmpExtraT()( left, right );
			return result;
		}
	};

	template< typename CmpMainPtrT, typename CmpExtraPtrT >
	struct JoinComparePtr : public BaseComparator
	{
		template< typename ObjectT >
		CompareResult operator()( const ObjectT* pLeft, const ObjectT* pRight ) const
		{
			CompareResult result = CmpMainPtrT()( pLeft, pRight );
			if ( Equal == result )
				result = CmpExtraPtrT()( pLeft, pRight );
			return result;
		}
	};


	template< typename CompareFirstT >
	struct CompareFirst : public BaseComparator
	{
		template< typename T, typename U >
		CompareResult operator()( const std::pair<T, U>& left, const std::pair<T, U>& right ) const
		{
			return CompareFirstT()( left.first, right.first );
		}
	};

	template< typename CompareSecondT >
	struct CompareSecond : public BaseComparator
	{
		template< typename T, typename U >
		CompareResult operator()( const std::pair<T, U>& left, const std::pair<T, U>& right ) const
		{
			return CompareSecondT()( left.second, right.second );
		}
	};


	template< typename CompareFirstT, typename CompareSecondT >
	struct CompareFirstSecond : public BaseComparator
	{
		template< typename T, typename U >
		CompareResult operator()( const std::pair<T, U>& left, const std::pair<T, U>& right ) const
		{
			CompareResult result = CompareFirstT()( left.first, right.first );
			if ( Equal == result )
				result = CompareSecondT()( left.second, right.second );
			return result;
		}
	};

	template< typename CompareFirstT, typename CompareSecondT >
	struct CompareSecondFirst : public BaseComparator
	{
		template< typename T, typename U >
		CompareResult operator()( const std::pair<T, U>& left, const std::pair<T, U>& right ) const
		{
			CompareResult result = CompareSecondT()( left.second, right.second );
			if ( Equal == result )
				result = CompareFirstT()( left.first, right.first );
			return result;
		}
	};
}


namespace func
{
	// make template functions for compare primitives that work stateful functors

	template< typename Functor, typename Adapter >
	inline ValueAdapter<Functor, Adapter> MakeValueAdapter( Functor functor, Adapter adapter ) { return ValueAdapter<Functor, Adapter>( functor, adapter ); }
}


namespace pred
{
	// make template functions for compare primitives that work stateful functors

	template< typename CmpValuesT >
	inline LessValue<CmpValuesT> MakeLessValue( CmpValuesT cmpValues ) { return LessValue<CmpValuesT>( cmpValues ); }

	template< typename CmpPtrT >
	inline LessPtr<CmpPtrT> MakeLessPtr( CmpPtrT cmpPtrs ) { return LessPtr<CmpPtrT>( cmpPtrs ); }

	template< typename ToKeyFunc >
	inline LessKey<ToKeyFunc> MakeLessKey( ToKeyFunc toKeyFunc ) { return LessKey<ToKeyFunc>( toKeyFunc ); }

	template< typename CmpValuesT >
	inline OrderByValue<CmpValuesT> MakeOrderByValue( CmpValuesT cmpValues, bool ascendingOrder = true ) { return OrderByValue<CmpValuesT>( cmpValues, ascendingOrder ); }

	template< typename CmpPtrT >
	inline OrderByPtr<CmpPtrT> MakeOrderByPtr( CmpPtrT cmpPtrs, bool ascendingOrder = true ) { return OrderByPtr<CmpPtrT>( cmpPtrs, ascendingOrder ); }
}


#endif // ComparePredicates_h
