#ifndef Compare_fwd_h
#define Compare_fwd_h
#pragma once


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
		else
			return Equal;
	}


	inline CompareResult GetResultInOrder( CompareResult result, bool ascendingOrder )
	{	// controls the direction of the compare result (for ascending/descending sorting)
		return ascendingOrder ? result : static_cast< CompareResult >( -(int)result );
	}


	// function version
	template< typename Type >
	inline CompareResult Compare_Scalar( const Type& left, const Type& right )
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
		template< typename T >
		CompareResult operator()( const T& left, const T& right ) const
		{
			return Compare_Scalar( left, right );
		}
	};
}


#endif // Compare_fwd_h
