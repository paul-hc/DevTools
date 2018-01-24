#ifndef StringUtilities_h
#define StringUtilities_h
#pragma once

#include "StringBase.h"
#include "StringCompare.h"


namespace str
{
	extern const TCHAR g_ellipsis[];
	extern const TCHAR g_paragraph[];


	int Tokenize( std::vector< std::tstring >& rTokens, const TCHAR* pSource, const TCHAR* pDelims = _T(" \t") );


	template< typename CharType >
	bool StripPrefix( std::basic_string< CharType >& rText, const CharType prefix[] )
	{
		if ( size_t prefixLen = GetLength( prefix ) )
			if ( pred::Equal == rText.compare( 0, prefixLen, prefix ) )
			{
				rText.erase( 0, prefixLen );
				return true;		// changed
			}

		return false;
	}

	template< typename CharType >
	bool StripSuffix( std::basic_string< CharType >& rText, const CharType suffix[] )
	{
		if ( size_t suffixLen = GetLength( suffix ) )
		{
			size_t suffixPos = rText.length();
			if ( suffixPos >= suffixLen )
			{
				suffixPos -= suffixLen;
				if ( pred::Equal == rText.compare( suffixPos, suffixLen, suffix ) )
				{
					rText.erase( suffixPos, suffixLen );
					return true;		// changed
				}
			}
		}

		return false;
	}


	std::tstring& Truncate( std::tstring& rText, size_t maxLen, const TCHAR suffix[] = g_ellipsis, bool atEnd = true );
	std::tstring& SingleLine( std::tstring& rText, size_t maxLen = utl::npos, const TCHAR sepLineEnd[] = g_paragraph );

	inline std::tstring FormatTruncate( std::tstring text, size_t maxLen, const TCHAR suffix[] = g_ellipsis, bool atEnd = true ) { return Truncate( text, maxLen, suffix, atEnd ); }
	inline std::tstring FormatSingleLine( std::tstring text, size_t maxLen = utl::npos, const TCHAR sepLineEnd[] = g_paragraph ) { return SingleLine( text, maxLen, sepLineEnd ); }


	// search & replace

	size_t ReplaceDelimiters( std::tstring& rText, const TCHAR* pDelimiters, const TCHAR* pNewDelimiter );
	size_t EnsureSingleSpace( std::tstring& rText );


	// environment variables

	std::tstring ExpandEnvironmentStrings( const TCHAR* pSource );
	void QueryEnvironmentVariables( std::vector< std::tstring >& rVariables, const TCHAR* pSource );
	void ExpandEnvironmentVariables( std::vector< std::tstring >& rValues, const std::vector< std::tstring >& rVariables );
}


namespace str
{
	// ex: query quoted sub-strings, or environment variables "abc%VAR1%ijk%VAR2%xyz"

	template< typename CharType >
	void QueryEnclosedItems( std::vector< std::basic_string< CharType > >& rItems, const CharType* pSource,
							 const CharType* pSepStart, const CharType* pSepEnd, bool keepSeps = true )
	{
		ASSERT_PTR( pSource );
		ASSERT_PTR( pSepStart );
		if ( NULL == pSepEnd )
			pSepEnd = pSepStart;

		const size_t sepStartLen = str::GetLength( pSepStart ), sepEndLen = str::GetLength( pSepEnd );

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

		std::basic_string< CharType > output; output.reserve( str::GetLength( pSource ) * 2 );
		const size_t sepStartLen = str::GetLength( pSepStart ), sepEndLen = str::GetLength( pSepEnd );

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
	std::string& ToWindowsLineEnds( std::string& rText );
	std::wstring& ToWindowsLineEnds( std::wstring& rText );
	std::string& ToUnixLineEnds( std::string& rText );
	std::wstring& ToUnixLineEnds( std::wstring& rText );
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
