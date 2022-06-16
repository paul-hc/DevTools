#ifndef StringBase_h
#define StringBase_h
#pragma once

#include <locale>
#include <sstream>


namespace str
{
	const std::locale& GetUserLocale( void );
	const std::locale& GetUtf8CvtLocale( void );


	std::string ToAnsi( const wchar_t* pWide, size_t charCount = std::wstring::npos );
	std::wstring FromAnsi( const char* pAnsi, size_t charCount = std::string::npos );

	std::string ToUtf8( const wchar_t* pWide, size_t charCount = std::wstring::npos );
	std::wstring FromUtf8( const char* pUtf8, size_t charCount = std::string::npos );


	std::string AsNarrow( const std::tstring& text );
	std::wstring AsWide( const std::tstring& text );


	const std::tstring& GetEmpty( void );

	std::string Format( const char* pFormat, ... );
	std::tstring Format( const TCHAR* pFormat, ... );
	std::tstring Format( UINT formatId, ... );

	std::tstring Load( UINT strId, bool* pLoaded = NULL );
	std::vector< std::tstring > LoadStrings( UINT strId, const TCHAR* pSep = _T("|"), bool* pLoaded = NULL );
	std::pair<std::tstring, std::tstring> LoadPair( UINT strId, const TCHAR* pSep = _T("|"), bool* pLoaded = NULL );


	namespace mfc
	{
	#ifdef _MFC_VER
		CString Load( UINT strId );
		CString GetTypeName( const CObject* pObject );
	#endif //_MFC_VER
	}


	std::tstring GetTypeName( const type_info& info );
}


namespace fs { class CPath; }

namespace str
{
	struct CharTraits		// CRT abstraction for strings API: character and character-ptr traits and predicates
	{
		static size_t GetLength( const char* pText ) { return pText != NULL ? ::strlen( pText ) : 0; }
		static size_t GetLength( const wchar_t* pText ) { return pText != NULL ? ::wcslen( pText ) : 0; }

		static char ToUpper( char ch ) { return (char)(unsigned char)::toupper( (unsigned char)ch ); }
		static wchar_t ToUpper( wchar_t ch ) { return static_cast<wchar_t>( ::towupper( ch ) ); }

		static bool IsDigit( char ch ) { return ::isdigit( (unsigned char)ch ) != 0; }
		static bool IsDigit( wchar_t ch ) { return ::iswdigit( ch ) != 0; }

		// case-sensitive
		static inline pred::CompareResult Compare( const char leftCh, const char rightCh ) { return pred::ToCompareResult( leftCh - rightCh ); }
		static inline pred::CompareResult Compare( const wchar_t leftCh, const wchar_t rightCh ) { return pred::ToCompareResult( leftCh - rightCh ); }

		static inline pred::CompareResult Compare( const char* pLeft, const char* pRight ) { return pred::ToCompareResult( ::strcmp( pLeft, pRight ) ); }
		static inline pred::CompareResult Compare( const wchar_t* pLeft, const wchar_t* pRight ) { return pred::ToCompareResult( ::wcscmp( pLeft, pRight ) ); }

		static inline pred::CompareResult CompareN( const char* pLeft, const char* pRight, size_t count ) { return pred::ToCompareResult( ::strncmp( pLeft, pRight, count ) ); }
		static inline pred::CompareResult CompareN( const wchar_t* pLeft, const wchar_t* pRight, size_t count ) { return pred::ToCompareResult( ::wcsncmp( pLeft, pRight, count ) ); }

		// case-insensitive
		static inline pred::CompareResult CompareI( const char leftCh, const char rightCh ) { return Compare( ToUpper( leftCh ), ToUpper( rightCh ) ); }
		static inline pred::CompareResult CompareI( const wchar_t leftCh, const wchar_t rightCh ) { return Compare( ToUpper( leftCh ), ToUpper( rightCh ) ); }

		static inline pred::CompareResult CompareI( const char* pLeft, const char* pRight ) { return pred::ToCompareResult( ::_stricmp( pLeft, pRight ) ); }
		static inline pred::CompareResult CompareI( const wchar_t* pLeft, const wchar_t* pRight ) { return pred::ToCompareResult( ::_wcsicmp( pLeft, pRight ) ); }

