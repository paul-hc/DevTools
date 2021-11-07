#ifndef StringUtilities_h
#define StringUtilities_h
#pragma once

#include "StringBase.h"
#include "StringCompare.h"


namespace str
{
	extern const TCHAR g_ellipsis[];
	extern const TCHAR g_paragraph[];

	template< typename CharT > const CharT* StdDelimiters( void );	// " \t"


	template< typename CharT, typename ContainerT >
	size_t Tokenize( ContainerT& rTokens, const CharT* pSource, const CharT delims[] = StdDelimiters<CharT>() )
	{
		ASSERT( pSource != NULL && delims != NULL );
		rTokens.clear();

		const CharT* pDelimsEnd = str::end( delims );
		size_t tokenCount = 0;
		bool inQuotes = false;
		std::tstring token;

		for ( const CharT* pChr = pSource; *pChr != '\0'; ++pChr )
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


	template< typename CharT >
	bool StripPrefix( std::basic_string<CharT>& rText, const CharT prefix[], size_t prefixLen = std::string::npos )
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

	template< typename CharT >
	bool StripSuffix( std::basic_string<CharT>& rText, const CharT suffix[], size_t suffixLen = std::string::npos )
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
}


#define NBSP '\xA0'


namespace str
{
	std::tstring& Truncate( std::tstring& rText, size_t maxLen, const TCHAR suffix[] = g_ellipsis, bool atEnd = true );
	std::tstring& SingleLine( std::tstring& rText, size_t maxLen = utl::npos, const TCHAR sepLineEnd[] = g_paragraph );

	inline std::tstring FormatTruncate( std::tstring text, size_t maxLen, const TCHAR suffix[] = g_ellipsis, bool atEnd = true ) { return Truncate( text, maxLen, suffix, atEnd ); }
	inline std::tstring FormatSingleLine( std::tstring text, size_t maxLen = utl::npos, const TCHAR sepLineEnd[] = g_paragraph ) { return SingleLine( text, maxLen, sepLineEnd ); }

	template< typename StringT >
	inline StringT ToNonBreakingSpace( const StringT& text )
	{
		StringT out = text;
		std::replace( out.begin(), out.end(), StringT::value_type( ' ' ), StringT::value_type( NBSP ) );
		return out;
	}


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

	template< typename CharT >
	void QueryEnclosedItems( std::vector< std::basic_string<CharT> >& rItems, const CharT* pSource,
							 const CharT* pSepStart, const CharT* pSepEnd, bool keepSeps = true )
	{
		ASSERT_PTR( pSource );
		ASSERT_PTR( pSepStart );
		if ( NULL == pSepEnd )
			pSepEnd = pSepStart;

		const size_t sepStartLen = str::GetLength( pSepStart ), sepEndLen = str::GetLength( pSepEnd );

		typedef const CharT* const_iterator;

		for ( const_iterator itStart = str::begin( pSource ), itEnd = str::end( pSource ); ; )
		{
			const_iterator itItemStart = std::search( itStart, itEnd, pSepStart, pSepStart + sepStartLen );
			if ( itItemStart == itEnd )
				break;					// no more substrings
			const_iterator itItemEnd = std::search( itItemStart + sepStartLen, itEnd, pSepEnd, pSepEnd + sepEndLen );
			if ( itItemEnd == itEnd )
				break;					// substring not enclosed

			itStart = itItemEnd + sepEndLen;

			if ( keepSeps )
				itItemEnd += sepEndLen;
			else
				itItemStart += sepStartLen;

			rItems.push_back( std::basic_string<CharT>( itItemStart, std::distance( itItemStart, itItemEnd ) ) );
		}
	}


	// ex: query quoted sub-strings, or environment variables "abc%VAR1%ijk%VAR2%xyz"

