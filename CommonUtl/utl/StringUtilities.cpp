
#include "stdafx.h"
#include "StringUtilities.h"
#include "TokenIterator.h"
#include "EnumTags.h"
#include <iomanip>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace str
{
	const TCHAR g_ellipsis[] = _T("...");
	const TCHAR g_paragraph[] = _T("\xB6");

	template<> const char* StdDelimiters< char >( void ) { return " \t"; }
	template<> const wchar_t* StdDelimiters< wchar_t >( void ) { return L" \t"; }


	std::tstring& Truncate( std::tstring& rText, size_t maxLen, const TCHAR suffix[] /*= g_ellipsis*/, bool atEnd /*= true*/ )
	{
		size_t suffixLen = str::GetLength( suffix );
		ASSERT( suffixLen <= maxLen );

		if ( rText.length() + suffixLen > maxLen )
			if ( atEnd )
			{
				rText.resize( maxLen - suffixLen );			// "something..."
				rText += suffix;
			}
			else
				rText = suffix + rText.substr( rText.length() - maxLen + suffixLen );	// "...sentence"

		return rText;
	}

	std::tstring& SingleLine( std::tstring& rText, size_t maxLen /*= utl::npos*/, const TCHAR sepLineEnd[] /*= g_paragraph*/ )
	{
		str::Replace( rText, _T("\r\n"), sepLineEnd );
		str::Replace( rText, _T("\n"), sepLineEnd );
		return Truncate( rText, maxLen );
	}


	size_t ReplaceDelimiters( std::tstring& rText, const TCHAR* pDelimiters, const TCHAR* pNewDelimiter )
	{
		std::vector< std::tstring > items;
		Tokenize( items, rText.c_str(), pDelimiters );
		rText = Join( items, pNewDelimiter );
		return items.size();
	}

	size_t EnsureSingleSpace( std::tstring& rText )
	{
		return ReplaceDelimiters( rText, _T(" \t"), _T(" ") );
	}


	std::tstring ExpandEnvironmentStrings( const TCHAR* pSource )
	{
		ASSERT_PTR( pSource );
		std::vector< TCHAR > expanded( ::ExpandEnvironmentStrings( pSource, NULL, 0 ) + 1 );						// allocate dest
		::ExpandEnvironmentStrings( pSource, &expanded.front(), static_cast< DWORD >( expanded.size() ) );			// expand vars
		return &expanded.front();
	}

	void QueryEnvironmentVariables( std::vector< std::tstring >& rVariables, const TCHAR* pSource )
	{
		static const TCHAR sep[] = _T("%");
		QueryEnclosedItems( rVariables, pSource, sep, sep );
	}

	void ExpandEnvironmentVariables( std::vector< std::tstring >& rValues, const std::vector< std::tstring >& rVariables )
	{
		rValues.clear();
		rValues.reserve( rVariables.size() );

		for ( std::vector< std::tstring >::const_iterator itVariable = rVariables.begin(); itVariable != rVariables.end(); ++itVariable )
			rValues.push_back( str::ExpandEnvironmentStrings( itVariable->c_str() ) );
	}

} //namespace str


namespace num
{
	const std::locale& GetEmptyLocale( void )
	{
		static const std::locale emptyLocale( std::locale::empty() );	// empty locale (devoid of facets)
		return emptyLocale;
	}

	bool StripFractionalZeros( std::tstring& rText, const std::locale& loc /*= str::GetUserLocale()*/ )
	{
		const TCHAR decimalPoint = std::use_facet< std::numpunct< TCHAR > >( loc ).decimal_point();
		std::tstring::reverse_iterator itStart = rText.rbegin();
		std::tstring::reverse_iterator itPoint = std::find( itStart, rText.rend(), decimalPoint );
		if ( itPoint != rText.rend() )
		{
			while ( itStart != itPoint && _T('0') == *itStart )
				++itStart;

			if ( itStart == itPoint )
				++itStart;
		}
		if ( itStart == rText.rbegin() )
			return false;

		rText.erase( std::distance( itStart, rText.rend() ) );
		return true;					// changed
	}