		static inline pred::CompareResult CompareIN( const char* pLeft, const char* pRight, size_t count ) { return pred::ToCompareResult( ::_strnicmp( pLeft, pRight, count ) ); }
		static inline pred::CompareResult CompareIN( const wchar_t* pLeft, const wchar_t* pRight, size_t count ) { return pred::ToCompareResult( ::_wcsnicmp( pLeft, pRight, count ) ); }
	};

	namespace traits		// utils for converting stringy (string-like) types to character-ptr
	{
		inline const char* GetCharPtr( const char* pText ) { return pText; }
		inline const wchar_t* GetCharPtr( const wchar_t* pText ) { return pText; }
		inline const char* GetCharPtr( const std::string& text ) { return text.c_str(); }
		inline const wchar_t* GetCharPtr( const std::wstring& text ) { return text.c_str(); }
		const TCHAR* GetCharPtr( const fs::CPath& filePath );

		inline size_t GetLength( const char* pText ) { return CharTraits::GetLength( pText ); }
		inline size_t GetLength( const wchar_t* pText ) { return CharTraits::GetLength( pText ); }
		inline size_t GetLength( const std::string& text ) { return text.length(); }
		inline size_t GetLength( const std::wstring& text ) { return text.length(); }
		size_t GetLength( const fs::CPath& filePath );
	}
}


namespace str
{
	template< typename CharT >
	inline bool IsEmpty( const CharT* pText ) { return NULL == pText || 0 == *pText; }

	template< typename CharT >
	inline size_t GetLength( const CharT* pText ) { return CharTraits::GetLength( pText ); }


	template< typename CharT >
	size_t& SettleLength( size_t& rCount, const CharT* pText )
	{
		if ( std::basic_string<CharT>::npos == rCount )
			rCount = str::GetLength( pText );
		else
			REQUIRE( rCount <= str::GetLength( pText ) );

		return rCount;
	}


	enum CaseType { Case, IgnoreCase };


	template< str::CaseType caseType, typename CharT > bool Equals( const CharT* pLeft, const CharT* pRight );

	template<> inline bool Equals< Case, char >( const char* pLeft, const char* pRight ) { return pred::Equal == CharTraits::Compare( pLeft, pRight ); }
	template<> inline bool Equals< Case, wchar_t >( const wchar_t* pLeft, const wchar_t* pRight ) { return pred::Equal == CharTraits::Compare( pLeft, pRight ); }

	template<> inline bool Equals< IgnoreCase, char >( const char* pLeft, const char* pRight ) { return pred::Equal == CharTraits::CompareI( pLeft, pRight ); }
	template<> inline bool Equals< IgnoreCase, wchar_t >( const wchar_t* pLeft, const wchar_t* pRight ) { return pred::Equal == CharTraits::CompareI( pLeft, pRight ); }

	template< str::CaseType caseType, typename StringT >
	bool EqualString( const StringT& left, const StringT& right ) { return Equals< caseType >( left.c_str(), right.c_str() ); }

	template<typename StringT>
	bool EqualString( str::CaseType caseType, const StringT& left, const StringT& right ) { return str::Case == caseType ? Equals<str::Case>( left.c_str(), right.c_str() ) : Equals<str::IgnoreCase>( left.c_str(), right.c_str() ); }


	// string to STL iterators conversions
	typedef const TCHAR* const_iterator;

	inline const char* begin( const char* pText ) { return pText; }
	inline const wchar_t* begin( const wchar_t* pText ) { return pText; }

	inline const char* end( const char* pText ) { return pText + GetLength( pText ); }
	inline const wchar_t* end( const wchar_t* pText ) { return pText + GetLength( pText ); }

	inline char* Copy( char* pBuffer, const std::string& text ) { return strcpy( pBuffer, text.c_str() ); }
	inline wchar_t* Copy( wchar_t* pBuffer, const std::wstring& text ) { return wcscpy( pBuffer, text.c_str() ); }


	template< typename CharT >
	bool ContainsAnyOf( const CharT* pCharSet, CharT ch )
	{
		ASSERT_PTR( pCharSet );
		for ( ; *pCharSet != 0; ++pCharSet )
			if ( ch == *pCharSet )
				return true;

		return false;
	}

	template< typename CharT >
	inline const CharT* FindTokenEnd( const CharT* pText, const CharT delims[] )
	{
		const CharT* pTextEnd = str::end( pText );
		return std::find_first_of( pText, pTextEnd, delims, str::end( delims ) );
	}

	template< typename CharT >
	const CharT* SkipLineEnd( const CharT* pText )		// work for both text or binary mode: skips "\n" or "\r\n"
	{
		ASSERT_PTR( pText );

		if ( '\r' == *pText )
			++pText;

		if ( '\n' == *pText )
			++pText;

		return pText;
	}
}


