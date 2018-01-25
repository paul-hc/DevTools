#ifndef StringBase_h
#define StringBase_h
#pragma once

#include <locale>
#include <sstream>


namespace str
{
	const std::locale& GetUserLocale( void );
	const std::locale& GetUtf8CvtLocale( void );


	std::string ToAnsi( const wchar_t* pWide );
	std::wstring FromAnsi( const char* pAnsi );

	std::string ToUtf8( const wchar_t* pWide );
	std::wstring FromUtf8( const char* pUtf8 );


	std::string AsNarrow( const std::tstring& text );
	std::wstring AsWide( const std::tstring& text );


	std::tstring Format( const TCHAR* pFormat, ... );
	std::tstring Format( UINT formatId, ... );

	std::tstring Load( UINT strId );
	std::vector< std::tstring > LoadStrings( UINT strId, const TCHAR* pSep = _T("|") );
	std::pair< std::tstring, std::tstring > LoadPair( UINT strId, const TCHAR* pSep = _T("|") );


	namespace mfc
	{
		CString Load( UINT strId );
		CString GetTypeName( const CObject* pObject );
	}


	std::tstring GetTypeName( const type_info& info );


	template< typename StringType >
	inline StringType& ToUpper( StringType& rText, const std::locale& loc = str::GetUserLocale() )
	{
		if ( !rText.empty() )
			std::transform( rText.begin(), rText.end(), rText.begin(), func::ToUpper( loc ) );
		return rText;
	}

	template< typename StringType >
	inline StringType& ToLower( StringType& rText, const std::locale& loc = str::GetUserLocale() )
	{
		if ( !rText.empty() )
			std::transform( rText.begin(), rText.end(), rText.begin(), func::ToLower( loc ) );
		return rText;
	}

	template< typename StringType >
	inline StringType MakeUpper( const StringType& rText, const std::locale& loc = str::GetUserLocale() )
	{
		StringType upperText = rText;
		return ToUpper( upperText, loc );
	}

	template< typename StringType >
	inline StringType MakeLower( const StringType& rText, const std::locale& loc = str::GetUserLocale() )
	{
		StringType lowerText = rText;
		return ToLower( lowerText, loc );
	}


	enum CaseType { Case, IgnoreCase };


	template< str::CaseType caseType, typename CharType > bool Equals( const CharType* pLeft, const CharType* pRight );

	template<> inline bool Equals< Case, char >( const char* pLeft, const char* pRight ) { return pred::Equal == strcmp( pLeft, pRight ); }
	template<> inline bool Equals< IgnoreCase, char >( const char* pLeft, const char* pRight ) { return pred::Equal == _stricmp( pLeft, pRight ); }
	template<> inline bool Equals< Case, wchar_t >( const wchar_t* pLeft, const wchar_t* pRight ) { return pred::Equal == wcscmp( pLeft, pRight ); }
	template<> inline bool Equals< IgnoreCase, wchar_t >( const wchar_t* pLeft, const wchar_t* pRight ) { return pred::Equal == _wcsicmp( pLeft, pRight ); }

	template< str::CaseType caseType, typename StringType >
	bool EqualString( const StringType& left, const StringType& right ) { return Equals< caseType >( left.c_str(), right.c_str() ); }


	// string to STL iterators conversions
	typedef const TCHAR* const_iterator;

	inline size_t GetLength( const char* pText ) { return pText != NULL ? strlen( pText ) : 0; }
	inline size_t GetLength( const wchar_t* pText ) { return pText != NULL ? wcslen( pText ) : 0; }

	inline const char* begin( const char* pText ) { return pText; }
	inline const wchar_t* begin( const wchar_t* pText ) { return pText; }

	inline const char* end( const char* pText ) { return pText + GetLength( pText ); }
	inline const wchar_t* end( const wchar_t* pText ) { return pText + GetLength( pText ); }

	template< typename CharType > bool IsEmpty( const CharType* pText ) { return NULL == pText || 0 == *pText; }

	inline char* Copy( char* pBuffer, const std::string& text ) { return strcpy( pBuffer, text.c_str() ); }
	inline wchar_t* Copy( wchar_t* pBuffer, const std::wstring& text ) { return wcscpy( pBuffer, text.c_str() ); }

	template< typename CharType >
	inline const CharType* FindTokenEnd( const CharType* pText, const CharType delims[] )
	{
		const TCHAR* pTextEnd = str::end( pText );
		return std::find_first_of( pText, pTextEnd, delims, str::end( delims ) );
	}


	template< typename CharType >
	struct CPart
	{
		CPart( const CharType* pString = NULL, size_t count = 0 ) : m_pString( pString ), m_count( count ) { ASSERT_PTR( m_pString ); }

		bool IsEmpty( void ) const { return 0 == m_count; }
		std::basic_string< CharType > ToString( void ) const { ASSERT_PTR( m_pString ); return std::basic_string< CharType >( m_pString, m_count ); }
	public:
		const CharType* m_pString;
		size_t m_count;
	};

