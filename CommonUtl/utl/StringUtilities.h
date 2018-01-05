#ifndef StringUtilities_h
#define StringUtilities_h
#pragma once

#include "ComparePredicates.h"
#include "StringBase.h"


namespace str
{
	int Tokenize( std::vector< std::tstring >& rTokens, const TCHAR* pSource, const TCHAR* pDelims = _T(" \t") );


	bool StripPrefix( std::tstring& rText, const TCHAR* pPrefix );
	bool StripSuffix( std::tstring& rText, const TCHAR* pSuffix );
	std::tstring Truncate( const std::tstring& text, size_t maxLength, bool useEllipsis = true );


	// search & replace

	size_t ReplaceDelimiters( std::tstring& rText, const TCHAR* pDelimiters, const TCHAR* pNewDelimiter );
	size_t EnsureSingleSpace( std::tstring& rText );


	// impl in StringIntuitiveCompare.cpp
	pred::CompareResult IntuitiveCompare( const char* pLeft, const char* pRight );
	pred::CompareResult IntuitiveCompare( const wchar_t* pLeft, const wchar_t* pRight );

	template< typename StringType >
	inline pred::CompareResult IntuitiveCompare( const StringType& left, const StringType& right )
	{
		return IntuitiveCompare( left.c_str(), right.c_str() );
	}


	// environment variables

	std::tstring ExpandEnvironmentStrings( const TCHAR* pSource );
	void QueryEnvironmentVariables( std::vector< std::tstring >& rVariables, const TCHAR* pSource );
	void ExpandEnvironmentVariables( std::vector< std::tstring >& rValues, const std::vector< std::tstring >& rVariables );

} //namespace str


namespace pred
{
	struct Straight
	{
		template< typename T >
		T operator()( const T& value ) const
		{
			return value;
		}
	};
}


namespace str
{
	template< typename CharType, typename TranslateCharFunc >
	std::pair< pred::CompareResult, size_t > BaseCompare( const CharType* pLeft, const CharType* pRight, TranslateCharFunc translateFunc, size_t count = std::tstring::npos )
	{
		int firstMismatch = 0;
		size_t matchLen = 0;
		if ( pLeft != pRight )
		{
			ASSERT( pLeft != NULL && pRight != NULL );

			while ( count-- != 0 &&
					0 == ( firstMismatch = translateFunc( *pLeft ) - translateFunc( *pRight ) ) &&
					*pLeft != 0 &&
					*pRight != 0 )
			{
				++pLeft;
				++pRight;
				++matchLen;
			}
		}
		else
			matchLen = str::length( pLeft );

		return std::make_pair( pred::ToCompareResult( firstMismatch ), matchLen );
	}

	template< typename CharType, typename TranslateCharFunc >
	inline pred::CompareResult CompareN( const CharType* pLeft, const CharType* pRight, TranslateCharFunc translateFunc, size_t count = std::tstring::npos )
	{
		return BaseCompare( pLeft, pRight, translateFunc, count ).first;
	}


	// pointer to character traits and predicates
	struct CharTraits
	{
		static bool IsDigit( char ch ) { return isdigit( (unsigned char)ch ) != 0; }
		static bool IsDigit( wchar_t ch ) { return iswdigit( ch ) != 0; }
		static char ToUpper( char ch ) { return (char)(unsigned char)toupper( (unsigned char)ch ); }
		static wchar_t ToUpper( wchar_t ch ) { return towupper( ch ); }

		static inline pred::CompareResult Compare( const char* pLeft, const char* pRight ) { return pred::ToCompareResult( strcmp( pLeft, pRight ) ); }
		static inline pred::CompareResult Compare( const wchar_t* pLeft, const wchar_t* pRight ) { return pred::ToCompareResult( wcscmp( pLeft, pRight ) ); }

		static inline pred::CompareResult CompareN( const char* pLeft, const char* pRight, size_t count ) { return pred::ToCompareResult( strncmp( pLeft, pRight, count ) ); }
		static inline pred::CompareResult CompareN( const wchar_t* pLeft, const wchar_t* pRight, size_t count ) { return pred::ToCompareResult( wcsncmp( pLeft, pRight, count ) ); }

		template< typename CharType >
		static pred::CompareResult CompareN_NoCase( const CharType* pLeft, const CharType* pRight, size_t count )
		{
			return str::CompareN( pLeft, pRight, &::tolower, count );
		}