namespace str
{
	template< typename CharT >
	std::basic_string<CharT> Clamp( const std::basic_string<CharT>& text, size_t maxLength, const TCHAR* pMoreSuffix = NULL )
	{	// clamps string to a maxLength, eventually adding a suffix
		std::basic_string<CharT> outText( text, 0, std::min( maxLength, text.length() ) );
		if ( text.length() > maxLength && pMoreSuffix != NULL )
			outText += pMoreSuffix;
		return outText;
	}


	template< typename CharT >
	inline std::basic_string<CharT>& PopBack( std::basic_string<CharT>& rText )
	{	// placeholder for basic_string::pop_back() that's missing in earlier versions of STD C++
		ASSERT( !rText.empty() );
		rText.erase( --rText.end() );
		return rText;
	}

	template< typename CharT >
	inline bool PopBackDelim( std::basic_string<CharT>& rText, CharT delim )
	{
		std::basic_string<CharT>::iterator itLast = rText.end();

		if ( rText.empty() || *--itLast != delim )
			return false;

		rText.erase( itLast );
		return true;
	}


	template< typename CharT >
	void EnquoteImpl( std::basic_string<CharT>& rOutText, const CharT* pText, const CharT leading[], const CharT trailing[], bool skipIfEmpty )
	{
		ASSERT_PTR( pText );
		rOutText = pText;
		if ( !skipIfEmpty || !rOutText.empty() )
		{
			rOutText.reserve( rOutText.length() + GetLength( leading ) + GetLength( trailing ) );
			rOutText.insert( 0, leading );
			rOutText.append( trailing );
		}
	}

	template< typename CharT >
	std::basic_string<CharT> Enquote( const CharT* pText, const CharT leading[], const CharT trailing[], bool skipIfEmpty = false )
	{
		std::basic_string<CharT> outText;
		EnquoteImpl( outText, pText, leading, trailing, skipIfEmpty );
		return outText;
	}


	// enquoting (by default with double-quotes)

	template< typename CharT >
	std::basic_string<CharT> Enquote( const CharT* pText, CharT quote = '"', bool skipIfEmpty = false )
	{
		const CharT quoteArray[] = { quote, '\0' };
		std::basic_string<CharT> outText;

		EnquoteImpl( outText, pText, quoteArray, quoteArray, skipIfEmpty );
		return outText;
	}

	template< typename StringT >
	inline StringT EnquoteStr( const StringT& value, typename StringT::value_type quote = '"', bool skipIfEmpty = false )
	{
		return Enquote( str::traits::GetCharPtr( value ), quote, skipIfEmpty );
	}

	inline std::tstring EnquoteStr( const fs::CPath& value, wchar_t quote = L'"', bool skipIfEmpty = false )
	{
		return Enquote( str::traits::GetCharPtr( value ), quote, skipIfEmpty );
	}


	namespace sq
	{
		// single-quote enquoting

		template< typename CharT >
		inline std::basic_string<CharT> Enquote( const CharT* pText, bool skipIfEmpty = false )
		{
			return ::str::Enquote<CharT>( pText, '\'', skipIfEmpty );
		}

		template< typename StringT >
		inline StringT EnquoteStr( const StringT& value, bool skipIfEmpty = false )
		{
			return ::str::EnquoteStr( value, '\'', skipIfEmpty );
		}

		inline std::tstring EnquoteStr( const fs::CPath& value, bool skipIfEmpty = false )
		{
			return ::str::EnquoteStr( value, '\'', skipIfEmpty );
		}
	}
}


namespace str
{
	template< typename StringT, typename ValueT >
	StringT ValueToString( const ValueT& value )
	{
		std::basic_ostringstream< typename StringT::value_type > oss;
		oss << value;
		return oss.str();
	}


	template< typename StringT >
	inline StringT& ToUpper( StringT& rText, const std::locale& loc = str::GetUserLocale() )
	{
		if ( !rText.empty() )
			std::transform( rText.begin(), rText.end(), rText.begin(), func::ToUpper( loc ) );
		return rText;
	}

	template< typename StringT >
	inline StringT& ToLower( StringT& rText, const std::locale& loc = str::GetUserLocale() )
	{
		if ( !rText.empty() )
			std::transform( rText.begin(), rText.end(), rText.begin(), func::ToLower( loc ) );
		return rText;
	}