	std::tstring FormatNumber( double value, const std::locale& loc /*= GetEmptyLocale()*/ )
	{
		std::tostringstream oss;
		oss.imbue( loc );
		oss << std::setiosflags( std::ios::fixed ) << value;			// << std::setprecision( 7 )

		std::tstring text = oss.str();
		StripFractionalZeros( text, loc );
		return text;
	}

	template<>
	bool ParseNumber< BYTE >( BYTE& rNumber, const std::tstring& text, size_t* pSkipLength, const std::locale& loc )
	{
		unsigned int number;
		if ( !ParseNumber( number, text, pSkipLength, loc ) || number > 255 )
			return false;
		rNumber = static_cast< BYTE >( number );
		return true;
	}

	template<>
	bool ParseNumber< signed char >( signed char& rNumber, const std::tstring& text, size_t* pSkipLength, const std::locale& loc )
	{
		int number;
		if ( !ParseNumber( number, text, pSkipLength, loc ) || number > 255 )
			return false;
		rNumber = static_cast< signed char >( number );
		return true;
	}


	Range< size_t > FindNumericSequence( const std::tstring& text, size_t pos /*= 0*/ )
	{
		std::tstring::const_iterator itStart = text.begin() + pos;
		ASSERT( itStart < text.end() );

		while ( itStart != text.end() && !str::CharTraits::IsDigit( *itStart ) )
			++itStart;

		Range< size_t > range( std::distance( text.begin(), itStart ) );

		if ( EnwrapNumericSequence( range, text ) )
			return range;
		else
			return Range< size_t >( std::tstring::npos );
	}

	size_t EnsureUniformZeroPadding( std::vector< std::tstring >& rItems )
	{
		std::vector< size_t > digitWidths;

		// 1: detect maximum padding for all numeric sequences in each item
		for ( std::vector< std::tstring >::const_iterator itItem = rItems.begin(); itItem != rItems.end(); ++itItem )
		{
			const TCHAR* pItem = itItem->c_str();
			std::vector< size_t > widths;			// per item

			for ( size_t pos = 0; pos != itItem->length(); )
			{
				Range< size_t > numRange = FindNumericSequence( *itItem, pos );
				if ( std::tstring::npos == numRange.m_start )
					break;

				ASSERT( !numRange.IsEmpty() );

				// skip leading zeros (reduce numbers to shortest format)
				while ( numRange.m_start < numRange.m_end - 1 &&
						_T('0') == pItem[ numRange.m_start ] &&
						str::CharTraits::IsDigit( pItem[ numRange.m_start + 1 ] ) )
					++numRange.m_start;

				widths.push_back( numRange.GetSpan< size_t >() );
				pos = numRange.m_end;
			}

			for ( size_t i = 0; i != widths.size(); ++i )
				if ( i < digitWidths.size() )
					digitWidths[ i ] = std::max( digitWidths[ i ], widths[ i ] );
				else
					digitWidths.push_back( widths[ i ] );
		}

		// 2: reformat numbers in all items using uniform zero padding
		if ( !digitWidths.empty() )
			for ( std::vector< std::tstring >::iterator itItem = rItems.begin(); itItem != rItems.end(); ++itItem )
			{
				size_t pos = 0;
				for ( std::vector< size_t >::const_iterator itWidth = digitWidths.begin(); itWidth != digitWidths.end() && pos != itItem->length(); ++itWidth )
				{
					Range< size_t > numRange = FindNumericSequence( *itItem, pos );
					if ( std::tstring::npos == numRange.m_start )
						break;

					std::tstring numberText = itItem->substr( numRange.m_start, numRange.GetSpan< size_t >() );
					UINT number = 0;
					if ( num::ParseNumber( number, numberText ) )
					{
						std::tstring newNumberText = str::Format( _T("%0*u"), *itWidth, number );		// reformat with 0 padding
						if ( numberText != newNumberText )
							itItem->replace( numRange.m_start, numRange.GetSpan< size_t >(), newNumberText );
						pos = numRange.m_start + newNumberText.length();
					}
					else
						pos = numRange.m_start + numberText.length();
				}
			}

		return digitWidths.size();
	}
}