		template< typename CharType >
		static inline int CompareNoCase( const CharType* pLeft, const CharType* pRight )
		{
			return CompareN_NoCase( pLeft, pRight, std::string::npos );
		}
	};


	template< typename StringType >
	static inline int Compare( const StringType& left, const StringType& right ) { return left.compare( right ); }

	template< typename StringType >
	static inline int CompareNoCase( const StringType& left, const StringType& right ) { return CharTraits::CompareNoCase( left.c_str(), right.c_str() ); }

	template< typename CharType >
	bool EqualsN( const CharType* pLeft, const CharType* pRight, size_t count, bool matchCase )
	{
		return matchCase
			? pred::Equal == CharTraits::CompareN( pLeft, pRight, count )
			: pred::Equal == CharTraits::CompareN_NoCase( pLeft, pRight, count );
	}

} //namespace str


namespace pred
{
	template< typename CharType, str::CaseType caseType >
	struct CharMatch
	{
		CharMatch( CharType chMatch ) : m_chMatch( str::Case == caseType ? chMatch : func::tolower( chMatch ) ) {}

		bool operator()( CharType chr ) const
		{
			return m_chMatch == ( str::Case == caseType ? chr : func::tolower( chr ) );
		}
	public:
		CharType m_chMatch;
	};


	template< str::CaseType caseType >
	struct CharEqual
	{
		template< typename CharType >
		bool operator()( CharType left, CharType right ) const
		{
			return str::Case == caseType ? ( left == right ) : ( func::tolower( left ) == func::tolower( right ) );
		}

		template< typename CharType >
		bool operator()( const CharType* pLeft, const CharType* pRight ) const
		{
			return str::Equals< caseType >( pLeft, pRight );
		}
	};
}


namespace str
{
	template< str::CaseType caseType, typename CharType >
	size_t Find( const CharType* pText, CharType chr, size_t startPos = 0 )
	{
		ASSERT( pText != 0 && startPos <= length( pText ) );

		const CharType* itEnd = end( pText );
		const CharType* itFound = std::find_if( begin( pText ) + startPos, itEnd, pred::CharMatch< CharType, caseType >( chr ) );
		return itFound != itEnd ? ( itFound - begin( pText ) ) : std::tstring::npos;
	}

	template< str::CaseType caseType, typename CharType >
	size_t Find( const CharType* pText, const CharType* pPart, size_t startPos = 0 )
	{
		ASSERT( pText != 0 && startPos <= length( pText ) );
		ASSERT( !str::IsEmpty( pPart ) );

		const CharType* itEnd = end( pText );
		const CharType* itFound = std::search( begin( pText ) + startPos, itEnd, begin( pPart ), end( pPart ), pred::CharEqual< caseType >() );
		return itFound != itEnd ? ( itFound - begin( pText ) ) : std::tstring::npos;
	}

	template< typename CharType >
	bool Matches( const CharType* pText, const CharType* pPart, bool matchCase, bool matchWhole )
	{
		if ( matchWhole )
			return matchCase
				? str::Equals< str::Case >( pText, pPart )
				: str::Equals< str::IgnoreCase >( pText, pPart );

		return matchCase
			? ( str::Find< str::Case >( pText, pPart ) != std::tstring::npos )
			: ( str::Find< str::IgnoreCase >( pText, pPart ) != std::tstring::npos );
	}


	// ex: query quoted sub-strings, or environment variables "abc%VAR1%ijk%VAR2%xyz"

	template< typename CharType >
	void QueryEnclosedItems( std::vector< std::basic_string< CharType > >& rItems, const CharType* pSource,
							 const CharType* pSepStart, const CharType* pSepEnd, bool keepSeps = true )
	{
		ASSERT_PTR( pSource );
		ASSERT_PTR( pSepStart );
		if ( NULL == pSepEnd )
			pSepEnd = pSepStart;

		const size_t sepStartLen = str::length( pSepStart ), sepEndLen = str::length( pSepEnd );

		for ( str::const_iterator itStart = str::begin( pSource ), itEnd = str::end( pSource ); ; )
		{
			str::const_iterator itItemStart = std::search( itStart, itEnd, pSepStart, pSepStart + sepStartLen );
			if ( itItemStart == itEnd )
				break;					// no more substrings
			str::const_iterator itItemEnd = std::search( itItemStart + sepStartLen, itEnd, pSepEnd, pSepEnd + sepEndLen );
			if ( itItemEnd == itEnd )
				break;					// substring not enclosed

			itStart = itItemEnd + sepEndLen;

			if ( keepSeps )
				itItemEnd += sepEndLen;
			else
				itItemStart += sepStartLen;

			rItems.push_back( std::basic_string< CharType >( itItemStart, std::distance( itItemStart, itItemEnd ) ) );
		}
	}


