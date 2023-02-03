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


#define NBSP '\xA0'		// non-breaking space


namespace str
{
	TCHAR* CopyTextToBuffer( TCHAR* pDestBuffer, const TCHAR* pText, size_t bufferSize, const TCHAR suffix[] = g_ellipsis );

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
}


namespace str
{
	// ex: query quoted sub-strings, or environment variables "abc%VAR1%ijk%VAR2%xyz"

	template< typename CharT >
	void QueryEnclosedItems( std::vector< std::basic_string<CharT> >& rItems, const CharT* pSource,
							 const CharT* pSepStart, const CharT* pSepEnd, bool keepSeps = true )
	{
		ASSERT_PTR( pSource );
		ASSERT_PTR( !str::IsEmpty( pSepStart ) );

		const str::CSequence<CharT> sepStart( pSepStart );
		const str::CSequence<CharT> sepEnd( str::IsEmpty( pSepEnd ) ? pSepStart : pSepEnd );

		typedef const CharT* Tconst_iterator;

		for ( Tconst_iterator itStart = str::begin( pSource ), itEnd = str::end( pSource ); ; )
		{
			Tconst_iterator itItemStart = std::search( itStart, itEnd, sepStart.Begin(), sepStart.End() );
			if ( itItemStart == itEnd )
				break;					// no more substrings
			Tconst_iterator itItemEnd = std::search( itItemStart + sepStart.m_length, itEnd, sepEnd.Begin(), sepEnd.End() );
			if ( itItemEnd == itEnd )
				break;					// substring not enclosed

			itStart = itItemEnd + sepEnd.m_length;

			if ( keepSeps )
				itItemEnd += sepEnd.m_length;
			else
				itItemStart += sepStart.m_length;

			rItems.push_back( std::basic_string<CharT>( itItemStart, std::distance( itItemStart, itItemEnd ) ) );
		}
	}


	// expand enclosed tags using KeyToValueFunc

	template< typename CharT, typename KeyToValueFunc >
	std::basic_string<CharT> ExpandKeysToValues( const CharT* pSource, const CharT* pSepStart, const CharT* pSepEnd,
												 KeyToValueFunc func, bool keepSeps = false )
	{
		ASSERT_PTR( pSource );
		ASSERT_PTR( !str::IsEmpty( pSepStart ) );

		const str::CSequence<CharT> sepStart( pSepStart );
		const str::CSequence<CharT> sepEnd( str::IsEmpty( pSepEnd ) ? pSepStart : pSepEnd );

		std::basic_string<CharT> output; output.reserve( str::GetLength( pSource ) * 2 );

		for ( str::const_iterator itStart = str::begin( pSource ), itEnd = str::end( pSource ); ; )
		{
			str::const_iterator itKeyStart = std::search( itStart, itEnd, sepStart.Begin(), sepStart.End() ), itKeyEnd = itEnd;
			if ( itKeyStart != itEnd )
				itKeyEnd = std::search( itKeyStart + sepStart.m_length, itEnd, sepEnd.Begin(), sepEnd.End() );

			if ( itKeyStart != itEnd && itKeyEnd != itEnd )
			{
				output += std::basic_string<CharT>( itStart, std::distance( itStart, itKeyStart ) );		// add leading text
				output += func( keepSeps
					? std::basic_string<CharT>( itKeyStart, itKeyEnd + sepEnd.m_length )
					: std::basic_string<CharT>( itKeyStart + sepStart.m_length, itKeyEnd ) );
			}
			else
				output += std::basic_string<CharT>( itStart, std::distance( itStart, itKeyEnd ) );

			itStart = itKeyEnd + sepEnd.m_length;
			if ( itStart >= itEnd )
				break;
		}
		return output;
	}
}


namespace code
{
	// FWD:
	template< typename CharT >
	size_t FindEnclosedIdentifier( size_t* pOutIdentLen, const std::basic_string<CharT>& text,
								   const str::CSequence<CharT>& openSep, const str::CSequence<CharT>& closeSep,
								   size_t offset /*= 0*/ );
}


namespace env
{
	// environment variables

	bool HasAnyVariable( const std::tstring& source );

	std::tstring GetVariableValue( const TCHAR varName[], const TCHAR* pDefaultValue = NULL );
	bool SetVariableValue( const TCHAR varName[], const TCHAR* pValue );

	std::tstring ExpandStrings( const TCHAR* pSource );			// expand "%WIN_VAR%" Windows environment variables
	std::tstring ExpandPaths( const TCHAR* pSource );	// expand "%WIN_VAR%" and "$(VC_MACRO_VAR)" - Windows and Visual Studio style environment variables
	size_t AddExpandedPaths( std::vector<fs::CPath>& rEvalPaths, const TCHAR* pSource, const TCHAR delim[] = _T(";") );		// add unique to rPaths

	std::tstring UnExpandPaths( const std::tstring& expanded, const TCHAR* pSource );


