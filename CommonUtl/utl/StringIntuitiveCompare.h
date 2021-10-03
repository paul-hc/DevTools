#ifndef StringIntuitiveCompare_h
#define StringIntuitiveCompare_h
#pragma once

#include "StringCompare.h"


namespace pred
{
	// intuitive order: case insensitive, compare numeric sequences by value
	//
	template< typename TranslateFunc = ToCharFunc() >
	struct IntuitiveComparator
	{
		IntuitiveComparator( TranslateFunc translateFunc = TranslateFunc() ) : m_translateFunc( translateFunc ) {}

		template< typename CharT >
		CompareResult operator()( const CharT* pLeft, const CharT* pRight ) const
		{
			return Compare( pLeft, pRight );
		}

		template< typename CharT >
		CompareResult Compare( const CharT* pLeft, const CharT* pRight ) const
		{
			REQUIRE( pLeft != NULL && pRight != NULL );

			for ( size_t posLeft = 0, posRight = 0; ; ++posLeft, ++posRight )
			{
				CharT chLeft = pLeft[ posLeft ];
				CharT chRight = pRight[ posRight ];

				if ( str::CharTraits::IsDigit( chLeft ) && str::CharTraits::IsDigit( chRight ) )			// a sequence of digits on both operands?
				{
					CompareResult numResult = CompareNumericValues( pLeft + posLeft, pRight + posRight );
					if ( numResult != Equal )
						return numResult;
				}

				if ( 0 == chLeft && 0 == chRight )
					return CompareTieBreak( pLeft, pRight );

				chLeft = m_translateFunc( str::CharTraits::ToUpper( chLeft ) );
				chRight = m_translateFunc( str::CharTraits::ToUpper( chRight ) );

				if ( chLeft < chRight )
					return Less;
				else if ( chRight < chLeft )
					return Greater;
			}
		}
	private:
		template< typename CharT >
		inline CompareResult CompareTieBreak( const CharT* pLeft, const CharT* pRight ) const
		{
			CompareResult result = str::CharTraits::CompareI( pLeft, pRight );		// case-insensitive
			if ( Equal == result )
				result = str::_CompareN( pLeft, pRight, m_translateFunc );			// they seem equal, but they may differ translated in case

			return result;
		}

		template< typename CharT >
		static CompareResult CompareNumericValues( const CharT* pLeft, const CharT* pRight )
		{
			REQUIRE( str::CharTraits::IsDigit( *pLeft ) && str::CharTraits::IsDigit( *pRight ) );

			// skip leading zeros for both operands to compare the number VALUES
			while ( '0' == *pLeft )
				++pLeft;

			while ( '0' == *pRight )
				++pRight;

			// compare numeric values; if equal, compare the count of digits

			CompareResult prevResult = Equal;				// store the previous comparion result since we don't know in advance which one is different

			for ( ; ; ++pLeft, ++pRight )
			{
				bool isDigitLeft = str::CharTraits::IsDigit( *pLeft ), isDigitRight = str::CharTraits::IsDigit( *pRight );

				if ( !isDigitLeft && !isDigitRight )
					break;
				else if ( !isDigitLeft )
					return Less;							// numeric sequence shorter on the left -> smaller number
				else if ( !isDigitRight )
					return Greater;							// numeric sequence shorter on the right -> greater number
				else if ( *pLeft < *pRight )
				{
					if ( Equal == prevResult )
						prevResult = Less;
				}
				else if ( *pLeft > *pRight )
				{
					if ( Equal == prevResult )
						prevResult = Greater;
				}
				else if ( 0 == *pLeft && 0 == *pRight )		// EOS on both sides
					break;
			}

			return prevResult;
		}
	private:
		TranslateFunc m_translateFunc;
	};


	template< typename TranslateFunc >
	inline IntuitiveComparator< TranslateFunc > MakeIntuitiveComparator( TranslateFunc translateFunc ) { return IntuitiveComparator< TranslateFunc >( translateFunc ); }
}


#endif // StringIntuitiveCompare_h