	template< typename StringT >
	inline StringT MakeUpper( const StringT& rText, const std::locale& loc = str::GetUserLocale() )
	{
		StringT upperText = rText;
		return ToUpper( upperText, loc );
	}

	template< typename StringT >
	inline StringT MakeLower( const StringT& rText, const std::locale& loc = str::GetUserLocale() )
	{
		StringT lowerText = rText;
		return ToLower( lowerText, loc );
	}


}


namespace str
{
	template< typename CharT >
	struct CPart
	{
		CPart( const CharT* pString = NULL, size_t count = 0 ) : m_pString( pString ), m_count( count ) {}
		explicit CPart( const std::basic_string<CharT>& text ) : m_pString( text.c_str() ), m_count( text.length() ) {}		// the whole string

		bool IsEmpty( void ) const { return NULL == m_pString || 0 == m_count; }
		std::basic_string<CharT> ToString( void ) const { return !IsEmpty() ? std::basic_string<CharT>( m_pString, m_count ) : std::basic_string<CharT>(); }
	public:
		const CharT* m_pString;
		size_t m_count;
	};

	template< typename CharT >
	inline CPart<CharT> MakePart( const CharT* pString, size_t count = std::basic_string<CharT>::npos )
	{
		return CPart<CharT>( pString, SettleLength( count, pString ) );
	}

	template< typename CharT >
	std::basic_string<CharT> FormatNameValueSpec( const std::basic_string<CharT>& tag, const std::basic_string<CharT>& value, CharT sep = '=' )
	{
		return tag + sep + value;
	}

	template< typename CharT >
	bool ParseNameValuePair( std::pair< CPart<CharT>, CPart<CharT> >& rPartsPair, const std::basic_string<CharT>& spec, CharT sep = '=' )
	{
		size_t sepPos = spec.find( sep );
		if ( std::basic_string<CharT>::npos == sepPos )
			return false;

		rPartsPair.first = MakePart( spec.c_str(), sepPos );
		rPartsPair.second = MakePart( spec.c_str() + sepPos + 1 );
		return true;
	}
}


namespace func
{
	inline const std::tstring& StringOf( const std::tstring& filePath ) { return filePath; }		// for uniform string algorithms


	namespace tor
	{
		struct StringOf
		{
			template< typename StringyT >
			const std::tstring& operator()( const StringyT& value ) const { return func::StringOf( value ); }
		};
	}


	template< typename CharT >
	inline CharT toupper( CharT ch, const std::locale& loc = str::GetUserLocale() )
	{
		return std::toupper( ch, loc );
	}

	template< typename CharT >
	inline CharT tolower( CharT ch, const std::locale& loc = str::GetUserLocale() )
	{
		return std::tolower( ch, loc );
	}


	struct ToUpper
	{
		ToUpper( const std::locale& loc = str::GetUserLocale() ) : m_locale( loc ) {}

		template< typename CharT >
		CharT operator()( CharT ch ) const
		{
			return std::toupper( ch, m_locale );
		}
	private:
		const std::locale& m_locale;
	};


	struct ToLower
	{
		ToLower( const std::locale& loc = str::GetUserLocale() ) : m_locale( loc ) {}

		template< typename CharT >
		CharT operator()( CharT ch ) const
		{
			return std::tolower( ch, m_locale );
		}
	private:
		const std::locale& m_locale;
	};
}


namespace str
{
	template< typename CharT > const CharT* StdWhitespace( void );	// " \t\r\n"


	template< typename CharT >
	std::basic_string<CharT>& Trim( std::basic_string<CharT>& rText, const CharT* pWhiteSpace = StdWhitespace<CharT>() )
	{
		size_t startPos = rText.find_first_not_of( pWhiteSpace ), endPos = rText.find_last_not_of( pWhiteSpace );

		if ( std::basic_string<CharT>::npos == startPos || std::basic_string<CharT>::npos == endPos || startPos > endPos )
			rText.clear();
		else
			rText = rText.substr( startPos, endPos - startPos + 1 );
		return rText;
	}

	template< typename CharT >
	std::basic_string<CharT>& TrimRight( std::basic_string<CharT>& rText, const CharT* pWhiteSpace = StdWhitespace<CharT>() )
	{
		size_t endPos = rText.find_last_not_of( pWhiteSpace );
		if ( std::basic_string<CharT>::npos == endPos )
			rText.clear();
		else
			rText = rText.substr( 0, endPos + 1 );
		return rText;
	}

