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


	template< typename StringType >
	inline pred::CompareResult IntuitiveCompareStr( const StringType& left, const StringType& right )
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


	template< typename StringType >
	static inline int Compare( const StringType& left, const StringType& right ) { return left.compare( right ); }

	template< typename StringType >
	static inline int CompareNoCase( const StringType& left, const StringType& right ) { return CharTraits::CompareNoCase( left.c_str(), right.c_str() ); }

	template< typename CharType >
	bool EqualsN( const CharType* pLeft, const CharType* pRight, size_t count, bool matchCase )
	{
		return matchCase
			? pred::Equal == CharTraits::CompareN( pLeft, pRight, count )
			: pred::Equal == CharTraits::CompareN_NoCase( pLeft, pRight, count );
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
	template< str::CaseType caseType, typename CharType >
	size_t Find( const CharType* pText, CharType chr, size_t startPos = 0 )
	{
		ASSERT( pText != 0 && startPos <= GetLength( pText ) );

		const CharType* itEnd = end( pText );
		const CharType* itFound = std::find_if( begin( pText ) + startPos, itEnd, pred::CharMatch< CharType, caseType >( chr ) );
		return itFound != itEnd ? ( itFound - begin( pText ) ) : std::tstring::npos;
	}

	template< str::CaseType caseType, typename CharType >
	size_t Find( const CharType* pText, const CharType* pPart, size_t startPos = 0 )
	{
		ASSERT( pText != 0 && startPos <= GetLength( pText ) );
		ASSERT( !str::IsEmpty( pPart ) );

		const CharType* itEnd = end( pText );
		const CharType* itFound = std::search( begin( pText ) + startPos, itEnd, begin( pPart ), end( pPart ), pred::CharEqual< caseType >() );
		return itFound != itEnd ? ( itFound - begin( pText ) ) : std::tstring::npos;
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


#endif // StringCompare_h
