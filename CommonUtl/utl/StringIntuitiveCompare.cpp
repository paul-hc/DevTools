
#include "stdafx.h"
#include "StringUtilities.h"
#include "ComparePredicates.h"


namespace str
{
	template< typename CharType >
	inline int CompareTieBreak( const CharType* pLeft, const CharType* pRight )
	{
		int result = CharTraits::CompareNoCase( pLeft, pRight );

		if ( 0 == result )
			result = CharTraits::Compare( pLeft, pRight ); // they seem equal, but they might differ in case

		return result;
	}


	template< typename CharType >
	int CompareDigitSequence( const CharType* pLeft, const CharType* pRight )
	{
		REQUIRE( CharTraits::IsDigit( *pLeft ) && CharTraits::IsDigit( *pRight ) );

		// numeric comparison: skip leading zeros for both operands
		while ( *pLeft == _T('0') )
			++pLeft;

		while ( *pRight == _T('0') )
			++pRight;

		// compare numeric values; if equal, compare the count of digits

		// store the previous comparion result since we don't know in advance which one is different
		int prevResult = 0;

		for ( ;; ++pLeft, ++pRight )
			if ( !CharTraits::IsDigit( *pLeft ) && !CharTraits::IsDigit( *pRight ) )
				return prevResult;
			else if ( !CharTraits::IsDigit( *pLeft ) )
				return -1;
			else if ( !CharTraits::IsDigit( *pRight ) )
				return 1;
			else if ( *pLeft < *pRight )
			{
				if ( 0 == prevResult )
					prevResult = -1;
			}
			else if ( *pLeft > *pRight )
			{
				if ( 0 == prevResult )
					prevResult = 1;
			}
			else if ( 0 == *pLeft && 0 == *pRight )
				return prevResult;
	}


	template< typename CharType >
	pred::CompareResult IntuitiveCompareImpl( const CharType* pLeft, const CharType* pRight )
	{
		REQUIRE( pLeft != NULL && pRight != NULL );

		CharType chLeft, chRight;
		int indexLeft = 0, indexRight = 0;
		int result;

		for ( ;; ++indexLeft, ++indexRight )
		{
			chLeft = pLeft[ indexLeft ];
			chRight = pRight[ indexRight ];

			// a sequence of digits on both operands?
			if ( CharTraits::IsDigit( chLeft ) && CharTraits::IsDigit( chRight ) )
				if ( ( result = CompareDigitSequence( pLeft + indexLeft, pRight + indexRight ) ) != 0 )
					return pred::ToCompareResult( result );

			if ( 0 == chLeft && 0 == chRight )
				return pred::ToCompareResult( CompareTieBreak( pLeft, pRight ) );

			chLeft = CharTraits::ToUpper( chLeft );
			chRight = CharTraits::ToUpper( chRight );

			if ( chLeft < chRight )
				return pred::Less;
			else if ( chLeft > chRight )
				return pred::Greater;
		}
	}


	/**
		case insensitive intuitive comparison - sequences of digits are compared numerically
	*/

	pred::CompareResult IntuitiveCompare( const char* pLeft, const char* pRight )
	{
		return IntuitiveCompareImpl( pLeft, pRight );
	}

	pred::CompareResult IntuitiveCompare( const wchar_t* pLeft, const wchar_t* pRight )
	{
		return IntuitiveCompareImpl( pLeft, pRight );
	}

} // namespace str