namespace num
{
	const CEnumTags& GetTags_BytesUnit( void )
	{
		static const CEnumTags tags( _T("bytes|kilo-bytes|mega-bytes|giga-bytes|tera-bytes|"), _T("B|KB|MB|GB|TB|") );
		return tags;
	}


	std::tstring FormatFileSize( ULONGLONG byteFileSize, BytesUnit unit /*= AutoBytes*/, bool longUnitTag /*= false*/, const std::locale& loc /*= str::GetUserLocale()*/ )
	{
		std::pair< double, BytesUnit > sizeUnit = ConvertFileSize( byteFileSize, unit );

		return num::FormatNumber( sizeUnit.first, loc ) + _T(" ") + GetTags_BytesUnit().Format( sizeUnit.second, longUnitTag ? CEnumTags::UiTag : CEnumTags::KeyTag );
	}

	double RoundOffFileSize( double number )
	{
		double factor = 1.0;			// round to 1 (no fractional digits)
		double roundUpBy = 0.5;			// fractional rounding up amount

		if ( number < 1.0 )
			factor = 1000.0;			// round to 3 fractional digits
		else if ( number < 10.0 )
			factor = 100.0;				// round to 1 fractional digit
		else if ( number < 100.0 )
			factor = 10.0;				// round to 2 fractional digit
		else if ( number < 1024.0 && number >= 1023.0 )			// a bit less than 1024
		{
			factor = 10.0;				// round to 1 fractional digit
			roundUpBy = 0;				// no rounding up
		}

		double roundedNumber = number * factor;
		ULONGLONG integer = static_cast< ULONGLONG >( roundedNumber + roundUpBy );

		roundedNumber = static_cast< float >( integer ) / factor;
		return roundedNumber;
	}

	std::pair< double, BytesUnit > ConvertFileSize( ULONGLONG fileSize, BytesUnit toUnit /*= AutoBytes*/ )
	{
		ULONGLONG newFileSize = fileSize, remainderSize = 0;
		static const size_t s_kiloSize = 1024;

		BytesUnit unit = Bytes;
		for ( ; unit != toUnit; ++(int&)unit )
		{
			if ( AutoBytes == toUnit && newFileSize < s_kiloSize )
				break;

			remainderSize = ( newFileSize % s_kiloSize );
			newFileSize /= s_kiloSize;
		}

		double newSize = (double)newFileSize + (double)remainderSize / s_kiloSize;

		if ( unit != Bytes )
			newSize = RoundOffFileSize( newSize );			// round off fractional part if not Bytes

		return std::pair< double, BytesUnit >( newSize, unit );
	}
}


namespace str
{
	std::string& ToWindowsLineEnds( std::string& rText )
	{
		str::Replace( rText, "\r\n", "\n" );
		str::Replace( rText, "\n", "\r\n" );
		return rText;
	}

	std::wstring& ToWindowsLineEnds( std::wstring& rText )
	{
		str::Replace( rText, L"\r\n", L"\n" );
		str::Replace( rText, L"\n", L"\r\n" );
		return rText;
	}

	std::string& ToUnixLineEnds( std::string& rText )
	{
		str::Replace( rText, "\r\n", "\n" );
		return rText;
	}

	std::wstring& ToUnixLineEnds( std::wstring& rText )
	{
		str::Replace( rText, L"\r\n", L"\n" );
		return rText;
	}
}


