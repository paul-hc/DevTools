#ifndef StringUtilities_h
#define StringUtilities_h
#pragma once

#include "StringBase.h"
#include "StringCompare.h"


namespace str
{
	extern const TCHAR g_ellipsis[];
	extern const TCHAR g_paragraph[];

	template< typename CharType > const CharType* StdDelimiters( void );	// " \t"


	template< typename CharType, typename ContainerType >
	size_t Tokenize( ContainerType& rTokens, const CharType* pSource, const CharType delims[] = StdDelimiters< CharType >() )
	{
		ASSERT( pSource != NULL && delims != NULL );
		rTokens.clear();

		const CharType* pDelimsEnd = str::end( delims );
		size_t tokenCount = 0;
		bool inQuotes = false;
		std::tstring token;

		for ( const CharType* pChr = pSource; *pChr != '\0'; ++pChr )
		{
			if ( '\"' == *pChr )
				inQuotes = !inQuotes;

			if ( inQuotes || std::find( delims, pDelimsEnd, *pChr ) == pDelimsEnd )
				token += *pChr;
			else if ( !token.empty() )
			{
				rTokens.push_back( token );
				++tokenCount;
				token.clear();
			}
		}

		if ( !token.empty() )
		{	// do the last token...
			rTokens.push_back( token );
			++tokenCount;
		}

		return tokenCount;
	}


	template< typename CharType >
	bool StripPrefix( std::basic_string< CharType >& rText, const CharType prefix[], size_t prefixLen = std::string::npos )
	{
		if ( std::string::npos == prefixLen )
			prefixLen = GetLength( prefix );

		if ( prefixLen != 0 )
			if ( pred::Equal == rText.compare( 0, prefixLen, prefix ) )
			{
				rText.erase( 0, prefixLen );
				return true;		// changed
			}

		return false;
	}

