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
}


namespace func
{
	// character translators

	struct ToChar
	{
		template< typename CharType >
		CharType operator()( CharType ch ) const
		{
			return ch;
		}
	};
}


namespace str
{
	// comparison with character translation (low-level)

	template< typename CharType, typename TranslateFunc >
	std::pair< int, size_t > _BaseCompareDiff( const CharType* pLeft, const CharType* pRight, TranslateFunc transFunc, size_t count = std::tstring::npos )
	{
		ASSERT_PTR( pLeft );
		ASSERT_PTR( pRight );

		int firstMismatch = 0;
		size_t matchLen = 0;

		if ( count != 0 )
			if ( pLeft != pRight )
			{
				while ( count-- != 0 &&
						0 == ( firstMismatch = transFunc( *pLeft ) - transFunc( *pRight ) ) &&
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

	template< typename CharType, typename TranslateFunc >
	inline std::pair< pred::CompareResult, size_t > _BaseCompare( const CharType* pLeft, const CharType* pRight, TranslateFunc transFunc, size_t count = std::tstring::npos )
	{
		std::pair< int, size_t > diffPair = _BaseCompareDiff( pLeft, pRight, transFunc, count );
		return std::pair< pred::CompareResult, size_t >( pred::ToCompareResult( diffPair.first ), diffPair.second );
	}

	template< typename CharType, typename TranslateFunc >
	inline pred::CompareResult _CompareN( const CharType* pLeft, const CharType* pRight, TranslateFunc transFunc, size_t count = std::tstring::npos )
	{
		return _BaseCompare( pLeft, pRight, transFunc, count ).first;
	}


	// comparison interface (high-level)

	template< typename CharType >
	inline pred::CompareResult CompareIN( const CharType* pLeft, const CharType* pRight, size_t count ) { return str::_CompareN( pLeft, pRight, &::toupper, count ); }


	template< typename CharType >
	inline bool EqualsN( const CharType* pLeft, const CharType* pRight, size_t count ) { return pred::Equal == CharTraits::CompareN( pLeft, pRight, count ); }

	template< typename CharType >
	inline bool EqualsIN( const CharType* pLeft, const CharType* pRight, size_t count ) { return pred::Equal == str::CompareIN( pLeft, pRight, count ); }

	template< typename CharType >
	inline bool EqualsN_ByCase( str::CaseType caseType, const CharType* pLeft, const CharType* pRight, size_t count )
	{
		return str::Case == caseType
			? EqualsN( pLeft, pRight, count )
			: EqualsIN( pLeft, pRight, count );
	}
}


namespace str
{
	template< typename EqualsN_FuncT, typename CharType >
	bool _HasPrefix( EqualsN_FuncT pEqualsNFunc, const CharType* pText, const CharType prefix[], size_t prefixLen )
	{
		if ( std::string::npos == prefixLen )
			prefixLen = GetLength( prefix );

		return
			0 == prefixLen ||					// empty prefix is always a match
			pEqualsNFunc( pText, prefix, prefixLen );
	}

	template< typename CompareN_FuncT, typename CharType >
	bool _HasSuffix( CompareN_FuncT pCompareN, const CharType* pText, const CharType suffix[], size_t suffixLen )
	{
		if ( std::string::npos == suffixLen )
			suffixLen = GetLength( suffix );

		if ( 0 == suffixLen )					// empty suffix is always a match
			return true;

		size_t textLength = GetLength( pText );
		if ( textLength < suffixLen )
			return false;

		return pCompareN( pText + textLength - suffixLen, suffix, suffixLen );
	}


	template< typename CharType >
	inline bool HasPrefix( const CharType* pText, const CharType prefix[], size_t prefixLen = std::string::npos ) { return _HasPrefix( &EqualsN< CharType >, pText, prefix, prefixLen ); }

	template< typename CharType >
	inline bool HasPrefixI( const CharType* pText, const CharType prefix[], size_t prefixLen = std::string::npos ) { return _HasPrefix( &EqualsIN< CharType >, pText, prefix, prefixLen ); }


	template< typename CharType >
	inline bool HasSuffix( const CharType* pText, const CharType suffix[], size_t suffixLen = std::string::npos ) { return _HasSuffix( &EqualsN< CharType >, pText, suffix, suffixLen ); }

	template< typename CharType >
	inline bool HasSuffixI( const CharType* pText, const CharType suffix[], size_t suffixLen = std::string::npos ) { return _HasSuffix( &EqualsIN< CharType >, pText, suffix, suffixLen ); }
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
		template< typename CharType >
		CompareResult operator()( CharType left, CharType right ) const
		{
			return Compare_Scalar( m_casePolicy( left ), m_casePolicy( right ) );
		}

		template< typename CharType >
		CompareResult operator()( const CharType* pLeft, const CharType* pRight, size_t count = std::string::npos ) const
		{
			return str::_CompareN( pLeft, pRight, m_casePolicy, count );
		}
	private:
		CharCasePolicy m_casePolicy;
	};


	typedef CompareCharPtr< func::ToChar > TCompareCase;
	typedef CompareCharPtr< func::ToUpper > TCompareNoCase;


	template< typename CharType, str::CaseType caseType >
	struct CharMatch
	{
		CharMatch( CharType chMatch ) : m_chMatch( str::Case == caseType ? chMatch : func::toupper( chMatch ) ) {}

		bool operator()( CharType chr ) const
		{
			return m_chMatch == ( str::Case == caseType ? chr : func::toupper( chr ) );
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
			return str::Case == caseType ? ( left == right ) : ( func::toupper( left ) == func::toupper( right ) );
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
	size_t FindPart( const CharType* pText, const CPart< CharType >& part, Compare compareStr, size_t offset = 0 )				// e.g. pred::TCompareNoCase
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
	bool AllContain( const ContainerType& items, const str::CPart< CharType >& part, Compare compareStr )		// e.g. pred::TCompareNoCase
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
	size_t GetCommonLength( const CharType* pLeft, const CharType* pRight, CompareFunc compareStr /*= pred::TCompareCase()*/ )
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
