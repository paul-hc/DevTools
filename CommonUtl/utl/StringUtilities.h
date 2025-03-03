#ifndef StringUtilities_h
#define StringUtilities_h
#pragma once

#include "StringBase.h"
#include "StringCompare.h"


namespace str
{
	extern const TCHAR g_ellipsis[];
	extern const TCHAR g_paragraph[];

	template< typename CharT >
	const CharT* StdBlanks( void );		// " \t"


	template< typename CharT, typename OutIteratorT >
	void TokenizeOut( OUT OutIteratorT itOutTokens, const CharT* pSource, const CharT* pDelims )
	{
		ASSERT( pSource != nullptr && pDelims != nullptr );

		const CharT* pDelimsEnd = str::end( pDelims );
		bool inQuotes = false;
		std::tstring token;

		for ( const CharT* pChr = pSource; *pChr != '\0'; ++pChr )
		{
			if ( '\"' == *pChr )
				inQuotes = !inQuotes;

			if ( inQuotes || std::find( pDelims, pDelimsEnd, *pChr ) == pDelimsEnd )
				token += *pChr;
			else if ( !token.empty() )
			{
				*itOutTokens++ = token;
				token.clear();
			}
		}

		if ( !token.empty() )
			*itOutTokens = token;		// do the last token...
	}

	template< typename CharT, typename VectLikeT >
	inline size_t Tokenize( OUT VectLikeT& rTokens, const CharT* pSource, const CharT* pDelims = StdBlanks<CharT>() )
	{
		rTokens.clear();
		TokenizeOut( std::back_inserter( rTokens ), pSource, pDelims );
		return rTokens.size();
	}


	template< typename CharT >
	bool StripPrefix( IN OUT std::basic_string<CharT>& rText, const CharT prefix[], size_t prefixLen = std::string::npos )
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
	bool StripSuffix( IN OUT std::basic_string<CharT>& rText, const CharT suffix[], size_t suffixLen = std::string::npos )
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


#define NBSP '\xA0'		// non-breaking space: ' ' (special space character)


namespace str
{
	TCHAR* CopyTextToBuffer( OUT TCHAR* pDestBuffer, const TCHAR* pText, size_t bufferSize, const TCHAR suffix[] = g_ellipsis );

	std::tstring& Truncate( IN OUT std::tstring& rText, size_t maxLen, const TCHAR suffix[] = g_ellipsis, bool atEnd = true );
	std::tstring& SingleLine( IN OUT std::tstring& rText, size_t maxLen = utl::npos, const TCHAR sepLineEnd[] = g_paragraph );

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

	size_t ReplaceDelimiters( IN OUT std::tstring& rText, const TCHAR* pDelimiters, const TCHAR* pNewDelimiter );
	size_t EnsureSingleSpace( IN OUT std::tstring& rText );
}


namespace env
{
	// environment variables

	bool HasAnyVariable( const std::tstring& source );

	std::tstring GetVariableValue( const TCHAR varName[], const TCHAR* pDefaultValue = nullptr );
	bool SetVariableValue( const TCHAR varName[], const TCHAR* pValue );

	std::tstring ExpandStrings( const TCHAR* pSource );			// expand "%WIN_VAR%" Windows environment variables
	std::tstring ExpandPaths( const TCHAR* pSource );			// expand "%WIN_VAR%" and "$(VC_MACRO_VAR)" - Windows and Visual Studio style environment variables
	size_t AddExpandedPaths( IN OUT std::vector<fs::CPath>& rEvalPaths, const TCHAR* pSource, const TCHAR delim[] = _T(";") );		// add unique to rPaths

	std::tstring UnExpandPaths( const std::tstring& expanded, const std::tstring& text );
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
	bool ParseValue( OUT ValueT& rValue, const std::basic_string<CharT>& text )
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
	inline bool ParseValue( OUT std::basic_string<CharT>& rValue, const std::basic_string<CharT>& text )
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
	bool ParseNameValue( OUT std::basic_string<CharT>& rName, ValueT& rValue, const std::basic_string<CharT>& spec, CharT sep = '=' )
	{
		std::pair< CSequence<CharT>, CSequence<CharT> > seqPair;

		if ( !ParseNameValuePair<CharT>( seqPair, spec, sep ) )
			return false;

		rName = seqPair.first.ToString();
		return ParseValue<CharT>( rValue, seqPair.second.ToString() );
	}
}


namespace num
{
	const std::locale& GetEmptyLocale( void );		// empty locale (devoid of facets)


	template< typename ValueT >
	inline ValueT MinValue( void ) { return std::numeric_limits<ValueT>::min(); }

	// for double doesn't work (DBL_MIN is minimal positive value); in C++ 11 use lowest()
	template<>
	inline double MinValue<double>( void ) { return -std::numeric_limits<double>::max(); }	// min doesn't work (DBL_MIN is minimal positive value)

