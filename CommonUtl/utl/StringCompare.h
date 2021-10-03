#ifndef StringCompare_h
#define StringCompare_h
#pragma once

#include "ComparePredicates.h"
#include "StringBase.h"
#include <locale>


namespace str
{
	pred::CompareResult IntuitiveCompare( const char* pLeft, const char* pRight );
	pred::CompareResult IntuitiveCompare( const wchar_t* pLeft, const wchar_t* pRight );
}


namespace func
{
	struct ToCharPtr		// character-ptr translator
	{
		const char* operator()( const std::string& value ) const { return str::traits::GetCharPtr( value ); }
		const wchar_t* operator()( const std::wstring& value ) const { return str::traits::GetCharPtr( value ); }
		const TCHAR* operator()( const fs::CPath& value ) const { return str::traits::GetCharPtr( value ); }
	};


	// character translators

	struct ToChar
	{
		template< typename CharT >
		CharT operator()( CharT ch ) const
		{
			return ch;
		}
	};
}


namespace pred
{
	// natural string compare

	template<>
	inline CompareResult Compare_Scalar< std::string >( const std::string& left, const std::string& right )			// by default sort std::string in natural order
	{
		return str::IntuitiveCompare( str::traits::GetCharPtr( left ), str::traits::GetCharPtr( right ) );
	}

	template<>
	inline CompareResult Compare_Scalar< std::wstring >( const std::wstring& left, const std::wstring& right )		// by default sort std::wstring in natural order
	{
		return str::IntuitiveCompare( str::traits::GetCharPtr( left ), str::traits::GetCharPtr( right ) );
	}


	struct CompareIntuitiveCharPtr
	{
		template< typename CharT >
		pred::CompareResult operator()( const CharT* pLeft, const CharT* pRight ) const
		{
			return str::IntuitiveCompare( pLeft, pRight );
		}
	};

	typedef CompareAdapter< CompareIntuitiveCharPtr, func::ToCharPtr > TStringyCompareIntuitive;		// for string-like objects
}


namespace str
{
	// comparison with character translation (low-level)