	// ex: query quoted sub-strings, or environment variables "abc%VAR1%ijk%VAR2%xyz"

	template< typename CharType, typename KeyToValueFunc >
	std::basic_string< CharType > ExpandKeysToValues( const CharType* pSource, const CharType* pSepStart, const CharType* pSepEnd,
													  KeyToValueFunc func, bool keepSeps = false )
	{
		ASSERT_PTR( pSource );
		ASSERT_PTR( pSepStart );
		if ( NULL == pSepEnd )
			pSepEnd = pSepStart;

		std::basic_string< CharType > output; output.reserve( str::length( pSource ) * 2 );
		const size_t sepStartLen = str::length( pSepStart ), sepEndLen = str::length( pSepEnd );

		for ( str::const_iterator itStart = str::begin( pSource ), itEnd = str::end( pSource ); ; )
		{
			str::const_iterator itKeyStart = std::search( itStart, itEnd, pSepStart, pSepStart + sepStartLen ), itKeyEnd = itEnd;
			if ( itKeyStart != itEnd )
				itKeyEnd = std::search( itKeyStart + sepStartLen, itEnd, pSepEnd, pSepEnd + sepEndLen );

			if ( itKeyStart != itEnd && itKeyEnd != itEnd )
			{
				output += std::basic_string< CharType >( itStart, std::distance( itStart, itKeyStart ) );		// add leading text
				output += func( keepSeps
					? std::basic_string< CharType >( itKeyStart, itKeyEnd + sepEndLen )
					: std::basic_string< CharType >( itKeyStart + sepStartLen, itKeyEnd ) );
			}
			else
				output += std::basic_string< CharType >( itStart, std::distance( itStart, itKeyEnd ) );

			itStart = itKeyEnd + sepEndLen;
			if ( itStart >= itEnd )
				break;
		}
		return output;
	}
}


namespace stream
{
	void Tag( std::tstring& rOutput, const std::tstring& tag, const TCHAR* pSep );
}


#include <limits>
#include "Range.h"


namespace num
{
	const std::locale& GetEmptyLocale( void );		// empty locale (devoid of facets)


	template< typename ValueType >
	inline ValueType MinValue( void ) { return (std::numeric_limits< ValueType >::min)(); }

	// for double  doesn't work (DBL_MIN is minimal positive value); in C++11 use lowest()
	template<>
	inline double MinValue< double >( void ) { return -(std::numeric_limits< double >::max)(); }	// min doesn't work (DBL_MIN is minimal positive value)

	template< typename ValueType >
	inline ValueType MaxValue( void ) { return (std::numeric_limits< ValueType >::max)(); }


	template< typename ValueType >		// [0, MAX]
	inline Range< ValueType > PositiveRange( void ) { return Range< ValueType >( 0, MaxValue< ValueType >() ); }

	template< typename ValueType >		// [MIN, 0]
	inline Range< ValueType > NegativeRange( void ) { return Range< ValueType >( MinValue< ValueType >(), 0 ); }

	template< typename ValueType >		// [MIN, MAX]
	inline Range< ValueType > FullRange( void ) { return Range< ValueType >( MinValue< ValueType >(), MaxValue< ValueType >() ); }


	template< typename ValueType >
	std::tstring FormatNumber( ValueType value, const std::locale& loc = GetEmptyLocale() )
	{
		std::tostringstream oss;
		oss.imbue( loc );
		oss << value;
		return oss.str();
	}

	std::tstring FormatNumber( double value, const std::locale& loc = GetEmptyLocale() );

	template< typename ValueType >
	bool ParseNumber( ValueType& rNumber, const std::tstring& text, const std::locale& loc = GetEmptyLocale() )
	{
		std::tistringstream iss( text );
		iss.imbue( loc );
		iss >> rNumber;
		return !iss.fail();
	}

	template<>
	inline bool ParseNumber< BYTE >( BYTE& rNumber, const std::tstring& text, const std::locale& loc )
	{
		UINT number;
		if ( !ParseNumber( number, text, loc ) || number > 255 )
			return false;
		rNumber = static_cast< BYTE >( number );
		return true;
	}


	const TCHAR* SkipHexPrefix( const TCHAR* pText );