	template< typename CharT, typename KeyToValueFunc >
	std::basic_string<CharT> ExpandKeysToValues( const CharT* pSource, const CharT* pSepStart, const CharT* pSepEnd,
												 KeyToValueFunc func, bool keepSeps = false )
	{
		ASSERT_PTR( pSource );
		ASSERT_PTR( pSepStart );
		if ( NULL == pSepEnd )
			pSepEnd = pSepStart;

		std::basic_string<CharT> output; output.reserve( str::GetLength( pSource ) * 2 );
		const size_t sepStartLen = str::GetLength( pSepStart ), sepEndLen = str::GetLength( pSepEnd );

		for ( str::const_iterator itStart = str::begin( pSource ), itEnd = str::end( pSource ); ; )
		{
			str::const_iterator itKeyStart = std::search( itStart, itEnd, pSepStart, pSepStart + sepStartLen ), itKeyEnd = itEnd;
			if ( itKeyStart != itEnd )
				itKeyEnd = std::search( itKeyStart + sepStartLen, itEnd, pSepEnd, pSepEnd + sepEndLen );

			if ( itKeyStart != itEnd && itKeyEnd != itEnd )
			{
				output += std::basic_string<CharT>( itStart, std::distance( itStart, itKeyStart ) );		// add leading text
				output += func( keepSeps
					? std::basic_string<CharT>( itKeyStart, itKeyEnd + sepEndLen )
					: std::basic_string<CharT>( itKeyStart + sepStartLen, itKeyEnd ) );
			}
			else
				output += std::basic_string<CharT>( itStart, std::distance( itStart, itKeyEnd ) );

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
	template< typename CharT, typename ValueT >
	std::basic_string<CharT> FormatValue( const ValueT& value )
	{
		std::basic_ostringstream<CharT> oss;
		oss << value;
		return oss.str();
	}

	template< typename CharT, typename ValueT >
	bool ParseValue( ValueT& rValue, const std::basic_string<CharT>& text )
	{
		std::basic_istringstream<CharT> iss( text );
		iss >> std::noskipws >> rValue;		// read the entire string, including whitespaces
		return !iss.fail();
	}


	// specializations

	template< typename CharT >
	inline std::basic_string<CharT> FormatValue( const std::basic_string<CharT>& value )
	{
		return value;
	}

	template< typename CharT >
	inline bool ParseValue( std::basic_string<CharT>& rValue, const std::basic_string<CharT>& text )
	{
		rValue = text;
		return true;
	}


	template< typename CharT, typename ValueT >
	inline std::basic_string<CharT> FormatNameValue( const std::basic_string<CharT>& name, const ValueT& value, CharT sep = '=' )
	{
		return FormatNameValueSpec<CharT>( name, FormatValue<CharT>( value ), sep );
	}

	template< typename CharT, typename ValueT >
	bool ParseNameValue( std::basic_string<CharT>& rName, ValueT& rValue, const std::basic_string<CharT>& spec, CharT sep = '=' )
	{
		std::pair<CPart<CharT>, CPart<CharT>> partsPair;
		if ( !ParseNameValuePair<CharT>( partsPair, spec, sep ) )
			return false;

		rName = partsPair.first.ToString();
		return ParseValue<CharT>( rValue, partsPair.second.ToString() );
	}
}


namespace num
{
	const std::locale& GetEmptyLocale( void );		// empty locale (devoid of facets)


	template< typename ValueT >
	inline ValueT MinValue( void ) { return (std::numeric_limits< ValueT >::min)(); }

	// for double doesn't work (DBL_MIN is minimal positive value); in C++ 11 use lowest()
	template<>
	inline double MinValue< double >( void ) { return -(std::numeric_limits< double >::max)(); }	// min doesn't work (DBL_MIN is minimal positive value)

	template< typename ValueT >
	inline ValueT MaxValue( void ) { return (std::numeric_limits< ValueT >::max)(); }


	template< typename ValueT >		// [0, MAX]
	inline Range<ValueT> PositiveRange( void ) { return Range<ValueT>( 0, MaxValue< ValueT >() ); }

	template< typename ValueT >		// [MIN, 0]
	inline Range<ValueT> NegativeRange( void ) { return Range<ValueT>( MinValue< ValueT >(), 0 ); }

	template< typename ValueT >		// [MIN, MAX]
	inline Range<ValueT> FullRange( void ) { return Range<ValueT>( MinValue< ValueT >(), MaxValue< ValueT >() ); }


	template< typename ValueT >
	std::tstring FormatNumber( ValueT value, const std::locale& loc = GetEmptyLocale() )
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


	template< typename ValueT >
	bool ParseNumber( ValueT& rNumber, const std::tstring& text, size_t* pSkipLength = NULL, const std::locale& loc = GetEmptyLocale() )
	{
		std::tistringstream iss( text );
		iss.imbue( loc );
		iss >> rNumber;
		if ( iss.fail() )
			return false;

		if ( pSkipLength != NULL )
			*pSkipLength = static_cast<size_t>( iss.tellg() );
		return true;
	}

	template<>
	bool ParseNumber< BYTE >( BYTE& rNumber, const std::tstring& text, size_t* pSkipLength, const std::locale& loc );

	template<>
	bool ParseNumber< signed char >( signed char& rNumber, const std::tstring& text, size_t* pSkipLength, const std::locale& loc );


	template< typename ValueT >
	std::tstring FormatHexNumber( ValueT value, const TCHAR* pFormat = _T("0x%X") )
	{
		return str::Format( pFormat, value );
	}

	template< typename ValueT >
	bool ParseHexNumber( ValueT& rNumber, const std::tstring& text, size_t* pSkipLength = NULL )
	{
		std::tistringstream iss( str::SkipHexPrefix( text.c_str(), str::IgnoreCase ) );
		size_t number;
		iss >> std::hex >> number;
		if ( iss.fail() )
			return false;

		rNumber = static_cast<ValueT>( number );
		if ( pSkipLength != NULL )
			*pSkipLength = static_cast<size_t>( iss.tellg() );
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
	std::tstring FormatFileSizeAsPair( UINT64 byteFileSize, bool longUnitTag = false );					// e.g. "6.89 MB (7,224,287 bytes)"

	std::pair<double, BytesUnit> ConvertFileSize( UINT64 fileSize, BytesUnit toUnit = AutoBytes );
}


namespace num
{
	// advanced numeric algorithms

	template< typename IntT, typename StringT >
	bool EnwrapNumericSequence( Range<IntT>& rRange, const StringT& text )
	{
		IntT len = static_cast<IntT>( text.length() );
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


	Range<size_t> FindNumericSequence( const std::tstring& text, size_t pos = 0 );
	size_t EnsureUniformZeroPadding( std::vector< std::tstring >& rItems );			// returns max count of numbers found for all items
}


namespace str
{
	// line utilities
	std::string& ToWindowsLineEnds( std::string& rText );
	std::wstring& ToWindowsLineEnds( std::wstring& rText );
	std::string& ToUnixLineEnds( std::string& rText );
	std::wstring& ToUnixLineEnds( std::wstring& rText );


	template< typename CharT, typename StringT >
	inline void SplitLines( std::vector< StringT >& rItems, const CharT* pSource, const CharT* pLineEnd )
	{
		Split( rItems, pSource, pLineEnd );

		if ( !rItems.empty() && pred::IsEmpty()( rItems.back() ) )		// last item is empty (from a line-end terminator); pred::IsEmpty works with paths
			rItems.pop_back();
	}

	template< typename CharT, typename ContainerT >
	inline std::basic_string<CharT> JoinLines( const ContainerT& items, const CharT* pLineEnd )
	{
		std::basic_string<CharT> text = Join( items, pLineEnd );
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
	inline std::tstring Enquote( const ValueT& value, TCHAR quote = _T('\"') )
	{
		return str::EnquoteStr( str::ValueToString<std::tstring>( value ), quote, true );
	}

	template< typename ValueT >
	std::tstring AutoEnquote( const ValueT& value, TCHAR quote = _T('\"') )		// enquote only if it contains spaces
	{
		std::tstring text = str::ValueToString<std::tstring>( value );
		if ( std::tstring::npos == text.find( _T(' ') ) )
			return text;

		return str::EnquoteStr( text, quote, true );
	}

	template< typename ValueT >
	bool AddCmd_Param( std::tstring& rCmdLine, const ValueT& param )
	{
		return stream::Tag( rCmdLine, AutoEnquote( param ), _T(" ") );
	}
}


#endif // StringUtilities_h
