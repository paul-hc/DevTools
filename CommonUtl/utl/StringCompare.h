#ifndef StringCompare_h
#define StringCompare_h
#pragma once

#include "ComparePredicates.h"
#include "StringBase.h"
#include <locale>


namespace pred
{
	struct BaseIsCharPred : public std::unary_function<wchar_t, bool>
	{
	};

	struct BaseIsCharPred_Loc : public BaseIsCharPred		// character type based on classic C locale
	{
		BaseIsCharPred_Loc( const std::locale& loc = std::locale::classic() ) : m_loc( loc ) {}
	public:
		const std::locale& m_loc;
	};


	struct IsSpace : public BaseIsCharPred_Loc
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return std::isspace( chr, m_loc ); }
	};

	struct IsBlank : public BaseIsCharPred_Loc
	{
	#ifdef IS_CPP_11
		template< typename CharT >
		bool operator()( CharT chr ) const { return std::isblank( chr, m_loc ); }
	#else
		template< typename CharT >
		bool operator()( CharT chr ) const { return ' ' == chr || '\t' == chr; }
	#endif
	};

	struct IsPunct : public BaseIsCharPred_Loc
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return std::ispunct( chr, m_loc ); }
	};

	struct IsControl : public BaseIsCharPred_Loc	// \t, \n, \r, etc
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return std::iscntrl( chr, m_loc ); }
	};

	struct IsPrint : public BaseIsCharPred_Loc		// [0x20 - 0x7E]
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return std::isprint( chr, m_loc ); }
	};


	struct IsAlpha : public BaseIsCharPred_Loc
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return std::isalpha( chr, m_loc ); }
	};

	struct IsAlphaNum : public BaseIsCharPred_Loc
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return std::isalnum( chr, m_loc ); }
	};

	struct IsDigit : public BaseIsCharPred_Loc
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return std::isdigit( chr, m_loc ); }
	};

	struct IsHexDigit : public BaseIsCharPred_Loc
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return std::isxdigit( chr, m_loc ); }
	};


	struct IsIdentifier : public BaseIsCharPred_Loc			// e.g. C/C++ identifier, or Windows environment variable identifier, etc
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return '_' == chr || std::isalnum( chr, m_loc ); }
	};

	struct IsIdentifierLead : public BaseIsCharPred_Loc		// first character in a C/C++ identifier (non-digit)
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return '_' == chr || std::isalpha( chr, m_loc ); }
	};


	struct IsLower : public BaseIsCharPred_Loc
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return std::islower( chr, m_loc ); }
	};

	struct IsUpper : public BaseIsCharPred_Loc
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return std::isupper( chr, m_loc ); }
	};


	struct ToLower : public BaseIsCharPred_Loc
	{
		template< typename CharT >
		CharT operator()( CharT chr ) const { return std::tolower( chr, m_loc ); }
	};

	struct ToUpper : public BaseIsCharPred_Loc
	{
		template< typename CharT >
		CharT operator()( CharT chr ) const { return std::toupper( chr, m_loc ); }
	};


	struct IsChar : public BaseIsCharPred_Loc
	{
		IsChar( wchar_t chr ) : m_chr( chr ) {}

		template< typename CharT >
		bool operator()( CharT chr ) const { return m_chr == chr; }
	private:
		wchar_t m_chr;		// wchar_t is also compatible with char
	};


	template< typename CharSetT >
	struct IsCharAnyOf : public BaseIsCharPred_Loc
	{
		IsCharAnyOf( const CharSetT* pCharSet ) : m_pCharSet( pCharSet ) { ASSERT_PTR( m_pCharSet ); }

		template< typename CharT >
		bool operator()( CharT chr ) const { return str::IsAnyOf( static_cast<CharSetT>( chr ), m_pCharSet ); }
	private:
		const CharSetT* m_pCharSet;
	};


	struct IsLineEnd : public BaseIsCharPred_Loc
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return '\r' == chr || '\n' == chr; }
	};
}


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
	inline CompareResult Compare_Scalar<std::string>( const std::string& left, const std::string& right )			// by default sort std::string in natural order
	{
		return str::IntuitiveCompare( left.c_str(), right.c_str() );
	}

	template<>
	inline CompareResult Compare_Scalar<std::wstring>( const std::wstring& left, const std::wstring& right )		// by default sort std::wstring in natural order
	{
		return str::IntuitiveCompare( left.c_str(), right.c_str() );
	}


	struct CompareIntuitiveCharPtr
	{
		template< typename CharT >
		pred::CompareResult operator()( const CharT* pLeft, const CharT* pRight ) const
		{
			return str::IntuitiveCompare( pLeft, pRight );
		}
	};

	typedef CompareAdapter<CompareIntuitiveCharPtr, func::ToCharPtr> TStringyCompareIntuitive;		// for string-like objects
	typedef LessValue<TStringyCompareIntuitive> TLess_StringyIntuitive;
}