	template< typename CharType >
	CPart< CharType > MakePart( const CharType* pString, size_t count = std::tstring::npos )
	{
		return CPart< CharType >( pString, count != std::tstring::npos ? count : GetLength( pString ) );
	}
}


namespace str
{
	namespace ignore_case
	{
		template< typename CharType >
		inline bool operator==( const std::basic_string< CharType >& left, const std::basic_string< CharType >& right )
		{
			return str::Equals< IgnoreCase >( left.c_str(), right.c_str() );
		}

		template< typename CharType >
		inline bool operator==( const CharType* pLeft, const std::basic_string< CharType >& right )
		{
			return str::Equals< IgnoreCase >( pLeft, right.c_str() );
		}

		template< typename CharType >
		inline bool operator==( const std::basic_string< CharType >& left, const CharType* pRight )
		{
			return str::Equals< IgnoreCase >( left.c_str(), pRight );
		}


		template< typename CharType >
		inline bool operator!=( const std::basic_string< CharType >& left, const std::basic_string< CharType >& right )
		{
			return !operator==( left, right );
		}

		template< typename CharType >
		inline bool operator!=( const CharType* pLeft, const std::basic_string< CharType >& right )
		{
			return !operator==( pLeft, right );
		}

		template< typename CharType >
		inline bool operator!=( const std::basic_string< CharType >& left, const CharType* pRight )
		{
			return !operator==( left, pRight );
		}
	}
}


namespace func
{
	template< typename CharType >
	inline CharType toupper( CharType ch, const std::locale& loc = str::GetUserLocale() )
	{
		return std::toupper( ch, loc );
	}

	template< typename CharType >
	inline CharType tolower( CharType ch, const std::locale& loc = str::GetUserLocale() )
	{
		return std::tolower( ch, loc );
	}


	struct ToUpper
	{
		ToUpper( const std::locale& loc = str::GetUserLocale() ) : m_locale( loc ) {}

		template< typename CharType >
		CharType operator()( CharType ch ) const
		{
			return std::toupper( ch, m_locale );
		}
	private:
		const std::locale& m_locale;
	};


	struct ToLower
	{
		ToLower( const std::locale& loc = str::GetUserLocale() ) : m_locale( loc ) {}

		template< typename CharType >
		CharType operator()( CharType ch ) const
		{
			return std::tolower( ch, m_locale );
		}
	private:
		const std::locale& m_locale;
	};
}


namespace str
{
	inline void EnsureStdWhiteSpace( const char*& rpWhiteSpace ) { if ( NULL == rpWhiteSpace ) rpWhiteSpace = " \t"; }
	inline void EnsureStdWhiteSpace( const wchar_t*& rpWhiteSpace ) { if ( NULL == rpWhiteSpace ) rpWhiteSpace = L" \t"; }


	template< typename CharType >
	std::basic_string< CharType >& Trim( std::basic_string< CharType >& rText, const CharType* pWhiteSpace = NULL )
	{
		EnsureStdWhiteSpace( pWhiteSpace );
		size_t startPos = rText.find_first_not_of( pWhiteSpace ), endPos = rText.find_last_not_of( pWhiteSpace );

		if ( std::tstring::npos == startPos || std::tstring::npos == endPos || startPos > endPos )
			rText.clear();
		else
			rText = rText.substr( startPos, endPos - startPos + 1 );
		return rText;
	}

	template< typename CharType >
	std::basic_string< CharType >& TrimRight( std::basic_string< CharType >& rText, const TCHAR* pWhiteSpace = NULL )
	{
		EnsureStdWhiteSpace( pWhiteSpace );
		size_t endPos = rText.find_last_not_of( pWhiteSpace );
		if ( std::basic_string< CharType >::npos == endPos )
			rText.clear();
		else
			rText = rText.substr( 0, endPos + 1 );
		return rText;
	}

	template< typename CharType >
	std::basic_string< CharType >& TrimLeft( std::basic_string< CharType >& rText, const TCHAR* pWhiteSpace = NULL )
	{
		EnsureStdWhiteSpace( pWhiteSpace );
		size_t startPos = rText.find_first_not_of( pWhiteSpace );
		if ( std::basic_string< CharType >::npos == startPos )
			rText.clear();
		else
			rText = rText.substr( startPos );
		return rText;
	}

	inline CStringA& Trim( CStringA& rText ) { rText.TrimLeft(); rText.TrimRight(); return rText; }
	inline CStringW& Trim( CStringW& rText ) { rText.TrimLeft(); rText.TrimRight(); return rText; }

	inline BSTR AllocSysString( const std::tstring& text ) { return CString( text.c_str() ).AllocSysString(); }


	template< typename CharType >
	size_t Replace( std::basic_string< CharType >& rText, const CharType* pSearch, const CharType* pReplace )
	{
		ASSERT_PTR( pSearch );
		ASSERT_PTR( pReplace );
		size_t count = 0;

		if ( !str::IsEmpty( pSearch ) )
		{
			const size_t searchLen = str::GetLength( pSearch ), replaceLen = str::GetLength( pReplace );

			for ( size_t pos = 0; ( pos = rText.find( pSearch, pos ) ) != std::basic_string< CharType >::npos; ++count, pos += replaceLen )
				rText.replace( pos, searchLen, pReplace );
		}

		return count;
	}