	template< typename ValueT >
	inline ValueT MaxValue( void ) { return std::numeric_limits<ValueT>::max(); }


	template< typename ValueT >		// [0, MAX]
	inline Range<ValueT> PositiveRange( void ) { return Range<ValueT>( 0, MaxValue<ValueT>() ); }

	template< typename ValueT >		// [MIN, 0]
	inline Range<ValueT> NegativeRange( void ) { return Range<ValueT>( MinValue<ValueT>(), 0 ); }

	template< typename ValueT >		// [MIN, MAX]
	inline Range<ValueT> FullRange( void ) { return Range<ValueT>( MinValue<ValueT>(), MaxValue<ValueT>() ); }


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
	bool ParseNumber( OUT ValueT& rNumber, const std::tstring& text, size_t* pSkipLength = nullptr, const std::locale& loc = GetEmptyLocale() )
	{
		std::tistringstream iss( text );
		iss.imbue( loc );
		iss >> rNumber;
		if ( iss.fail() )
			return false;

		if ( pSkipLength != nullptr )
			*pSkipLength = static_cast<size_t>( iss.tellg() );
		return true;
	}

	template<>
	bool ParseNumber<BYTE>( OUT BYTE& rNumber, const std::tstring& text, size_t* pSkipLength, const std::locale& loc );

	template<>
	bool ParseNumber<signed char>( OUT signed char& rNumber, const std::tstring& text, size_t* pSkipLength, const std::locale& loc );


	template< typename ValueT >
	std::tstring FormatHexNumber( ValueT value, const TCHAR* pFormat = _T("0x%X") )
	{
		return str::Format( pFormat, value );
	}

	template< typename ValueT >
	bool ParseHexNumber( OUT ValueT& rNumber, const std::tstring& text, size_t* pSkipLength = nullptr )
	{
		std::tistringstream iss( str::SkipHexPrefix( text.c_str(), str::IgnoreCase ) );
		size_t number;
		iss >> std::hex >> number;
		if ( iss.fail() )
			return false;

		rNumber = static_cast<ValueT>( number );
		if ( pSkipLength != nullptr )
			*pSkipLength = static_cast<size_t>( iss.tellg() );
		return true;
	}

	bool StripFractionalZeros( OUT std::tstring& rText, const std::locale& loc = str::GetUserLocale() );
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
	bool EnwrapNumericSequence( IN OUT Range<IntT>& rRange, const StringT& text )
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
	size_t EnsureUniformZeroPadding( IN OUT std::vector<std::tstring>& rItems );			// returns max count of numbers found for all items
}


namespace str
{
	// line utilities

	template< typename StringT >
	StringT& ToWindowsLineEnds( IN OUT StringT* pText )
	{
		ASSERT_PTR( pText );
		const StringT::value_type winLineEnd[] = { '\r', '\n', 0 }, unixLineEnd[] = { '\n', 0 };

		str::Replace( *pText, winLineEnd, unixLineEnd );
		str::Replace( *pText, unixLineEnd, winLineEnd );
		return *pText;
	}

	template< typename StringT >
	StringT& ToUnixLineEnds( IN OUT StringT* pText )
	{
		ASSERT_PTR( pText );
		const StringT::value_type winLineEnd[] = {'\r', '\n', 0 }, unixLineEnd[] = { '\n', 0 };

		str::Replace( *pText, winLineEnd, unixLineEnd );
		return *pText;
	}


	template< typename CharT, typename StringT >
	inline void SplitLines( OUT std::vector<StringT>& rItems, const CharT* pSource, const CharT* pLineEnd )
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
		AppendLine( IN OUT std::tstring* pOutput, const TCHAR* pLineSep = _T("\n") ) : m_pOutput( pOutput ), m_pLineSep( pLineSep ) { ASSERT_PTR( m_pOutput ); }

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
	std::tstring FormatEscapeSeq( const std::tstring& actualCode, bool uiSeq = false );
	std::tstring ParseEscapeSeqs( const std::tstring& literalText, bool uiSeq = false );
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


namespace app
{
	bool HasCommandLineOption( const TCHAR* pOption, std::tstring* pValue = nullptr );
}


namespace arg
{
	bool IsSwitch( const TCHAR* pArg );					// "-arg" or "/arg"
	const TCHAR* GetSwitch( const TCHAR* pArg );		// return "arg" for "-arg" or "/arg"

	bool Equals( const TCHAR* pArg, const TCHAR* pMatch );
	bool EqualsAnyOf( const TCHAR* pArg, const TCHAR* pMatchList, const TCHAR* pListDelims = _T("|") );

	bool StartsWith( const TCHAR* pArg, const TCHAR* pPrefix, size_t count = std::tstring::npos );
	bool StartsWithAnyOf( const TCHAR* pArg, const TCHAR* pPrefixList, const TCHAR* pListDelims = _T("|") );

