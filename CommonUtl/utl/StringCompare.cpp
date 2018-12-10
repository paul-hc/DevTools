
#include "stdafx.h"
#include "StringCompare.h"
#include "ComparePredicates.h"


namespace func
{
	int ToNaturalChar::Translate( int charCode )
	{
		enum TranslatedCode		// natural order for punctuation characters
		{
			Dot = 1, Colon, SemiColon, Comma, Dash, Plus, Underbar,
			CurvedBraceB, CurvedBraceE,
			SquareBraceB, SquareBraceE,
			CurlyBraceB, CurlyBraceE,
			AngularBraceB, AngularBraceE
		};

		switch ( charCode )
		{
			case '.':	return Dot;
			case ':':	return Colon;
			case ';':	return SemiColon;
			case ',':	return Comma;
			case '-':	return Dash;
			case '+':	return Plus;
			case '_':	return Underbar;
			case '(':	return CurvedBraceB;
			case ')':	return CurvedBraceE;
			case '[':	return SquareBraceB;
			case ']':	return SquareBraceE;
			case '{':	return CurlyBraceB;
			case '}':	return CurlyBraceE;
			case '<':	return AngularBraceB;
			case '>':	return AngularBraceE;
		}
		return charCode;
	}
}


namespace str
{
	template< typename CharType, typename ToCharFunc >
	inline int CompareTieBreak( const CharType* pLeft, const CharType* pRight, ToCharFunc cvtFunc )
	{
		int result = str::CompareI( pLeft, pRight );					// case-insensitive
		if ( pred::Equal == result )
			result = str::_CompareN( pLeft, pRight, cvtFunc );			// they seem equal, but they might differ in case

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


	template< typename CharType, typename ToCharFunc >
	pred::CompareResult IntuitiveCompareImpl( const CharType* pLeft, const CharType* pRight, ToCharFunc cvtFunc )
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
				return pred::ToCompareResult( CompareTieBreak( pLeft, pRight, cvtFunc ) );

			chLeft = cvtFunc( CharTraits::ToUpper( chLeft ) );
			chRight = cvtFunc( CharTraits::ToUpper( chRight ) );

			if ( chLeft < chRight )
				return pred::Less;
			else if ( chLeft > chRight )
				return pred::Greater;
		}
	}


	// case insensitive intuitive comparison - sequences of digits are compared numerically

	pred::CompareResult IntuitiveCompare( const char* pLeft, const char* pRight )
	{
		return IntuitiveCompareImpl( pLeft, pRight, func::ToNaturalChar() );
	}

	pred::CompareResult IntuitiveCompare( const wchar_t* pLeft, const wchar_t* pRight )
	{
		return IntuitiveCompareImpl( pLeft, pRight, func::ToNaturalChar() );
	}

} // namespace str
