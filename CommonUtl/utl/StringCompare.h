#ifndef StringCompare_h
#define StringCompare_h
#pragma once

#include "Compare_fwd.h"
#include "StringBase.h"
#include <locale>


namespace str
{
	pred::CompareResult IntuitiveCompare( const char* pLeft, const char* pRight );
	pred::CompareResult IntuitiveCompare( const wchar_t* pLeft, const wchar_t* pRight );


	template< typename StringT >
	inline pred::CompareResult IntuitiveCompareStr( const StringT& left, const StringT& right )
	{
		return IntuitiveCompare( left.c_str(), right.c_str() );
	}
}


namespace str
{
	template< typename CharType, typename TranslateCharFunc >
	std::pair< int, size_t > BaseCompareDiff( const CharType* pLeft, const CharType* pRight, TranslateCharFunc translateFunc, size_t count = std::tstring::npos )
	{
		ASSERT_PTR( pLeft );
		ASSERT_PTR( pRight );

		int firstMismatch = 0;
		size_t matchLen = 0;
		if ( pLeft != pRight )
		{
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
			matchLen = str::GetLength( pLeft );

		return std::pair< int, size_t >( firstMismatch, matchLen );
	}

	template< typename CharType, typename TranslateCharFunc >
	inline std::pair< pred::CompareResult, size_t > BaseCompare( const CharType* pLeft, const CharType* pRight, TranslateCharFunc translateFunc, size_t count = std::tstring::npos )
	{
		std::pair< int, size_t > diffPair = BaseCompareDiff( pLeft, pRight, translateFunc, count );
		return std::pair< pred::CompareResult, size_t >( pred::ToCompareResult( diffPair.first ), diffPair.second );
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


	template< typename StringT >
	static inline int Compare( const StringT& left, const StringT& right ) { return left.compare( right ); }

	template< typename StringT >
	static inline int CompareNoCase( const StringT& left, const StringT& right ) { return CharTraits::CompareNoCase( left.c_str(), right.c_str() ); }

	template< typename CharType >
	bool EqualsN( const CharType* pLeft, const CharType* pRight, size_t count, bool matchCase )
	{
		return matchCase
			? pred::Equal == CharTraits::CompareN( pLeft, pRight, count )
			: pred::Equal == CharTraits::CompareN_NoCase( pLeft, pRight, count );
	}
}


namespace str
{
	template< typename CharType >
	const CharType* SkipWhitespace( const CharType* pText, const CharType* pWhiteSpace = StdWhitespace< CharType >() )
	{
		ASSERT_PTR( pText );
		while ( ContainsAnyOf( pWhiteSpace, *pText ) )
			++pText;

		return pText;
	}

	template< typename CharType >
	const CharType* SkipHexPrefix( const CharType* pText, CaseType caseType = str::Case )
	{
		pText = SkipWhitespace( pText );

		const CharType hexPrefix[] = { '0', 'x', 0 };
		enum { HexPrefixLen = 2 };

		if ( str::EqualsN( pText, hexPrefix, HexPrefixLen, str::Case == caseType ) )
			pText += HexPrefixLen;

		return pText;
	}
}


namespace func
{
	struct ToChar
	{
		template< typename CharType >
		CharType operator()( CharType ch ) const
		{
			return ch;
		}
	};
}


namespace pred
{
	template<>
	inline CompareResult Compare_Scalar< std::string >( const std::string& left, const std::string& right )
	{
		return str::IntuitiveCompareStr( left, right );
	}

	template<>
	inline CompareResult Compare_Scalar< std::wstring >( const std::wstring& left, const std::wstring& right )
	{
		return str::IntuitiveCompareStr( left, right );
	}


	// string compare policies

	template< typename CharCasePolicy >
	struct CompareString
	{
		template< typename CharType >
		CompareResult operator()( CharType left, CharType right ) const
		{
			return Compare_Scalar( m_casePolicy( left ), m_casePolicy( right ) );
		}

		template< typename CharType >
		CompareResult operator()( const CharType* pLeft, const CharType* pRight, size_t count = std::string::npos ) const
		{
			return str::CompareN( pLeft, pRight, m_casePolicy, count );
		}
	private:
		CharCasePolicy m_casePolicy;
	};


	typedef CompareString< func::ToChar > CompareCase;
	typedef CompareString< func::ToLower > CompareNoCase;


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
	// part (sub-string) search

	template< typename CharType >
	size_t FindPart( const CharType* pText, const CPart< CharType >& part, size_t offset = 0 )
	{
		ASSERT( pText != 0 && offset <= GetLength( pText ) );
		ASSERT( !part.IsEmpty() );

		const CharType* pEnd = str::end( pText );
		const CharType* pFound = std::search( pText + offset, pEnd, part.m_pString, part.m_pString + part.m_count );
		return pFound != pEnd ? std::distance( pText, pFound ) : std::tstring::npos;
	}

	template< typename CharType, typename Compare >
	size_t FindPart( const CharType* pText, const CPart< CharType >& part, Compare compareStr, size_t offset = 0 )				// e.g. pred::CompareNoCase
	{
		ASSERT( pText != 0 && offset <= GetLength( pText ) );
		ASSERT( !part.IsEmpty() );

		const CharType* pEnd = str::end( pText );
		const CharType* pFound = std::search( pText + offset, pEnd, part.m_pString, part.m_pString + part.m_count, pred::IsEqual< Compare >( compareStr ) );
		return pFound != pEnd ? std::distance( pText, pFound ) : std::tstring::npos;
	}

	template< typename CharType >
	inline bool ContainsPart( const CharType* pText, const CPart< CharType >& part ) { return FindPart( pText, part ) != std::tstring::npos; }

	template< typename CharType, typename Compare >
	inline bool ContainsPart( const CharType* pText, const CPart< CharType >& part, Compare compareStr ) { return FindPart( pText, part, compareStr ) != std::tstring::npos; }


	template< typename CharType, typename ContainerType >
	bool AllContain( const ContainerType& items, const str::CPart< CharType >& part )
	{
		for ( typename ContainerType::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
			if ( std::tstring::npos == FindPart( itItem->c_str(), part ) )
				return false;			// not a match for this item

		return !items.empty();
	}

	template< typename CharType, typename ContainerType, typename Compare >
	bool AllContain( const ContainerType& items, const str::CPart< CharType >& part, Compare compareStr )		// e.g. pred::CompareNoCase
	{
		for ( typename ContainerType::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
			if ( std::tstring::npos == FindPart( itItem->c_str(), part, compareStr ) )
				return false;			// not a match for this item

		return !items.empty();
	}

	template< typename CharType >
	size_t GetPartCount( const CharType* pText, const CPart< CharType >& part )
	{
		size_t count = 0;

		if ( !part.IsEmpty() )
			for ( size_t offset = str::FindPart( pText, part ); offset != std::string::npos; offset = str::FindPart( pText, part, offset + part.m_count ) )
				++count;

		return count;
	}
}


namespace str
{
	template< typename CharType, typename CompareFunc >
	size_t GetCommonLength( const CharType* pLeft, const CharType* pRight, CompareFunc compareStr /*= pred::CompareCase()*/ )
	{
		size_t len = 0;
		while ( *pLeft != 0 && pred::Equal == compareStr( *pLeft++, *pRight++ ) )
			len++;

		return len;
	}


	// custom case type

	template< str::CaseType caseType, typename CharType >
	const CharType* SkipPrefix( const CharType* pText, const CharType* pPrefix )
	{
		pred::CharEqual< caseType > eqChar;

		for ( const CharType* pSkip = pText; ; )
			if ( _T('\0') == *pPrefix )
				return pSkip;
			else if ( !eqChar( *pSkip++, *pPrefix++ ) )
				break;

		return pText;
	}

	template< str::CaseType caseType, typename CharType >
	size_t Find( const CharType* pText, CharType chr, size_t offset = 0 )
	{
		ASSERT( pText != 0 && offset <= GetLength( pText ) );

		const CharType* itEnd = end( pText );
		const CharType* itFound = std::find_if( begin( pText ) + offset, itEnd, pred::CharMatch< CharType, caseType >( chr ) );
		return itFound != itEnd ? std::distance( begin( pText ), itFound ) : std::tstring::npos;
	}

	template< str::CaseType caseType, typename CharType >
	size_t Find( const CharType* pText, const CharType* pPart, size_t offset = 0 )
	{
		ASSERT( pText != 0 && offset <= GetLength( pText ) );
		ASSERT( !str::IsEmpty( pPart ) );

		const CharType* itEnd = end( pText );
		const CharType* itFound = std::search( begin( pText ) + offset, itEnd, begin( pPart ), end( pPart ), pred::CharEqual< caseType >() );
		return itFound != itEnd ? std::distance( begin( pText ), itFound ) : std::tstring::npos;
	}

	template< str::CaseType caseType, typename CharType >
	size_t GetCountOf( const CharType* pText, const CharType* pPart )
	{
		size_t count = 0, matchLen = str::GetLength( pPart );

		if ( !str::IsEmpty( pPart ) )
			for ( size_t offset = str::Find< caseType >( pText, pPart ); offset != std::string::npos; offset = str::Find< caseType >( pText, pPart, offset + matchLen ) )
				++count;

		return count;
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


	enum Match { MatchEqual, MatchEqualDiffCase, MatchNotEqual };


	template< typename ToNormalFunc, typename ToEquivFunc >
	struct EvalMatch
	{
		template< typename CharType >
		Match operator()( CharType leftCh, CharType rightCh ) const
		{
			if ( m_toNormalFunc( leftCh ) == m_toNormalFunc( rightCh ) )				// case sensitive match?
				return MatchEqual;
			if ( m_toEquivFunc( leftCh ) == m_toEquivFunc( rightCh ) )					// case insensitive match?
				return MatchEqualDiffCase;
			return MatchNotEqual;
		}

		template< typename CharType >
		Match operator()( const CharType* pLeft, const CharType* pRight ) const
		{
			if ( pred::Equal == str::CompareN( pLeft, pRight, m_toNormalFunc ) )		// case sensitive match?
				return MatchEqual;
			if ( pred::Equal == str::CompareN( pLeft, pRight, m_toEquivFunc ) )			// case insensitive match?
				return MatchEqualDiffCase;
			return MatchNotEqual;
		}
	private:
		ToNormalFunc m_toNormalFunc;		// for case-sensitive matching (with specialization for paths)
		ToEquivFunc m_toEquivFunc;			// for case-insensitive matching (with specialization for paths)
	};

	typedef EvalMatch< func::ToChar, func::ToLower > GetMatch;
}


namespace pred
{
	// to find most occurences in a string from an array of a parts
	template< typename CharType >
	struct LessPartCount
	{
		LessPartCount( const CharType* pText ) : m_pText( pText ) { ASSERT_PTR( m_pText ); }

		bool operator()( const str::CPart< CharType >& left, const str::CPart< CharType >& right ) const
		{
			return Less == Compare_Scalar( str::GetPartCount( m_pText, left ), str::GetPartCount( m_pText, right ) );
		}

		bool operator()( const CharType* pLeftPart, const CharType* pRightPart ) const
		{
			return operator()( str::MakePart( pLeftPart ), str::MakePart( pRightPart ) );
		}
	private:
		const CharType* m_pText;
	};
}


#endif // StringCompare_h