	bool ParseValuePair( OUT std::tstring& rValue, const TCHAR* pArg, const TCHAR* pNameList, TCHAR valueSep = _T('='), const TCHAR* pListDelims = _T("|") );
	bool ParseOptionalValuePair( OUT std::tstring* pValue, const TCHAR* pArg, const TCHAR* pNameList, TCHAR valueSep = _T('='), const TCHAR* pListDelims = _T("|") );


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


namespace word
{
	enum WordStatus { WordStart, WordCore, WordEnd, Whitespace };

	WordStatus GetWordStatus( const std::tstring& text, size_t pos, const std::locale& loc = str::GetUserLocale() );
	size_t FindPrevWordBreak( const std::tstring& text, size_t pos, const std::locale& loc = str::GetUserLocale() );
	size_t FindNextWordBreak( const std::tstring& text, size_t pos, const std::locale& loc = str::GetUserLocale() );

	bool IsAlphaNumericWord( const std::tstring& text, const std::locale& loc = str::GetUserLocale() );


	bool IsWordBreak( func::CharKind::Kind prevKind, func::CharKind::Kind kind );


	template< typename CharT >
	std::basic_string<CharT> ToSpacedWordBreaks( const CharT* pLiteral, char delim = ' ' )
	{	// convert "DarkGray80" to "Dark Gray 80"
		ASSERT_PTR( pLiteral );
		std::basic_string<CharT> outText;
		outText.reserve( str::GetLength( pLiteral ) + 10 );

		const func::CharKind charKindFunc;

		for ( func::CharKind::Kind prevKind = func::CharKind::Delim; *pLiteral != '\0'; ++pLiteral )
		{
			func::CharKind::Kind kind = charKindFunc( *pLiteral );

			if ( !outText.empty() && IsWordBreak( prevKind, kind ) )
				outText += delim;

			outText += *pLiteral;
			prevKind = kind;
		}

		return outText;
	}

	template< typename CharT >
	std::basic_string<CharT> ToUpperLiteral( const CharT* pLiteral, char delim = '_' )
	{	// convert "DarkGray80" or "Dark Gray 80" to "DARK_GRAY_80"
		ASSERT_PTR( pLiteral );
		std::basic_string<CharT> outText;
		outText.reserve( str::GetLength( pLiteral ) + 10 );

		const func::CharKind charKindFunc;
		const func::ToUpper toUpperFunc;
		const pred::IsSpace isSpace;

		for ( func::CharKind::Kind prevKind = func::CharKind::Delim; *pLiteral != '\0'; ++pLiteral )
		{
			func::CharKind::Kind kind = charKindFunc( *pLiteral );

			if ( isSpace( *pLiteral ) )
			{
				if ( str::LastChar( outText ) != delim )			// avoid doubling the delimiter
					outText += delim;
			}
			else if ( delim == *pLiteral )							// allow doubling the delimiter if in source literal
				outText += *pLiteral;
			else
			{
				if ( !outText.empty() && IsWordBreak( prevKind, kind ) )
					if ( str::LastChar( outText ) != delim )		// avoid doubling the delimiter on word breaks
						outText += delim;

				outText += toUpperFunc( *pLiteral );
			}

			prevKind = kind;
		}

		return outText;
	}

	template< typename CharT >
	std::basic_string<CharT> ToCapitalizedLiteral( const CharT* pLiteral, char delim = '_', bool camelCase = false )
	{	// convert "DARK_GRAY_80" to "DarkGray80"
		ASSERT_PTR( pLiteral );
		std::basic_string<CharT> outText;
		outText.reserve( str::GetLength( pLiteral ) );

		const func::ToUpper toUpperFunc;
		const func::ToLower toLowerFunc;

		for ( bool nextUpper = !camelCase; *pLiteral != '\0'; ++pLiteral )
			if ( delim == *pLiteral )
				nextUpper = true;
			else if ( nextUpper )
			{
				outText += toUpperFunc( *pLiteral );
				nextUpper = false;
			}
			else
				outText += toLowerFunc( *pLiteral );

		return outText;
	}

	template< typename CharT >
	inline std::basic_string<CharT> ToCamelCaseLiteral( const CharT* pLiteral, char delim = '_' ) { return ToCapitalizedLiteral( pLiteral, delim, true ); }

	template< typename CharT >
	std::basic_string<CharT> ToggleUpperLiteral( const CharT* pLiteral, char delim = '_', bool camelCase = false )
	{	// cycle through "DARK_GRAY_80" -> "DarkGray80" -> "DARK_GRAY_80" -> ...
		if ( str::Contains( pLiteral, delim ) )		// is delimited?
			return ToCapitalizedLiteral( pLiteral, delim, camelCase );
		else
			return ToUpperLiteral( pLiteral, delim );
	}
}


#endif // StringUtilities_h