namespace str
{
	// comparison with character translation (low-level)

	template< typename CharT, typename ToCharFunc >
	std::pair<int, size_t> _BaseCompareDiff( const CharT* pLeft, const CharT* pRight, ToCharFunc toCharFunc, size_t count = std::tstring::npos )
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

		return std::pair<int, size_t>( firstMismatch, matchLen );
	}

	template< typename CharT, typename ToCharFunc >
	inline std::pair<pred::CompareResult, size_t> _BaseCompare( const CharT* pLeft, const CharT* pRight, ToCharFunc toCharFunc, size_t count = std::tstring::npos )
	{
		std::pair<int, size_t> diffPair = _BaseCompareDiff( pLeft, pRight, toCharFunc, count );
		return std::pair<pred::CompareResult, size_t>( pred::ToCompareResult( diffPair.first ), diffPair.second );
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
	inline bool EqualsSequence( const CharT* pLeft, const CSequence<CharT>& rightSequence, str::CaseType caseType = str::Case )
	{
		return EqualsN_ByCase( caseType, pLeft, rightSequence.m_pStr, rightSequence.m_count );
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
	inline bool HasPrefixI( const CharT* pText, const CharT prefix[], size_t prefixLen = std::string::npos )	// empty prefix is always a match
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
		inline bool operator==( const std::basic_string<CharT>& left, const std::basic_string<CharT>& right ) { return str::Equals<IgnoreCase>( left.c_str(), right.c_str() ); }

		template< typename CharT >
		inline bool operator==( const CharT* pLeft, const std::basic_string<CharT>& right ) { return str::Equals<IgnoreCase>( pLeft, right.c_str() ); }

		template< typename CharT >
		inline bool operator==( const std::basic_string<CharT>& left, const CharT* pRight ) { return str::Equals<IgnoreCase>( left.c_str(), pRight ); }


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
		while ( IsAnyOf( *pText, pWhiteSpace ) )
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
	typedef CompareCharPtr<func::ToChar> TCompareCase;
	typedef CompareCharPtr<func::ToUpper> TCompareNoCase;

	// stringy comparators for string-like objects
	typedef CompareAdapter<TCompareCase, func::ToCharPtr> TStringyCompareCase;
	typedef CompareAdapter<TCompareNoCase, func::ToCharPtr> TStringyCompareNoCase;

	typedef LessValue<TStringyCompareCase> TLess_StringyCase;
	typedef LessValue<TStringyCompareNoCase> TLess_StringyNoCase;
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
			return str::Equals<caseType>( pLeft, pRight );
		}
	};
}


namespace str
{
	// sequence (sub-string) search

	template< typename IteratorT, typename StringT >
	inline IteratorT Search( IteratorT itFirst, IteratorT itLast, const StringT& seq )		// like strstr() - returns itLast if not found
	{
		return std::search( itFirst, itLast, seq.begin(), seq.end() );
	}

	template< typename CharT >
	size_t FindSequence( const CharT* pText, const CSequence<CharT>& seq, size_t offset = 0 )
	{
		ASSERT( pText != 0 && offset <= GetLength( pText ) );
		ASSERT( !seq.IsEmpty() );

		const CharT* pEnd = str::end( pText );
		const CharT* pFound = std::search( pText + offset, pEnd, seq.Begin(), seq.End() );

		return pFound != pEnd ? std::distance( pText, pFound ) : std::tstring::npos;
	}

	template< typename CharT, typename Compare >
	size_t FindSequence( const CharT* pText, const CSequence<CharT>& seq, Compare compareStr, size_t offset = 0 )				// e.g. pred::TCompareNoCase
	{
		ASSERT( pText != 0 && offset <= GetLength( pText ) );
		ASSERT( !seq.IsEmpty() );

		const CharT* pEnd = str::end( pText );
		const CharT* pFound = std::search( pText + offset, pEnd, seq.Begin(), seq.End(), pred::IsEqual<Compare>( compareStr ) );

		return pFound != pEnd ? std::distance( pText, pFound ) : std::tstring::npos;
	}