namespace code
{
	std::tstring FormatEscapeSeq( const std::tstring& text, bool uiSeq /*= false*/ )
	{
		std::tostringstream display;

		for ( const TCHAR* pSource = text.c_str(); *pSource != _T('\0'); ++pSource )
			switch ( *pSource )
			{
				case _T('\r'): display << _T('\\'); display << _T('r'); break;
				case _T('\n'): display << _T('\\'); display << _T('n'); break;
				case _T('\t'): display << _T('\\'); display << _T('t'); break;
				case _T('\v'): display << _T('\\'); display << _T('v'); break;
				case _T('\a'): display << _T('\\'); display << _T('a'); break;
				case _T('\b'): display << _T('\\'); display << _T('b'); break;
				case _T('\f'): display << _T('\\'); display << _T('f'); break;
				case _T('\\'): display << _T('\\'); display << _T('\\'); break;
				default:
					if ( !uiSeq )
						switch ( *pSource )
						{
							case _T('\''): display << _T('\\'); display << _T('\''); continue;
							case _T('\"'): display << _T('\\'); display << _T('\"'); continue;
						}

					display << *pSource;
					break;
			}

		return display.str();
	}

	std::tstring ParseEscapeSeqs( const std::tstring& displayText, bool uiSeq /*= false*/ )
	{
		std::tostringstream text;

		for ( const TCHAR* pSource = displayText.c_str(); *pSource != _T('\0'); ++pSource )
			if ( *pSource == _T('\\') )
				switch ( *++pSource )
				{
					case _T('r'):  text << _T('\r'); break;
					case _T('n'):  text << _T('\n'); break;
					case _T('t'):  text << _T('\t'); break;
					case _T('v'):  text << _T('\v'); break;
					case _T('a'):  text << _T('\a'); break;
					case _T('b'):  text << _T('\b'); break;
					case _T('f'):  text << _T('\f'); break;
					case _T('\\'): text << _T('\\'); break;
					default:
						if ( !uiSeq )
							switch ( *pSource )
							{
								case _T('\"'): text << _T('\"'); continue;
								case _T('\''): text << _T('\''); continue;
							}

						// bad escape -> consider it an '\'
						text << _T('\\') << *pSource;
				}
			else
				text << *pSource;

		return text.str();
	}

} //namespace code


namespace word
{
	WordStatus GetWordStatus( const std::tstring& text, size_t pos, const std::locale& loc /*= str::GetUserLocale()*/ )
	{
		const TCHAR* pText = text.c_str();
		ASSERT( pos <= text.length() );

		bool curAlphaNum = std::isalnum( pText[ pos ], loc );
		if ( 0 == pos )
			return curAlphaNum ? WordStart : Whitespace;

		bool prevAlphaNum = std::isalnum( pText[ pos - 1 ], loc );
		if ( curAlphaNum )
			return prevAlphaNum ? WordCore : WordStart;
		else
			return prevAlphaNum ? WordEnd : Whitespace;
	}

	size_t FindPrevWordBreak( const std::tstring& text, size_t pos, const std::locale& loc /*= str::GetUserLocale()*/ )
	{
		ASSERT( pos <= text.length() );

		if ( pos != 0 )
			switch ( GetWordStatus( text, pos, loc ) )
			{
				case WordStart:
				case Whitespace:
					while ( pos != 0 && !std::isalnum( text[ pos - 1 ], loc ) )
						--pos;
					break;
				case WordCore:
				case WordEnd:
					while ( pos != 0 && std::isalnum( text[ pos - 1 ], loc ) )
						--pos;
					break;
			}

		return pos;
	}

	size_t FindNextWordBreak( const std::tstring& text, size_t pos, const std::locale& loc /*= str::GetUserLocale()*/ )
	{
		ASSERT( pos <= text.length() );
		size_t len = text.length();

		if ( pos != len )
			switch ( GetWordStatus( text, pos, loc ) )
			{
				case WordStart:
				case WordCore:
					while ( pos != len && std::isalnum( text[ pos ], loc ) )
						++pos;
					break;
				case WordEnd:
				case Whitespace:
  					while ( pos != len && !std::isalnum( text[ pos ], loc ) )
						++pos;
					break;
			}

		return pos;
	}

} //namespace word


