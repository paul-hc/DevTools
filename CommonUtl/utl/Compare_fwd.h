#ifndef Compare_fwd_h
#define Compare_fwd_h
#pragma once


enum SortType { NoSort, SortAscending, SortDescending };


namespace func
{
	struct ToSelf
	{
		template< typename ValueT >
		inline const ValueT& operator()( const ValueT& value ) const { return value; }
	};
}


namespace pred
{
	enum CompareResult { Less = -1, Equal, Greater };


	template< typename DiffType >
	inline CompareResult ToCompareResult( DiffType difference )
	{
		return (pred::CompareResult)( ( -difference < 0 ) - ( difference < 0 ) );	// (if positive) - (if negative) generates branchless code
	}


	inline CompareResult GetInvertedResult( CompareResult result )
	{	// reverses the direction of the compare result (for descending sorting)
		return static_cast<CompareResult>( -(int)result );
	}

	inline CompareResult GetResultInOrder( CompareResult result, bool ascendingOrder )
	{	// controls the direction of the compare result (for ascending/descending sorting)
		return ascendingOrder ? result : static_cast<CompareResult>( -(int)result );
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

	// FWD-declare explicit instantiations:
	template<>
	CompareResult Compare_Scalar<std::string>( const std::string& left, const std::string& right );			// by default sort std::string in natural order

	template<>
	CompareResult Compare_Scalar<std::wstring>( const std::wstring& left, const std::wstring& right );		// by default sort std::string in natural order


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
			return is_a<ObjectT>( pObject );
		}
	};

	template< typename ObjectT >
	struct IsNotA
	{
		template< typename AnyType >
		bool operator()( const AnyType* pObject ) const
		{
			return !is_a<ObjectT>( pObject );
		}
	};


	// predicate that's always true

	struct True
	{
		template< typename ValueT >
		bool operator()( const ValueT* ) const { return true; }

		template< typename ValueT >
		bool operator()( const ValueT& ) const { return true; }
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