	template< typename CharT >
	inline bool ContainsSequence( const CharT* pText, const CSequence<CharT>& seq ) { return FindSequence( pText, seq ) != std::tstring::npos; }

	template< typename CharT, typename Compare >
	inline bool ContainsSequence( const CharT* pText, const CSequence<CharT>& seq, Compare compareStr ) { return FindSequence( pText, seq, compareStr ) != std::tstring::npos; }


	template< typename CharT, typename ContainerT >
	bool AllContain( const ContainerT& items, const str::CSequence<CharT>& seq )
	{
		for ( typename ContainerT::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
			if ( std::tstring::npos == FindSequence( itItem->c_str(), seq ) )
				return false;			// not a match for this item

		return !items.empty();
	}

	template< typename CharT, typename ContainerT, typename Compare >
	bool AllContain( const ContainerT& items, const str::CSequence<CharT>& seq, Compare compareStr )		// e.g. pred::TCompareNoCase
	{
		for ( typename ContainerT::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
			if ( std::tstring::npos == FindSequence( itItem->c_str(), seq, compareStr ) )
				return false;			// not a match for this item

		return !items.empty();
	}

	template< typename CharT >
	size_t GetSequenceCount( const CharT* pText, const CSequence<CharT>& seq )
	{
		size_t count = 0;

		if ( !seq.IsEmpty() )
			for ( size_t offset = str::FindSequence( pText, seq ); offset != std::string::npos; offset = str::FindSequence( pText, seq, offset + seq.m_length ) )
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
		pred::CharEqual<caseType> eqChar;

		for ( const CharT* pSkip = pText; ; )
			if ( '\0' == *pPrefix )
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
		const CharT* itFound = std::find_if( begin( pText ) + offset, itEnd, pred::CharMatch<CharT, caseType>( chr ) );
		return itFound != itEnd ? std::distance( begin( pText ), itFound ) : std::tstring::npos;
	}

	template< str::CaseType caseType, typename CharT >
	size_t Find( const CharT* pText, const CharT* pSequence, size_t offset = 0 )
	{
		ASSERT( pText != 0 && offset <= GetLength( pText ) );
		ASSERT( !str::IsEmpty( pSequence ) );

		const CharT* itEnd = end( pText );
		const CharT* itFound = std::search( begin( pText ) + offset, itEnd, begin( pSequence ), end( pSequence ), pred::CharEqual<caseType>() );
		return itFound != itEnd ? std::distance( begin( pText ), itFound ) : std::tstring::npos;
	}

	template< str::CaseType caseType, typename CharT >
	size_t GetCountOf( const CharT* pText, const CharT* pSequence )
	{
		size_t count = 0, matchLen = str::GetLength( pSequence );

		if ( !str::IsEmpty( pSequence ) )
			for ( size_t offset = str::Find<caseType>( pText, pSequence ); offset != std::string::npos; offset = str::Find<caseType>( pText, pSequence, offset + matchLen ) )
				++count;

		return count;
	}

	template< typename CharT >
	bool Matches( const CharT* pText, const CharT* pSequence, bool matchCase, bool matchWhole )
	{
		if ( matchWhole )
			return matchCase
				? str::Equals<str::Case>( pText, pSequence )
				: str::Equals<str::IgnoreCase>( pText, pSequence );

		return matchCase
			? ( str::Find<str::Case>( pText, pSequence ) != std::tstring::npos )
			: ( str::Find<str::IgnoreCase>( pText, pSequence ) != std::tstring::npos );
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

	typedef EvalMatch<func::ToChar, func::ToUpper> TGetMatch;
}


namespace pred
{
	// to find most occurences in a string from an array of a sequences
	template< typename CharT >
	struct LessSequenceCount
	{
		LessSequenceCount( const CharT* pText ) : m_pText( pText ) { ASSERT_PTR( m_pText ); }

		bool operator()( const str::CSequence<CharT>& left, const str::CSequence<CharT>& right ) const
		{
			return Less == Compare_Scalar( str::GetSequenceCount( m_pText, left ), str::GetSequenceCount( m_pText, right ) );
		}

		bool operator()( const CharT* pLeftSeq, const CharT* pRightSeq ) const
		{
			return operator()( str::MakeSequence( pLeftSeq ), str::MakeSequence( pRightSeq ) );
		}
	private:
		const CharT* m_pText;
	};
}


#endif // StringCompare_h