	template< typename ValueType >
	std::tstring FormatHexNumber( ValueType value, const TCHAR* pFormat = _T("0x%X") )
	{
		return str::Format( pFormat, value );
	}

	template< typename ValueType >
	bool ParseHexNumber( ValueType& rNumber, const std::tstring& text )
	{
		std::tistringstream iss( SkipHexPrefix( text.c_str() ) );
		iss >> std::hex >> rNumber;
		return !iss.fail();
	}
}


namespace num
{
	// advanced numeric algorithms

	template< typename IntType, typename StringType >
	bool EnwrapNumericSequence( Range< IntType >& rRange, const StringType& text )
	{
		IntType len = static_cast< IntType >( text.length() );
		if ( text.empty() || rRange.m_start >= len || !str::CharTraits::IsDigit( text[ rRange.m_start ] ) )
			return false;						// no text to modify

		while ( rRange.m_start != 0 && str::CharTraits::IsDigit( text[ rRange.m_start - 1 ] ) )
			--rRange.m_start;

		rRange.m_end = rRange.m_start;		// rewind to ensure a contiguous sequence of digits
		while ( rRange.m_end != len && str::CharTraits::IsDigit( text[ rRange.m_end ] ) )
			++rRange.m_end;

		ENSURE( str::CharTraits::IsDigit( text[ rRange.m_start ] ) );
		return true;
	}


	Range< size_t > FindNumericSequence( const std::tstring& text, size_t pos = 0 );
	size_t EnsureUniformZeroPadding( std::vector< std::tstring >& rItems );			// returns max count of numbers found for all items
}



namespace str
{
	inline std::string& ToWindowsLineEnds( std::string& rText )
	{
		str::Replace( rText, "\r\n", "\n" );
		str::Replace( rText, "\n", "\r\n" );
		return rText;
	}

	inline std::wstring& ToWindowsLineEnds( std::wstring& rText )
	{
		str::Replace( rText, L"\r\n", L"\n" );
		str::Replace( rText, L"\n", L"\r\n" );
		return rText;
	}

	inline std::string& ToUnixLineEnds( std::string& rText )
	{
		str::Replace( rText, "\r\n", "\n" );
		return rText;
	}

	inline std::wstring& ToUnixLineEnds( std::wstring& rText )
	{
		str::Replace( rText, L"\r\n", L"\n" );
		return rText;
	}
}


namespace code
{
	std::tstring FormatEscapeSeq( const std::tstring& text, bool uiSeq = false );
	std::tstring ParseEscapeSeqs( const std::tstring& displayText, bool uiSeq = false );
}


namespace ui
{
	inline std::tstring FormatEscapeSeq( const std::tstring& text )
	{
		return code::FormatEscapeSeq( text, true );
	}

	inline std::tstring ParseEscapeSeqs( const std::tstring& displayText )
	{
		return code::ParseEscapeSeqs( displayText, true );
	}
}


namespace word
{
	enum WordStatus { WordStart, WordCore, WordEnd, Whitespace };

	WordStatus GetWordStatus( const std::tstring& text, size_t pos, const std::locale& loc = str::GetUserLocale() );
	size_t FindPrevWordBreak( const std::tstring& text, size_t pos, const std::locale& loc = str::GetUserLocale() );
	size_t FindNextWordBreak( const std::tstring& text, size_t pos, const std::locale& loc = str::GetUserLocale() );
}


namespace app
{
	bool HasCommandLineOption( const TCHAR* pOption, std::tstring* pValue = NULL );
}


namespace arg
{
	bool IsSwitch( const TCHAR* pArg );					// "-arg" or "/arg"
	const TCHAR* GetSwitch( const TCHAR* pArg );		// return "arg" for "-arg" or "/arg"

	bool Equals( const TCHAR* pArg, const TCHAR* pMatch );
	bool EqualsAnyOf( const TCHAR* pArg, const TCHAR* pMatchList, const TCHAR* pListDelims = _T("|") );

	bool StartsWith( const TCHAR* pArg, const TCHAR* pPrefix, size_t count = std::tstring::npos );
	bool StartsWithAnyOf( const TCHAR* pArg, const TCHAR* pPrefixList, const TCHAR* pListDelims = _T("|") );

	bool ParseValuePair( std::tstring& rValue, const TCHAR* pPairArg, const TCHAR* pNameList, TCHAR valueSep = _T('='), const TCHAR* pListDelims = _T("|") );
}


#endif // StringUtilities_h