	template< typename CharT, typename ToCharFunc >
	std::pair< int, size_t > _BaseCompareDiff( const CharT* pLeft, const CharT* pRight, ToCharFunc toCharFunc, size_t count = std::tstring::npos )
	{
		ASSERT_PTR( pLeft );
		ASSERT_PTR( pRight );

		int firstMismatch = 0;
		size_t matchLen = 0;

		if ( count != 0 )
			if ( pLeft != pRight )
			{
				while ( count-- != 0 &&
						0 == ( firstMismatch = toCharFunc( *pLeft ) - toCharFunc( *pRight ) ) &&
						*pLeft != 0 && *pRight != 0 )
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

	template< typename CharT, typename ToCharFunc >
	inline std::pair< pred::CompareResult, size_t > _BaseCompare( const CharT* pLeft, const CharT* pRight, ToCharFunc toCharFunc, size_t count = std::tstring::npos )
	{
		std::pair< int, size_t > diffPair = _BaseCompareDiff( pLeft, pRight, toCharFunc, count );
		return std::pair< pred::CompareResult, size_t >( pred::ToCompareResult( diffPair.first ), diffPair.second );
	}

	template< typename CharT, typename ToCharFunc >
	inline pred::CompareResult _CompareN( const CharT* pLeft, const CharT* pRight, ToCharFunc toCharFunc, size_t count = std::tstring::npos )
	{
		return _BaseCompare( pLeft, pRight, toCharFunc, count ).first;
	}


	// comparison interface (high-level)

	template< typename CharT >
	inline pred::CompareResult CompareIN( const CharT* pLeft, const CharT* pRight, size_t count ) { return str::_CompareN( pLeft, pRight, &::toupper, count ); }


	template< typename CharT >
	inline bool EqualsN( const CharT* pLeft, const CharT* pRight, size_t count ) { return pred::Equal == CharTraits::CompareN( pLeft, pRight, count ); }

	template< typename CharT >
	inline bool EqualsIN( const CharT* pLeft, const CharT* pRight, size_t count ) { return pred::Equal == str::CompareIN( pLeft, pRight, count ); }

	template< typename CharT >
	inline bool EqualsN_ByCase( str::CaseType caseType, const CharT* pLeft, const CharT* pRight, size_t count )
	{
		return str::Case == caseType
			? EqualsN( pLeft, pRight, count )
			: EqualsIN( pLeft, pRight, count );
	}

	template< typename CharT >
	inline bool EqualsPart( const CharT* pLeft, const CPart<CharT>& rightPart, str::CaseType caseType = str::Case )
	{
		return EqualsN_ByCase( caseType, pLeft, rightPart.m_pString, rightPart.m_count );
	}


	template< typename CharT >
	bool IsUpperMatch( const CharT* pText, size_t count, const std::locale& loc = str::GetUserLocale() )
	{
		ASSERT_PTR( pText );

		for ( size_t pos = 0; pos != count; ++pos )
			if ( 0 == pText[ pos ] || !std::isupper( pText[ pos ], loc ) )
				return false;

		return count != 0;
	}

	template< typename CharT >
	bool IsLowerMatch( const CharT* pText, size_t count, const std::locale& loc = str::GetUserLocale() )
	{
		ASSERT_PTR( pText );

		for ( size_t pos = 0; pos != count; ++pos )
			if ( 0 == pText[ pos ] || !std::islower( pText[ pos ], loc ) )
				return false;

		return count != 0;
	}
}


namespace str
{
	template< typename CharT >
	inline bool HasPrefix( const CharT* pText, const CharT prefix[], size_t prefixLen = std::string::npos )		// empty prefix is always a match
	{
		return EqualsN( pText, prefix, prefixLen != std::string::npos ? prefixLen : GetLength( prefix ) );
	}

	template< typename CharT >
	inline bool HasPrefixI( const CharT* pText, const CharT prefix[], size_t prefixLen = std::string::npos )		// empty prefix is always a match
	{
		return EqualsIN( pText, prefix, prefixLen != std::string::npos ? prefixLen : GetLength( prefix ) );
	}


	template< typename CharT >
	bool HasSuffixN_ByCase( str::CaseType caseType, const CharT* pText, const CharT suffix[], size_t suffixLen )
	{	// empty suffix is always a match
		size_t textLength = GetLength( pText );
		return
			textLength >= suffixLen &&			// shorter pText is always a mismatch
			EqualsN_ByCase( caseType, pText + textLength - suffixLen, suffix, suffixLen );
	}

	template< typename CharT >
	inline bool HasSuffix( const CharT* pText, const CharT suffix[], size_t suffixLen = std::string::npos )
	{
		return HasSuffixN_ByCase( str::Case, pText, suffix, suffixLen != std::string::npos ? suffixLen : GetLength( suffix ) );
	}

	template< typename CharT >
	inline bool HasSuffixI( const CharT* pText, const CharT suffix[], size_t suffixLen = std::string::npos )
	{
		return HasSuffixN_ByCase( str::IgnoreCase, pText, suffix, suffixLen != std::string::npos ? suffixLen : GetLength( suffix ) );
	}
}


namespace str
{
	namespace ignore_case
	{
		template< typename CharT >
		inline bool operator==( const std::basic_string<CharT>& left, const std::basic_string<CharT>& right ) { return str::Equals< IgnoreCase >( left.c_str(), right.c_str() ); }

		template< typename CharT >
		inline bool operator==( const CharT* pLeft, const std::basic_string<CharT>& right ) { return str::Equals< IgnoreCase >( pLeft, right.c_str() ); }

		template< typename CharT >
		inline bool operator==( const std::basic_string<CharT>& left, const CharT* pRight ) { return str::Equals< IgnoreCase >( left.c_str(), pRight ); }


		template< typename CharT >
		inline bool operator!=( const std::basic_string<CharT>& left, const std::basic_string<CharT>& right ) { return !operator==( left, right ); }

		template< typename CharT >
		inline bool operator!=( const CharT* pLeft, const std::basic_string<CharT>& right ) { return !operator==( pLeft, right ); }

		template< typename CharT >
		inline bool operator!=( const std::basic_string<CharT>& left, const CharT* pRight ) { return !operator==( left, pRight ); }

		// don't bother defining operator<, it's not picked up by sorting algorithms due to Koenig lookup
	}
}


namespace str
{
	template< typename CharT >
	const CharT* SkipWhitespace( const CharT* pText, const CharT* pWhiteSpace = StdWhitespace<CharT>() )
	{
		ASSERT_PTR( pText );
		while ( ContainsAnyOf( pWhiteSpace, *pText ) )
			++pText;

		return pText;
	}

	template< typename CharT >
	const CharT* SkipHexPrefix( const CharT* pText, CaseType caseType = str::Case )
	{
		pText = SkipWhitespace( pText );

		const CharT hexPrefix[] = { '0', 'x', 0 };
		enum { HexPrefixLen = 2 };

		if ( str::EqualsN_ByCase( caseType, pText, hexPrefix, HexPrefixLen ) )
			pText += HexPrefixLen;

		return pText;
	}
}


namespace pred
{
	// string compare policies

	template< typename CharCasePolicy >
	struct CompareCharPtr		// rename to avoid collision with CompareStringW/A (<WinNls.h>)
	{
		template< typename CharT >
		CompareResult operator()( CharT left, CharT right ) const
		{
			return Compare_Scalar( m_casePolicy( left ), m_casePolicy( right ) );
		}

		template< typename CharT >
		CompareResult operator()( const CharT* pLeft, const CharT* pRight, size_t count = std::string::npos ) const
		{
			return str::_CompareN( pLeft, pRight, m_casePolicy, count );
		}
	private:
		CharCasePolicy m_casePolicy;
	};


	// character-ptr comparators
	typedef CompareCharPtr< func::ToChar > TCompareCase;
	typedef CompareCharPtr< func::ToUpper > TCompareNoCase;

	// stringy comparators for string-like objects
	typedef CompareAdapter< TCompareCase, func::ToCharPtr > TStringyCompareCase;
	typedef CompareAdapter< TCompareNoCase, func::ToCharPtr > TStringyCompareNoCase;
}


namespace pred
{
	template< typename CharT, str::CaseType caseType >
	struct CharMatch
	{
		CharMatch( CharT chMatch ) : m_chMatch( str::Case == caseType ? chMatch : func::toupper( chMatch ) ) {}

		bool operator()( CharT chr ) const
		{
			return m_chMatch == ( str::Case == caseType ? chr : func::toupper( chr ) );
		}
	public:
		CharT m_chMatch;
	};


	template< str::CaseType caseType >
	struct CharEqual
	{
		template< typename CharT >
		bool operator()( CharT left, CharT right ) const
		{
			return str::Case == caseType ? ( left == right ) : ( func::toupper( left ) == func::toupper( right ) );
		}

		template< typename CharT >
		bool operator()( const CharT* pLeft, const CharT* pRight ) const
		{
			return str::Equals< caseType >( pLeft, pRight );
		}
	};
}


namespace str
{
	// part (sub-string) search

	template< typename CharT >
	size_t FindPart( const CharT* pText, const CPart<CharT>& part, size_t offset = 0 )
	{
		ASSERT( pText != 0 && offset <= GetLength( pText ) );
		ASSERT( !part.IsEmpty() );

		const CharT* pEnd = str::end( pText );
		const CharT* pFound = std::search( pText + offset, pEnd, part.m_pString, part.m_pString + part.m_count );
		return pFound != pEnd ? std::distance( pText, pFound ) : std::tstring::npos;
	}

	template< typename CharT, typename Compare >
	size_t FindPart( const CharT* pText, const CPart<CharT>& part, Compare compareStr, size_t offset = 0 )				// e.g. pred::TCompareNoCase
	{
		ASSERT( pText != 0 && offset <= GetLength( pText ) );
		ASSERT( !part.IsEmpty() );

		const CharT* pEnd = str::end( pText );
		const CharT* pFound = std::search( pText + offset, pEnd, part.m_pString, part.m_pString + part.m_count, pred::IsEqual< Compare >( compareStr ) );
		return pFound != pEnd ? std::distance( pText, pFound ) : std::tstring::npos;
	}

	template< typename CharT >
	inline bool ContainsPart( const CharT* pText, const CPart<CharT>& part ) { return FindPart( pText, part ) != std::tstring::npos; }

	template< typename CharT, typename Compare >
	inline bool ContainsPart( const CharT* pText, const CPart<CharT>& part, Compare compareStr ) { return FindPart( pText, part, compareStr ) != std::tstring::npos; }


	template< typename CharT, typename ContainerT >
	bool AllContain( const ContainerT& items, const str::CPart<CharT>& part )
	{
		for ( typename ContainerT::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
			if ( std::tstring::npos == FindPart( itItem->c_str(), part ) )
				return false;			// not a match for this item

		return !items.empty();
	}

	template< typename CharT, typename ContainerT, typename Compare >
	bool AllContain( const ContainerT& items, const str::CPart<CharT>& part, Compare compareStr )		// e.g. pred::TCompareNoCase
	{
		for ( typename ContainerT::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
			if ( std::tstring::npos == FindPart( itItem->c_str(), part, compareStr ) )
				return false;			// not a match for this item

		return !items.empty();
	}

	template< typename CharT >
	size_t GetPartCount( const CharT* pText, const CPart<CharT>& part )
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
	template< typename CharT, typename CompareFunc >
	size_t GetCommonLength( const CharT* pLeft, const CharT* pRight, CompareFunc compareStr /*= pred::TCompareCase()*/ )
	{
		size_t len = 0;
		while ( *pLeft != 0 && pred::Equal == compareStr( *pLeft++, *pRight++ ) )
			++len;

		return len;
	}


	// custom case type

	template< str::CaseType caseType, typename CharT >
	const CharT* SkipPrefix( const CharT* pText, const CharT* pPrefix )
	{
		pred::CharEqual< caseType > eqChar;

		for ( const CharT* pSkip = pText; ; )
			if ( _T('\0') == *pPrefix )
				return pSkip;
			else if ( !eqChar( *pSkip++, *pPrefix++ ) )
				break;

		return pText;
	}

	template< str::CaseType caseType, typename CharT >
	size_t Find( const CharT* pText, CharT chr, size_t offset = 0 )
	{
		ASSERT( pText != 0 && offset <= GetLength( pText ) );

		const CharT* itEnd = end( pText );
		const CharT* itFound = std::find_if( begin( pText ) + offset, itEnd, pred::CharMatch< CharT, caseType >( chr ) );
		return itFound != itEnd ? std::distance( begin( pText ), itFound ) : std::tstring::npos;
	}

	template< str::CaseType caseType, typename CharT >
	size_t Find( const CharT* pText, const CharT* pPart, size_t offset = 0 )
	{
		ASSERT( pText != 0 && offset <= GetLength( pText ) );
		ASSERT( !str::IsEmpty( pPart ) );

		const CharT* itEnd = end( pText );
		const CharT* itFound = std::search( begin( pText ) + offset, itEnd, begin( pPart ), end( pPart ), pred::CharEqual< caseType >() );
		return itFound != itEnd ? std::distance( begin( pText ), itFound ) : std::tstring::npos;
	}

	template< str::CaseType caseType, typename CharT >
	size_t GetCountOf( const CharT* pText, const CharT* pPart )
	{
		size_t count = 0, matchLen = str::GetLength( pPart );

		if ( !str::IsEmpty( pPart ) )
			for ( size_t offset = str::Find< caseType >( pText, pPart ); offset != std::string::npos; offset = str::Find< caseType >( pText, pPart, offset + matchLen ) )
				++count;

		return count;
	}

	template< typename CharT >
	bool Matches( const CharT* pText, const CharT* pPart, bool matchCase, bool matchWhole )
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
		template< typename CharT >
		Match operator()( CharT leftCh, CharT rightCh ) const
		{
			if ( m_toNormalFunc( leftCh ) == m_toNormalFunc( rightCh ) )				// case sensitive match?
				return MatchEqual;
			if ( m_toEquivFunc( leftCh ) == m_toEquivFunc( rightCh ) )					// case insensitive match?
				return MatchEqualDiffCase;
			return MatchNotEqual;
		}

		template< typename CharT >
		Match operator()( const CharT* pLeft, const CharT* pRight ) const
		{
			if ( pred::Equal == str::_CompareN( pLeft, pRight, m_toNormalFunc ) )		// case sensitive match?
				return MatchEqual;
			if ( pred::Equal == str::_CompareN( pLeft, pRight, m_toEquivFunc ) )		// case insensitive match?
				return MatchEqualDiffCase;
			return MatchNotEqual;
		}
	private:
		ToNormalFunc m_toNormalFunc;		// for case-sensitive matching (with specialization for paths)
		ToEquivFunc m_toEquivFunc;			// for case-insensitive matching (with specialization for paths)
	};

	typedef EvalMatch< func::ToChar, func::ToUpper > GetMatch;
}


namespace pred
{
	// to find most occurences in a string from an array of a parts
	template< typename CharT >
	struct LessPartCount
	{
		LessPartCount( const CharT* pText ) : m_pText( pText ) { ASSERT_PTR( m_pText ); }

		bool operator()( const str::CPart<CharT>& left, const str::CPart<CharT>& right ) const
		{
			return Less == Compare_Scalar( str::GetPartCount( m_pText, left ), str::GetPartCount( m_pText, right ) );
		}

		bool operator()( const CharT* pLeftPart, const CharT* pRightPart ) const
		{
			return operator()( str::MakePart( pLeftPart ), str::MakePart( pRightPart ) );
		}
	private:
		const CharT* m_pText;
	};
}


#endif // StringCompare_h