	template< typename StringT >
	size_t ReplaceEnvVar_VcMacroToWindows( StringT& rText, size_t varMaxCount = StringT::npos )
	{
		// replaces multiple occurences of e.g. "$(UTL_INCLUDE)" to "%UTL_INCLUDE%" - only for literals that resemble a C/C++ identifier
		typedef typename StringT::value_type TChar;

		const TChar _openSep[] = { '$', '(', '\0' };		// "$("
		const TChar _closeSep[] = { ')', '\0' };			// ")"
		const str::CSequence<TChar> openSep( _openSep );
		const str::CSequence<TChar> closeSep( _closeSep );

		const TChar winVarSep[] = { '%', '\0' };			// "%"

		size_t varCount = 0, identLen;

		for ( size_t pos = 0;
			  varCount != varMaxCount && ( pos = code::FindEnclosedIdentifier( &identLen, rText, openSep, closeSep, pos ) ) != StringT::npos;
			  ++varCount )
		{
			StringT winEnvVar;
			winEnvVar = winVarSep + rText.substr( pos + openSep.m_length, identLen ) + winVarSep;

			rText.replace( pos, openSep.m_length + identLen + closeSep.m_length, winEnvVar );
			pos += winEnvVar.length();
		}

		return varCount;
	}

	template< typename CharT >
	std::basic_string<CharT> GetReplaceEnvVar_VcMacroToWindows( const CharT* pSource, size_t varMaxCount = utl::npos )
	{
		std::basic_string<CharT> text( pSource );
		ReplaceEnvVar_VcMacroToWindows( text, varMaxCount );
		return text;
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
	bool ParseNumber<BYTE>( BYTE& rNumber, const std::tstring& text, size_t* pSkipLength, const std::locale& loc );

	template<>
	bool ParseNumber<signed char>( signed char& rNumber, const std::tstring& text, size_t* pSkipLength, const std::locale& loc );


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


namespace func
{
	struct AppendLine
	{
		AppendLine( std::tstring* pOutput, const TCHAR* pLineSep = _T("\n") ) : m_pOutput( pOutput ), m_pLineSep( pLineSep ) { ASSERT_PTR( m_pOutput ); }

		template< typename StringyT >
		void operator()( const StringyT& value )
		{
			stream::Tag( *m_pOutput, func::StringOf( value ), m_pLineSep );
		}
	public:
		std::tstring* m_pOutput;
		const TCHAR* m_pLineSep;
	};
}


namespace code
{
	std::tstring FormatEscapeSeq( const std::tstring& text, bool uiSeq = false );
	std::tstring ParseEscapeSeqs( const std::tstring& displayText, bool uiSeq = false );


	template< typename CharT >
	inline bool IsIdentifierChar( CharT chr, const std::locale& loc = str::GetUserLocale() )		// e.g. C/C++ identifier, or Windows environment variable identifier, etc
	{
		return '_' == chr || std::isalnum( chr, loc );
	}


	template< typename CharT >
	size_t FindEnclosedIdentifier( size_t* pOutIdentLen, const std::basic_string<CharT>& text,
								   const str::CSequence<CharT>& openSep, const str::CSequence<CharT>& closeSep,
								   size_t offset = 0 )
	{	// look for "<openSep>identifier<closeSep>" beginnng at offset
		ASSERT( !openSep.IsEmpty() );
		ASSERT( !closeSep.IsEmpty() );

		size_t identLen = 0;
		size_t sepPos = text.find( openSep.m_pStr, offset, openSep.m_length );

		if ( sepPos != utl::npos )
		{
			size_t identPos = sepPos + openSep.m_length, endPos = identPos;

			// skip identifier
			while ( endPos != text.length() && code::IsIdentifierChar( text[ endPos ] ) )
				++endPos;

			if ( endPos == text.length() )			// not ended in closeSep?
				sepPos = utl::npos;
			else if ( pred::Equal == text.compare( endPos, closeSep.m_length, closeSep.m_pStr ) )
			{
				identLen = endPos - identPos;
				endPos += closeSep.m_length;		// skip past closeSep
			}
			else
				return FindEnclosedIdentifier( pOutIdentLen, text, openSep, closeSep, identPos );		// continue searching past openSep for an enclosed identifier
		}

		utl::AssignPtr( pOutIdentLen, identLen );
		if ( 0 == identLen )		// empty identifier?
			return utl::npos;
		return sepPos;
	}

	template< typename CharT >
	size_t FindEnclosedIdentifier( size_t* pOutIdentLen, const std::basic_string<CharT>& text,
								   const CharT* pOpenSep, const CharT* pCloseSep,
								   size_t offset = 0 )
	{	// look for "<openSep>identifier<closeSep>" beginnng at offset
		const str::CSequence<CharT> openSep( pOpenSep );
		const str::CSequence<CharT> closeSep( pCloseSep );

		return FindEnclosedIdentifier( pOutIdentLen, text, openSep, closeSep, offset );
	}
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