namespace app
{
	bool HasCommandLineOption( const TCHAR* pOption, std::tstring* pValue /*= NULL*/ )
	{
		if ( pValue != NULL )
			pValue->clear();

		for ( int i = 1; i < __argc; ++i )
		{
			std::tstring argument = __targv[ i ];
			str::CTokenIterator< pred::CompareNoCase > itToken( argument );
			if ( itToken.Matches( _T('/') ) || itToken.Matches( _T('-') ) )
				if ( itToken.Matches( pOption ) )
				{
					if ( pValue != NULL && itToken.Matches( '=' ) )
						*pValue = itToken.GetCurrentSubstr();
					return true;
				}
		}

		return false;
	}

} //namespace app


namespace arg
{
	bool IsSwitch( const TCHAR* pArg )
	{
		ASSERT_PTR( pArg );
		return _T('/') == *pArg || _T('-') == *pArg;
	}

	const TCHAR* GetSwitch( const TCHAR* pArg )
	{
		return IsSwitch( pArg ) ? ( pArg + 1 ) : NULL;
	}

	bool Equals( const TCHAR* pArg, const TCHAR* pMatch )
	{
		return str::Equals< str::IgnoreCase >( pArg, pMatch );
	}

	bool EqualsAnyOf( const TCHAR* pArg, const TCHAR* pMatchList, const TCHAR* pListDelims /*= _T("|")*/ )
	{
		for ( str::const_iterator pItem = pMatchList, pMatchListEnd = str::end( pMatchList ); pItem != pMatchListEnd; )
		{
			const TCHAR* pItemEnd = str::FindTokenEnd( pItem, pListDelims );
			if ( str::EqualsN( pArg, pItem, std::distance( pItem, pItemEnd ), false ) )
				return true;
			if ( pItemEnd == pMatchListEnd )
				break;							// reached last item
			pItem = pItemEnd + 1;
		}

		return false;
	}

	bool StartsWith( const TCHAR* pArg, const TCHAR* pPrefix, size_t count /*= std::tstring::npos*/ )
	{
		return pred::Equal == str::CompareN( pArg, pPrefix, func::ToUpper(), count != std::tstring::npos ? count : str::GetLength( pPrefix ) );
	}

	bool StartsWithAnyOf( const TCHAR* pArg, const TCHAR* pPrefixList, const TCHAR* pListDelims )
	{
		for ( str::const_iterator pPrefix = pPrefixList, pPrefixListEnd = str::end( pPrefixList ); pPrefix != pPrefixListEnd; )
		{
			const TCHAR* pPrefixEnd = str::FindTokenEnd( pPrefix, pListDelims );
			if ( StartsWith( pArg, pPrefix, std::distance( pPrefix, pPrefixEnd ) ) )
				return true;
			if ( pPrefixEnd == pPrefixListEnd )
				break;							// reached last item
			pPrefix = pPrefixEnd + 1;
		}

		return false;
	}

	bool ParseValuePair( std::tstring& rValue, const TCHAR* pPairArg, const TCHAR* pNameList, TCHAR valueSep /*= _T('=')*/, const TCHAR* pListDelims /*= _T("|")*/ )
	{	// argument syntax: name=value
		const TCHAR* pValueSep = std::find( pPairArg, str::end( pPairArg ), valueSep );
		if ( !str::IsEmpty( pValueSep ) )		// a "name=value" pair
			if ( StartsWithAnyOf( pPairArg, pNameList, pListDelims ) )
			{
				rValue = pValueSep + 1;
				return true;
			}

		rValue.clear();
		return false;
	}
}
