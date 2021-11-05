#ifndef Compare_fwd_h
#define Compare_fwd_h
#pragma once


enum SortType { NoSort, SortAscending, SortDescending };


namespace pred
{
	enum CompareResult { Less = -1, Equal, Greater };


	template< typename DiffType >
	inline CompareResult ToCompareResult( DiffType difference )
	{
		if ( difference < 0 )
			return Less;
		else if ( difference > 0 )
			return Greater;

		return Equal;
	}


	inline CompareResult GetResultInOrder( CompareResult result, bool ascendingOrder )
	{	// controls the direction of the compare result (for ascending/descending sorting)
		return ascendingOrder ? result : static_cast< CompareResult >( -(int)result );
	}


	// function version
	template< typename ValueT >
	inline CompareResult Compare_Scalar( const ValueT& left, const ValueT& right )
	{
		if ( left < right )
			return Less;
		else if ( right < left )
			return Greater;
		return Equal;
	}


	// functor version
	struct CompareValue
	{
		template< typename ValueT >
		CompareResult operator()( const ValueT& left, const ValueT& right ) const
		{
			return Compare_Scalar( left, right );
		}

		template< typename ValueT >
		CompareResult operator()( const ValueT* pLeft, const ValueT* pRight ) const
		{
			return Compare_Scalar( *pLeft, *pRight );
		}
	};


	// functor versions of is_a

	template< typename ObjectT >
	struct IsA
	{
		template< typename AnyType >
		bool operator()( const AnyType* pObject ) const
		{
			return is_a< ObjectT >( pObject );
		}
	};

	template< typename ObjectT >
	struct IsNotA
	{
		template< typename AnyType >
		bool operator()( const AnyType* pObject ) const
		{
			return !is_a< ObjectT >( pObject );
		}
	};
}


namespace pred
{
	abstract class BasePredicate				// abstract base for any predicate, i.e. that evaluates to bool result
	{
	public:
		// used in assertions in algorithms requiring a boolean predicate
		static bool IsBoolPred( void ) { return true; }
	};


	abstract class BaseComparator				// abstract base for any comparator function, i.e. that evaluates to CompareResult
	{
	public:
		// used in assertions in algorithms requiring a comparator functor
		static bool IsComparator( void ) { return true; }
	};
}


#endif // Compare_fwd_h