	template< typename CharType >
	bool StripSuffix( std::basic_string< CharType >& rText, const CharType suffix[], size_t suffixLen = std::string::npos )
	{
		if ( std::string::npos == suffixLen )
			suffixLen = GetLength( suffix );

		if ( suffixLen != 0 )
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


#include <limits>
#include "Range.h"


namespace str
{
	template< typename CharType, typename ValueType >
	std::basic_string< CharType > FormatValue( const ValueType& value )
	{
		std::basic_ostringstream< CharType > oss;
		oss << value;
		return oss.str();
	}

	template< typename CharType, typename ValueType >
	bool ParseValue( ValueType& rValue, const std::basic_string< CharType >& text )
	{
		std::basic_istringstream< CharType > iss( text );
		iss >> std::noskipws >> rValue;		// read the entire string, including whitespaces
		return !iss.fail();
	}


	// specializations

	template< typename CharType >
	inline std::basic_string< CharType > FormatValue( const std::basic_string< CharType >& value )
	{
		return value;
	}

	template< typename CharType >
	inline bool ParseValue( std::basic_string< CharType >& rValue, const std::basic_string< CharType >& text )
	{
		rValue = text;
		return true;
	}


	template< typename CharType, typename ValueType >
	inline std::basic_string< CharType > FormatNameValue( const std::basic_string< CharType >& name, const ValueType& value, CharType sep = '=' )
	{
		return FormatNameValueSpec< CharType >( name, FormatValue< CharType >( value ), sep );
	}

	template< typename CharType, typename ValueType >
	bool ParseNameValue( std::basic_string< CharType >& rName, ValueType& rValue, const std::basic_string< CharType >& spec, CharType sep = '=' )
	{
		std::pair< CPart< CharType >, CPart< CharType > > partsPair;
		if ( !ParseNameValuePair< CharType >( partsPair, spec, sep ) )
			return false;

		rName = partsPair.first.ToString();
		return ParseValue< CharType >( rValue, partsPair.second.ToString() );
	}
}


namespace num
{
	const std::locale& GetEmptyLocale( void );		// empty locale (devoid of facets)


	template< typename ValueType >
	inline ValueType MinValue( void ) { return (std::numeric_limits< ValueType >::min)(); }

	// for double doesn't work (DBL_MIN is minimal positive value); in C++ 11 use lowest()
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

	double GetRounded( double number, unsigned int fractDigits );
	double GetWithPrecision( double number, unsigned int fractDigits );		// no rounding, just the zeroes additional digits
	double GetRoundingFactor( unsigned int fractDigits );					// fractDigits is precision

	inline std::tstring FormatDouble( double value, unsigned int precision, const std::locale& loc = GetEmptyLocale() ) { return FormatNumber( GetRounded( value, precision ), loc ); }


	template< typename ValueType >
	bool ParseNumber( ValueType& rNumber, const std::tstring& text, size_t* pSkipLength = NULL, const std::locale& loc = GetEmptyLocale() )
	{
		std::tistringstream iss( text );
		iss.imbue( loc );
		iss >> rNumber;
		if ( iss.fail() )
			return false;

		if ( pSkipLength != NULL )
			*pSkipLength = static_cast< size_t >( iss.tellg() );
		return true;
	}

	template<>
	bool ParseNumber< BYTE >( BYTE& rNumber, const std::tstring& text, size_t* pSkipLength, const std::locale& loc );

	template<>
	bool ParseNumber< signed char >( signed char& rNumber, const std::tstring& text, size_t* pSkipLength, const std::locale& loc );


	template< typename ValueType >
	std::tstring FormatHexNumber( ValueType value, const TCHAR* pFormat = _T("0x%X") )
	{
		return str::Format( pFormat, value );
	}

	template< typename ValueType >
	bool ParseHexNumber( ValueType& rNumber, const std::tstring& text, size_t* pSkipLength = NULL )
	{
		std::tistringstream iss( str::SkipHexPrefix( text.c_str(), str::IgnoreCase ) );
		size_t number;
		iss >> std::hex >> number;
		if ( iss.fail() )
			return false;

		rNumber = static_cast< ValueType >( number );
		if ( pSkipLength != NULL )
			*pSkipLength = static_cast< size_t >( iss.tellg() );
		return true;
	}

	bool StripFractionalZeros( std::tstring& rText, const std::locale& loc = str::GetUserLocale() );
}


class CEnumTags;


namespace num
{
	// file size formatting

	enum BytesUnit { Bytes, KiloBytes, MegaBytes, GigaBytes, TeraBytes, AutoBytes };

	const CEnumTags& GetTags_BytesUnit( void );

	std::tstring FormatFileSize( UINT64 byteFileSize, BytesUnit unit = AutoBytes, bool longUnitTag = false, const std::locale& loc = str::GetUserLocale() );		// use num::GetEmptyLocale() for no commas
	std::pair< double, BytesUnit > ConvertFileSize( UINT64 fileSize, BytesUnit toUnit = AutoBytes );
}


namespace num
{
	// advanced numeric algorithms

	template< typename IntType, typename StringT >
	bool EnwrapNumericSequence( Range< IntType >& rRange, const StringT& text )
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
	// line utilities
	std::string& ToWindowsLineEnds( std::string& rText );
	std::wstring& ToWindowsLineEnds( std::wstring& rText );
	std::string& ToUnixLineEnds( std::string& rText );
	std::wstring& ToUnixLineEnds( std::wstring& rText );


	template< typename CharType, typename StringT >
	inline void SplitLines( std::vector< StringT >& rItems, const CharType* pSource, const CharType* pLineEnd )
	{
		Split( rItems, pSource, pLineEnd );

		if ( !rItems.empty() && pred::IsEmpty()( rItems.back() ) )		// last item is empty (from a line-end terminator); pred::IsEmpty works with paths
			rItems.pop_back();
	}

	template< typename CharType, typename ContainerType >
	inline std::basic_string< CharType > JoinLines( const ContainerType& items, const CharType* pLineEnd )
	{
		std::basic_string< CharType > text = Join( items, pLineEnd );
		if ( items.size() > 1 )								// multiple lines
			text += pLineEnd;								// add a final line-end terminator to have a set of complete lines
		return text;
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

	bool IsAlphaNumericWord( const std::tstring& text, const std::locale& loc = str::GetUserLocale() );
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

	bool ParseValuePair( std::tstring& rValue, const TCHAR* pArg, const TCHAR* pNameList, TCHAR valueSep = _T('='), const TCHAR* pListDelims = _T("|") );
	bool ParseOptionalValuePair( std::tstring* pValue, const TCHAR* pArg, const TCHAR* pNameList, TCHAR valueSep = _T('='), const TCHAR* pListDelims = _T("|") );


	// command line

	template< typename ValueT >
	inline std::tstring Enquote( const ValueT& value, TCHAR quote = _T('\"') ) { return str::Enquote< std::tstring >( value, quote ); }

	template< typename ValueT >
	std::tstring AutoEnquote( const ValueT& value, TCHAR quote = _T('\"') )		// enquote only if it contains spaces
	{
		std::tstring text = str::ValueToString< std::tstring >( value );
		if ( text.find( _T(' ') ) != std::tstring::npos )
		{
			text.reserve( text.length() + 2 );
			text.insert( 0, 1, quote );
			text.append( 1, quote );
		}
		return text;
	}

	template< typename ValueT >
	bool AddCmd_Param( std::tstring& rCmdLine, const ValueT& param )
	{
		return stream::Tag( rCmdLine, AutoEnquote( param ), _T(" ") );
	}
}


#endif // StringUtilities_h