	template< typename CharT >
	std::basic_string<CharT>& TrimLeft( std::basic_string<CharT>& rText, const CharT* pWhiteSpace = StdWhitespace<CharT>() )
	{
		size_t startPos = rText.find_first_not_of( pWhiteSpace );
		if ( std::basic_string<CharT>::npos == startPos )
			rText.clear();
		else
			rText = rText.substr( startPos );
		return rText;
	}


	// trim copy

	template< typename CharT >
	std::basic_string<CharT> GetTrim( const std::basic_string<CharT>& rText, const CharT* pWhiteSpace = StdWhitespace<CharT>() )
	{
		std::basic_string<CharT> trimmed = rText;
		return Trim( trimmed, pWhiteSpace );
	}

	template< typename CharT >
	std::basic_string<CharT> GetTrimRight( const std::basic_string<CharT>& rText, const CharT* pWhiteSpace = StdWhitespace<CharT>() )
	{
		std::basic_string<CharT> trimmed = rText;
		return TrimRight( trimmed, pWhiteSpace );
	}

	template< typename CharT >
	std::basic_string<CharT> GetTrimLeft( const std::basic_string<CharT>& rText, const CharT* pWhiteSpace = StdWhitespace<CharT>() )
	{
		std::basic_string<CharT> trimmed = rText;
		return TrimLeft( trimmed, pWhiteSpace );
	}


#ifdef _MFC_VER
	inline CStringA& Trim( CStringA& rText ) { rText.TrimLeft(); rText.TrimRight(); return rText; }
	inline CStringW& Trim( CStringW& rText ) { rText.TrimLeft(); rText.TrimRight(); return rText; }

	inline BSTR AllocSysString( const std::tstring& text ) { return CString( text.c_str() ).AllocSysString(); }
#endif //_MFC_VER


	template< typename CharT >
	size_t Replace( std::basic_string<CharT>& rText, const CharT* pSearch, const CharT* pReplace, size_t maxCount = utl::npos )
	{
		ASSERT_PTR( pSearch );
		ASSERT_PTR( pReplace );
		size_t count = 0;

		if ( !str::IsEmpty( pSearch ) )
		{
			const size_t searchLen = str::GetLength( pSearch ), replaceLen = str::GetLength( pReplace );

			for ( size_t pos = 0;
				  count != maxCount && ( pos = rText.find( pSearch, pos ) ) != std::basic_string<CharT>::npos;
				  ++count, pos += replaceLen )
				rText.replace( pos, searchLen, pReplace );
		}

		return count;
	}


	template< typename CharT, typename StringT >
	void SplitAdd( std::vector< StringT >& rItems, const CharT* pSource, const CharT* pSep )
	{
		ASSERT( !str::IsEmpty( pSep ) );

		if ( !str::IsEmpty( pSource ) )
		{
			const size_t sepLen = str::GetLength( pSep );
			typedef const CharT* const_iterator;

			for ( const_iterator itItemStart = str::begin( pSource ), itEnd = str::end( pSource ); ; )
			{
				const_iterator itItemEnd = std::search( itItemStart, itEnd, pSep, pSep + sepLen );
				if ( itItemEnd != itEnd )
				{
					rItems.push_back( std::basic_string<CharT>( itItemStart, std::distance( itItemStart, itItemEnd ) ) );
					itItemStart = itItemEnd + sepLen;
				}
				else
				{
					rItems.push_back( std::basic_string<CharT>( itItemStart ) );			// last item
					break;
				}
			}
		}
	}

	template< typename CharT, typename StringT >
	inline void Split( std::vector< StringT >& rItems, const CharT* pSource, const CharT* pSep )
	{
		rItems.clear();
		SplitAdd( rItems, pSource, pSep );
	}

	template< typename CharT >
	void QuickSplit( std::vector<CharT>& rItems, const CharT* pSource, CharT sepCh )
	{
		// build a vector copy of source characters replacing all sepCh with '\0'
		rItems.assign( str::begin( pSource ), str::end( pSource ) + 1 );			// copy the EOS char
		for ( std::vector<CharT>::iterator itItem = rItems.begin(); itItem != rItems.end(); ++itItem )
			if ( sepCh == *itItem )
				*itItem = 0;
	}

