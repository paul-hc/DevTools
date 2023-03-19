#ifndef StringIntuitiveCompare_h
#define StringIntuitiveCompare_h
#pragma once

#include "StringCompare.h"


namespace pred
{
	// intuitive order: case insensitive, compare numeric sequences by value
	//
	template< typename TranslateFunc >
	struct IntuitiveComparator
	{
		IntuitiveComparator( TranslateFunc translateFunc = TranslateFunc() )
			: m_evalChar( translateFunc )
			, m_compareCase( translateFunc )
			, m_compareIgnoreCase( translateFunc )
		{
		}

		template< typename CharT >
		CompareResult operator()( const CharT* pLeft, const CharT* pRight ) const
		{
			return Compare( pLeft, pRight );
		}

		template< typename CharT >
		CompareResult Compare( const CharT* pLeft, const CharT* pRight ) const
		{
			REQUIRE( pLeft != nullptr && pRight != nullptr );

			for ( size_t pos = 0; ; ++pos )
			{
				CharT chLeft = pLeft[ pos ];
				CharT chRight = pRight[ pos ];

				if ( s_isDigit( chLeft ) && s_isDigit( chRight ) )			// a sequence of digits on both operands?
				{
					CompareResult numResult = CompareNumericValues( pLeft + pos, pRight + pos );
					if ( numResult != Equal )
						return numResult;
				}

				if ( '\0' == chLeft && '\0' == chRight )
					return CompareTieBreak( pLeft, pRight );

				chLeft = m_evalChar( chLeft );
				chRight = m_evalChar( chRight );

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
			CompareResult result = m_compareIgnoreCase( pLeft, pRight );		// case-insensitive
			if ( Equal == result )
				result = m_compareCase( pLeft, pRight );				// they seem equal, but they may differ in case (translated)

			return result;
		}

		template< typename CharT >
		static CompareResult CompareNumericValues( const CharT* pLeft, const CharT* pRight )
		{
			REQUIRE( s_isDigit( *pLeft ) && s_isDigit( *pRight ) );

			// skip leading zeros for both operands to compare the number VALUES
			while ( '0' == *pLeft )
				++pLeft;

			while ( '0' == *pRight )
				++pRight;

			// compare numeric values; if equal, compare the count of digits

			CompareResult prevResult = Equal;				// store the previous comparion result since we don't know in advance which one is different

			for ( ; ; ++pLeft, ++pRight )
			{
				bool isDigitLeft = s_isDigit( *pLeft ), isDigitRight = s_isDigit( *pRight );

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
				else if ( '\0' == *pLeft && '\0' == *pRight )		// EOS on both sides
					break;
			}

			return prevResult;
		}
	private:
		typedef func::ToCharAs<func::C::ToLower, TranslateFunc> TLowerTranslatedFunc;

		TLowerTranslatedFunc m_evalChar;
		const func::StrCompareBase<TranslateFunc> m_compareCase;
		const func::StrCompareBase<TLowerTranslatedFunc> m_compareIgnoreCase;
		static const pred::IsDigit s_isDigit;
	};


	template< typename TranslateFunc >
	inline IntuitiveComparator<TranslateFunc> MakeIntuitiveComparator( TranslateFunc translateFunc ) { return IntuitiveComparator<TranslateFunc>( translateFunc ); }


	// define template static data-members

	template< typename TranslateFunc >
	const pred::IsDigit pred::IntuitiveComparator<TranslateFunc>::s_isDigit;
}


#endif // StringIntuitiveCompare_h