	template< typename CharType >
	void SplitAdd( std::vector< std::basic_string< CharType > >& rItems, const CharType* pSource, const CharType* pSep )
	{
		ASSERT( !str::IsEmpty( pSep ) );

		if ( !str::IsEmpty( pSource ) )
		{
			const size_t sepLen = str::GetLength( pSep );
			typedef const CharType* const_iterator;

			for ( const_iterator itItemStart = str::begin( pSource ), itEnd = str::end( pSource ); ; )
			{
				const_iterator itItemEnd = std::search( itItemStart, itEnd, pSep, pSep + sepLen );
				if ( itItemEnd != itEnd )
				{
					rItems.push_back( std::basic_string< CharType >( itItemStart, std::distance( itItemStart, itItemEnd ) ) );
					itItemStart = itItemEnd + sepLen;
				}
				else
				{
					rItems.push_back( itItemStart );			// last item
					break;
				}
			}
		}
	}

	template< typename CharType >
	inline void Split( std::vector< std::basic_string< CharType > >& rItems, const CharType* pSource, const CharType* pSep )
	{
		rItems.clear();
		SplitAdd( rItems, pSource, pSep );
	}

	template< typename CharType >
	void QuickSplit( std::vector< CharType >& rItems, const CharType* pSource, CharType sepCh )
	{
		// build a vector copy of source characters replacing all sepCh with '\0'
		rItems.assign( str::begin( pSource ), str::end( pSource ) + 1 );			// copy the EOS char
		for ( std::vector< CharType >::iterator itItem = rItems.begin(); itItem != rItems.end(); ++itItem )
			if ( sepCh == *itItem )
				*itItem = 0;
	}

	template< typename CharType >
	void QuickTokenize( std::vector< CharType >& rItems, const CharType* pSource, const CharType* pSepTokens )
	{
		const CharType* pSepEnd = str::end( pSepTokens );
		// build a vector copy of source characters replacing any chars in pSepTokens with '\0'
		rItems.assign( str::begin( pSource ), str::end( pSource ) + 1 );			// copy the EOS char
		for ( std::vector< CharType >::iterator itItem = rItems.begin(); itItem != rItems.end(); ++itItem )
			if ( std::find( pSepTokens, pSepEnd, *itItem ) != pSepEnd )				// any sep token
				*itItem = 0;
	}


	template< typename CharType, typename Iterator >
	std::basic_string< CharType > Join( Iterator itFirstToken, Iterator itLastToken, const CharType* pSep )
	{	// works with any forward/reverse iterator
		std::basic_ostringstream< CharType > oss;
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
	template< typename CharType, typename ContainerType >
	inline std::basic_string< CharType > Join( const ContainerType& items, const CharType* pSep )
	{
		return Join( items.begin(), items.end(), pSep );
	}


	// works with container of any value type that has stream insertor defined
	//
	template< typename ContainerType >
	inline std::tstring FormatSet( const ContainerType& items, const TCHAR* pSep = _T(",") )
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

	template< typename StringType >
	struct EqualString
	{
		EqualString( const StringType& text, str::CaseType caseType = str::IgnoreCase ) : m_text( text ), m_caseType( caseType ) {}

		bool operator()( const StringType& itemText ) const
		{
			return str::IgnoreCase == m_caseType
				? str::Equals< str::IgnoreCase >( m_text.c_str(), itemText.c_str() )
				: str::Equals< str::Case >( m_text.c_str(), itemText.c_str() );
		}
	private:
		const StringType& m_text;
		str::CaseType m_caseType;
	};
}


namespace str
{
	template< typename ContainerType >
	void RemoveEmptyItems( ContainerType& rItems )
	{
		rItems.erase( std::remove_if( rItems.begin(), rItems.end(), pred::IsEmpty() ), rItems.end() );
	}


	template< typename StrContainerType >
	void TrimItems( StrContainerType& rItems )
	{
		for ( typename StrContainerType::iterator itLine = rItems.begin(); itLine != rItems.end(); ++itLine )
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


#include <iosfwd>


std::ostream& operator<<( std::ostream& os, const wchar_t* pWide );
std::wostream& operator<<( std::wostream& os, const char* pUtf8 );

inline std::ostream& operator<<( std::ostream& os, const std::wstring& wide ) { return os << wide.c_str(); }
inline std::wostream& operator<<( std::wostream& os, const std::string& utf8 ) { return os << utf8.c_str(); }
inline std::ostream& operator<<( std::ostream& os, const CString& mfcString ) { return os << mfcString.GetString(); }
inline std::wostream& operator<<( std::wostream& os, const CString& mfcString ) { return os << mfcString.GetString(); }


#endif // StringBase_h