	template< typename CharT >
	void QuickTokenize( std::vector<CharT>& rItems, const CharT* pSource, const CharT* pSepTokens )
	{
		const CharT* pSepEnd = str::end( pSepTokens );
		// build a vector copy of source characters replacing any chars in pSepTokens with '\0'
		rItems.assign( str::begin( pSource ), str::end( pSource ) + 1 );			// copy the EOS char
		for ( std::vector<CharT>::iterator itItem = rItems.begin(); itItem != rItems.end(); ++itItem )
			if ( std::find( pSepTokens, pSepEnd, *itItem ) != pSepEnd )				// any sep token
				*itItem = 0;
	}


	template< typename CharT, typename Iterator >
	std::basic_string<CharT> Join( Iterator itFirstToken, Iterator itLastToken, const CharT* pSep )
	{	// works with any forward/reverse iterator
		std::basic_ostringstream<CharT> oss;
		for ( Iterator itItem = itFirstToken; itItem != itLastToken; ++itItem )
		{
			if ( itItem != itFirstToken )
				oss << pSep;
			oss << *itItem;
		}
		return oss.str();
	}

	// works with container of any value type that has stream insertor defined
	//
	template< typename CharT, typename ContainerT >
	inline std::basic_string<CharT> Join( const ContainerT& items, const CharT* pSep )
	{
		return Join( items.begin(), items.end(), pSep );
	}


	// works with container of any value type that has stream insertor defined
	//
	template< typename ContainerT >
	inline std::tstring FormatSet( const ContainerT& items, const TCHAR* pSep = _T(",") )
	{
		return str::Format( _T("{%s}:count=%d"), Join( items.begin(), items.end(), pSep ).c_str(), items.size() );
	}
}


namespace pred
{
	struct IsEmpty
	{
		template< typename Type >
		bool operator()( const Type& object ) const
		{
			return object.empty();
		}
	};

	template< typename StringT >
	struct EqualString
	{
		EqualString( const StringT& text, str::CaseType caseType = str::IgnoreCase ) : m_text( text ), m_caseType( caseType ) {}

		bool operator()( const StringT& itemText ) const
		{
			return str::IgnoreCase == m_caseType
				? str::Equals< str::IgnoreCase >( m_text.c_str(), itemText.c_str() )
				: str::Equals< str::Case >( m_text.c_str(), itemText.c_str() );
		}
	private:
		const StringT& m_text;
		str::CaseType m_caseType;
	};
}


namespace str
{
	template< typename ContainerT >
	void RemoveEmptyItems( ContainerT& rItems )
	{
		rItems.erase( std::remove_if( rItems.begin(), rItems.end(), pred::IsEmpty() ), rItems.end() );
	}


	template< typename StrContainerT >
	void TrimItems( StrContainerT& rItems )
	{
		for ( typename StrContainerT::iterator itLine = rItems.begin(); itLine != rItems.end(); ++itLine )
			str::Trim( *itLine );
	}
}


namespace str
{
	// pass character and string constants for conditional compilation under Unicode or ANSI

	inline TCHAR Conditional( wchar_t wideCh, char ansiCh )
	{
	#ifdef  _UNICODE
		ansiCh;
		return wideCh;
	#else
		wideCh;
		return ansiCh;
	#endif
	}

	inline const TCHAR* Conditional( const wchar_t* pWide, const char* pAnsi )
	{
	#ifdef  _UNICODE
		pAnsi;
		return pWide;
	#else
		pWide;
		return pAnsi;
	#endif
	}
}


namespace stream
{
	bool Tag( std::tstring& rOutput, const std::tstring& tag, const TCHAR* pPrefixSep );

	bool InputLine( std::istream& is, std::tstring& rLine );
}


#include <iosfwd>


std::ostream& operator<<( std::ostream& os, const wchar_t* pWide );
std::wostream& operator<<( std::wostream& os, const char* pUtf8 );

std::ostream& operator<<( std::ostream& os, wchar_t chWide );
std::wostream& operator<<( std::wostream& os, char chUtf8 );

inline std::ostream& operator<<( std::ostream& os, const std::wstring& wide ) { return os << wide.c_str(); }
inline std::wostream& operator<<( std::wostream& os, const std::string& utf8 ) { return os << utf8.c_str(); }

#ifdef _MFC_VER
	inline std::ostream& operator<<( std::ostream& os, const CString& mfcString ) { return os << mfcString.GetString(); }
	inline std::wostream& operator<<( std::wostream& os, const CString& mfcString ) { return os << mfcString.GetString(); }
#endif //_MFC_VER


#endif // StringBase_h
