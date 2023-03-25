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
		template< typename CharT >
		bool operator()( CharT chr ) const
	#ifdef IS_CPP_11
		{ return std::isblank( chr, m_loc ); }
	#else
		{ return ' ' == chr || '\t' == chr; }
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


	// locale-independent predicates

	struct IsLineEnd : public BaseIsCharPred
	{
		bool operator()( wchar_t chr ) const { return '\r' == chr || '\n' == chr; }
	};

	struct IsSign : public BaseIsCharPred
	{
		bool operator()( wchar_t chr ) const { return '-' == chr || '+' == chr; }
	};


	template< str::CaseType caseType = str::Case >
	struct IsChar : public BaseIsCharPred
	{
		IsChar( wchar_t chMatch ) : m_chMatch( chMatch ) {}

		bool operator()( wchar_t chr ) const { return m_chMatch == chr; }
	private:
		wchar_t m_chMatch;		// wchar_t is also compatible with char
	};

	template<>
	struct IsChar<str::IgnoreCase>
	{
		IsChar( wchar_t chMatch ) : m_chMatch( func::toupper( chMatch ) ) {}

		bool operator()( wchar_t chr ) const
		{
			return m_chMatch == func::toupper( chr );
		}
	public:
		wchar_t m_chMatch;
	};


	template< typename CharSetT >
	struct IsCharAnyOf : public BaseIsCharPred
	{
		IsCharAnyOf( const CharSetT* pCharSet ) : m_pCharSet( pCharSet ) { ASSERT_PTR( m_pCharSet ); }

		bool operator()( wchar_t chr ) const { return str::IsAnyOf( static_cast<CharSetT>( chr ), m_pCharSet ); }
	private:
		const CharSetT* m_pCharSet;
	};
}


namespace func
{
	namespace C
	{
		struct BaseLocaleFunc		// character conversions based on classic C locale
		{
			BaseLocaleFunc( const std::locale& loc = std::locale::classic() ) : m_loc( loc ) {}
		public:
			const std::locale& m_loc;
		};


		struct ToLower : public BaseLocaleFunc
		{
			template< typename CharT >
			CharT operator()( CharT chr ) const { return std::tolower( chr, m_loc ); }
		};

		struct ToUpper : public BaseLocaleFunc
		{
			template< typename CharT >
			CharT operator()( CharT chr ) const { return std::toupper( chr, m_loc ); }
		};
	}


	struct CharKind			// for detecting word-breaks
	{
		enum Kind { Lower, Upper, Digit, Delim };

		Kind operator()( wchar_t chr ) const
		{
			if ( m_isDigit( chr ) )
				return Digit;
			else if ( m_isLower( chr ) )
				return Lower;
			else if ( m_isUpper( chr ) )
				return Upper;

			return Delim;
		}
	public:
		pred::IsLower m_isLower;
		pred::IsUpper m_isUpper;
		pred::IsDigit m_isDigit;
	};
}


namespace func
{
	struct ToChar			// character translator
	{
		template< typename CharT >
		CharT operator()( CharT ch ) const
		{
			return ch;
		}
	};


	struct ToCharPtr		// character-ptr translator
	{
		const char* operator()( const std::string& value ) const { return str::traits::GetCharPtr( value ); }
		const wchar_t* operator()( const std::wstring& value ) const { return str::traits::GetCharPtr( value ); }
		const TCHAR* operator()( const fs::CPath& value ) const { return str::traits::GetCharPtr( value ); }
	};


	// e.g. for combining func::ToLower with another character translation functor
	//
	template< typename BaseToCharFunc, typename AsCharFunc >
	struct ToCharAs : public BaseToCharFunc
	{
		ToCharAs( AsCharFunc asChar = AsCharFunc() ) : m_asChar( asChar ) {}

		template< typename CharT >
		CharT operator()( CharT chr ) const { return m_asChar( BaseToCharFunc::operator()( chr ) ); }
	public:
		const AsCharFunc m_asChar;
	};


	// conversion to int for evaluating the difference (mismatch) in value between characters - for strcmp() type of usage
	//
	template< str::CaseType caseType = str::Case >
	struct ToCharValue
	{
		int operator()( char chr ) const { return static_cast<int>( static_cast<unsigned char>( chr ) ); }
		int operator()( wchar_t chr ) const { return static_cast<int>( static_cast<unsigned short>( chr ) ); }
		int operator()( char32_t chr ) const { return static_cast<int>( static_cast<unsigned int>( chr ) ); }
	};

	template<>
	struct ToCharValue<str::IgnoreCase>
	{
		ToCharValue<str::IgnoreCase>( void ) : m_toLower() {}

		int operator()( char chr ) const { return static_cast<int>( static_cast<unsigned char>( m_toLower( chr ) ) ); }
		int operator()( wchar_t chr ) const { return static_cast<int>( static_cast<unsigned short>( m_toLower( chr ) ) ); }		// use 'unsigned short' since there is no 'unsigned wchar_t'
		int operator()( char32_t chr ) const { return static_cast<int>( static_cast<unsigned int>( m_toLower( chr ) ) ); }
	private:
		const func::ToLower m_toLower;
	};


	// abstract base for functors/predicates that use character translation: provides access to the translation functor
	//
	template< typename ToCharValueFunc >
	abstract class CharTranslateBase
	{
	protected:
		CharTranslateBase( ToCharValueFunc toCharValue = ToCharValueFunc() ) : m_toCharValue( toCharValue ) {}
	public:
		const ToCharValueFunc m_toCharValue;
	};
}


namespace pred
{
	template< typename ToCharFunc >
	struct CharEqualBase : public std::binary_function<wchar_t, wchar_t, bool>
	{
		CharEqualBase( ToCharFunc toChar = ToCharFunc() ) : m_toChar( toChar ) {}

		bool operator()( wchar_t chLeft, wchar_t chRight ) const
		{	// rely to implicit conversion to wchar_t to compare any combination of character types: char==char, wchar_t==wchar_t, char==wchar_t, wchar_t==char
			return m_toChar( chLeft ) == m_toChar( chRight );
		}
	public:
		const ToCharFunc m_toChar;
	};

	template<>
	struct CharEqualBase<func::ToChar> : public std::binary_function<wchar_t, wchar_t, bool>		// efficient specialization for straight character
	{
		bool operator()( wchar_t chLeft, wchar_t chRight ) const
		{
			return chLeft == chRight;
		}
	};


	template< typename ToCharValueFunc >
	inline CharEqualBase<ToCharValueFunc> MakeCharEquals( const func::CharTranslateBase<ToCharValueFunc>& translateFunc )
	{	// very useful: borrows the character translation functor to generate a char-equals predicate based on a str-equals functor that inherits from CharTranslateBase
		return CharEqualBase<ToCharValueFunc>( translateFunc.m_toCharValue );
	}



	// by case policy:

	template< str::CaseType caseType = str::Case >
	struct CharEqual : public std::binary_function<wchar_t, wchar_t, bool>
	{
		bool operator()( wchar_t chLeft, wchar_t chRight ) const
		{	// rely to implicit conversion to wchar_t to compare any combination of character types: char==char, wchar_t==wchar_t, char==wchar_t, wchar_t==char
			return chLeft == chRight;
		}
	};

	template<>
	struct CharEqual<str::IgnoreCase> : public std::binary_function<wchar_t, wchar_t, bool>
	{
		bool operator()( wchar_t chLeft, wchar_t chRight ) const
		{
			return func::toupper( chLeft ) == func::toupper( chRight );
		}
	};


	typedef CharEqual<str::Case> TCharEqualCase;
	typedef CharEqual<str::IgnoreCase> TCharEqualIgnoreCase;
}


namespace func
{
	// comparator of heterogenous character strings and characters, covering both compare() and compareN() use cases
	//
	template< typename ToCharValueFunc >
	struct StrCompareBase : public CharTranslateBase<ToCharValueFunc>
	{
		typedef CharTranslateBase<ToCharValueFunc> TBaseClass;
		using TBaseClass::m_toCharValue;

		StrCompareBase( ToCharValueFunc toCharValue = ToCharValueFunc() ) : TBaseClass( toCharValue ) {}
			// default ctor to prevent warning C4269: 'strCompare': 'const' automatic data initialized with compiler generated default constructor produces unreliable results

		template< typename LeftCharT, typename RightCharT >
		pred::CompareResult operator()( const LeftCharT* pLeft, const RightCharT* pRight, size_t count = std::tstring::npos ) const
		{
			return Compare( pLeft, pRight, count ).first;
		}

		template< typename LeftCharT, typename RightCharT >
		std::pair<pred::CompareResult, size_t> Compare( const LeftCharT* pLeft, const RightCharT* pRight, size_t count = std::tstring::npos ) const
		{	// returns a pair of CompareResult and the matching length for low-level matching analysis
			ASSERT( pLeft != nullptr && pRight != nullptr );
			int diff = 0;
			size_t matchLen = 0;

			if ( count != 0 )
				if ( !utl::SamePtr( pLeft, pRight ) )
				{
					while ( count-- != 0 &&
							0 == ( diff = m_toCharValue( *pLeft ) - m_toCharValue( *pRight ) ) &&
							*pLeft != '\0' && *pRight != '\0' )
					{
						++pLeft;
						++pRight;
						++matchLen;
					}
				}
				else
					matchLen = str::GetLength( pLeft );

			return std::pair<pred::CompareResult, size_t>( pred::ToCompareResult( diff ), matchLen );
		}
	};


	template< typename ToCharValueFunc >
	inline StrCompareBase<ToCharValueFunc> MakeStrCompareBase( ToCharValueFunc toCharValueFunc ) { return StrCompareBase<ToCharValueFunc>( toCharValueFunc ); }


	// compare by Case Policy:
#ifdef IS_CPP_11
	template< str::CaseType caseType = str::Case >
	using StrCompare = StrCompareBase< func::ToCharValue<caseType> >;		// declare an alias template
#else
	template< str::CaseType caseType = str::Case >
	struct StrCompare : public StrCompareBase< func::ToCharValue<caseType> >
	{
		StrCompare( void ) : StrCompareBase< func::ToCharValue<caseType> >() {}
	};
#endif

	typedef StrCompare<str::Case> TStrCompareCase;
	typedef StrCompare<str::IgnoreCase> TStrCompareIgnoreCase;
}


namespace pred
{
	// predicate for heterogenous character strings and characters, covering both equals() and equalsN() use cases
	//
	template< typename ToCharValueFunc >
	struct StrEqualsBase
	{
		template< typename LeftCharT, typename RightCharT >
		bool operator()( LeftCharT leftChr, RightCharT rightChr ) const
		{
			return m_compare.m_toCharValue( leftChr ) == m_compare.m_toCharValue( rightChr );
		}

		template< typename LeftCharT, typename RightCharT >
		bool operator()( const LeftCharT* pLeft, const RightCharT* pRight, size_t count = std::tstring::npos ) const		// equals/equalsN
		{
			return pred::Equal == m_compare( pLeft, pRight, count );
		}
	public:
		func::StrCompareBase<ToCharValueFunc> m_compare;
	};


	// equals by Case Policy:
#ifdef IS_CPP_11
	template< str::CaseType caseType = str::Case >
	using StrEquals = StrEqualsBase< func::ToCharValue<caseType> >;		// declare an alias template
#else
	template< str::CaseType caseType = str::Case >
	struct StrEquals : public StrEqualsBase< func::ToCharValue<caseType> >
	{
		StrEquals( void ) : StrEqualsBase< func::ToCharValue<caseType> >() {}
	};
#endif


	typedef StrEquals<str::Case> TStrEqualsCase;
	typedef StrEquals<str::IgnoreCase> TStrEqualsIgnoreCase;

	// stringy comparators for string-like objects
	typedef CompareAdapter<func::TStrCompareCase, func::ToCharPtr> TStringyCompareCase;
	typedef CompareAdapter<func::TStrCompareIgnoreCase, func::ToCharPtr> TStringyCompareNoCase;

	typedef LessValue<TStringyCompareCase> TLess_StringyCase;
	typedef LessValue<TStringyCompareNoCase> TLess_StringyNoCase;
}


namespace str
{
	// natural string compare

	pred::CompareResult IntuitiveCompare( const char* pLeft, const char* pRight );
	pred::CompareResult IntuitiveCompare( const wchar_t* pLeft, const wchar_t* pRight );
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

	template< typename CharT, typename ToCharValueFunc >
	inline pred::CompareResult _CompareN( const CharT* pLeft, const CharT* pRight, ToCharValueFunc toCharValueFunc, size_t count = std::tstring::npos )
	{
		return func::MakeStrCompareBase( toCharValueFunc )( pLeft, pRight, count );
	}


	// comparison interface (high-level)

	template< typename CharT >
	inline pred::CompareResult CompareIN( const CharT* pLeft, const CharT* pRight, size_t count ) { return func::StrCompare<str::IgnoreCase>()( pLeft, pRight, count ); }


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


namespace str
{
	template< typename CharT >
	inline bool ContainsI( const CharT* pText, wchar_t chr )
	{
		return str::Contains( pText, chr, func::ToLower() );
	}


	// sub-string search (iterator)

	template< typename IteratorT, typename StringT >
	inline IteratorT Search( IteratorT itFirst, IteratorT itLast, const StringT& seq )		// like strstr() - returns itLast if not found
	{
		return std::search( itFirst, itLast, seq.begin(), seq.end() );
	}

	template< typename IteratorT, typename StringT >
	inline IteratorT SearchI( IteratorT itFirst, IteratorT itLast, const StringT& seq )
	{
		return std::search( itFirst, itLast, seq.begin(), seq.end(), pred::CharEqual<str::IgnoreCase>() );
	}


	// sub-string find (position)

	template< str::CaseType caseType, typename CharT >
	size_t Find( const CharT* pText, wchar_t chr, size_t offset = 0 )
	{	// i.e. strchr - the zero terminating character is included in the search
		ASSERT( pText != nullptr && offset <= GetLength( pText ) );

		const CharT* pCursor = pText + offset;
		const pred::CharEqual<caseType> charEqual;

		while ( !charEqual( *pCursor, chr ) )
			if ( '\0' == *pCursor++ )		// reached the end?
				return utl::npos;

		return std::distance( pText, pCursor );
	}

	template< str::CaseType caseType, typename CharT, typename SeqCharT >
	size_t Find( const CharT* pText, const SeqCharT* pSequence, size_t seqLength = utl::npos, size_t offset = 0 )
	{
		ASSERT( pText != nullptr && offset <= GetLength( pText ) );

		if ( str::IsEmpty( pSequence ) )
			return offset;			// first match for empty sequence

		const pred::CharEqual<caseType> charEqual;
		const pred::StrEquals<caseType> strEquals;

		SettleLength( seqLength, pSequence );
		for ( const CharT* pCursor = pText + offset; *pCursor != '\0'; ++pCursor )
			if ( charEqual( *pCursor, *pSequence ) && strEquals( pCursor, pSequence, seqLength ) )
				return std::distance( pText, pCursor );

		return utl::npos;
	}


	template< str::CaseType caseType, typename CharT, typename SeqCharT >
	size_t FindLast( const CharT* pText, const SeqCharT* pSequence, size_t seqLength = utl::npos, size_t offset = utl::npos )
	{
		ASSERT_PTR( pText );

		str::SettleLength( offset, pText );
		str::SettleLength( seqLength, pSequence );

		const CharT* pEnd = pText + offset;
		const CharT* pSeqEnd = pSequence + seqLength;
		const pred::CharEqual<caseType> charEqual;

		const CharT* pFound = std::find_end( pText, pEnd, pSequence, pSeqEnd, charEqual );

		return pFound != pEnd ? std::distance( pText, pFound ) : utl::npos;
	}

	template< str::CaseType caseType, typename CharT, typename SeqCharT >
	inline size_t FindLast( const CharT* pText, SeqCharT chr, size_t offset = utl::npos )
	{
		return FindLast<caseType>( pText, &chr, 1, offset );
	}


	template< str::CaseType caseType, typename StringT, typename SeqCharT >
	size_t Replace( IN OUT StringT* pString, const SeqCharT* pSearch, const SeqCharT* pReplace, size_t maxCount = utl::npos )
	{
		ASSERT( pString != nullptr && pSearch != nullptr && pReplace != nullptr );
		size_t count = 0;

		if ( !str::IsEmpty( pSearch ) )
		{
			const size_t searchLen = str::GetLength( pSearch ), replaceLen = str::GetLength( pReplace );

			for ( size_t pos = 0;
				  count != maxCount && ( pos = str::Find<caseType>( pString->c_str(), pSearch, searchLen, pos ) ) != std::string::npos;
				  ++count, pos += replaceLen )
				pString->replace( pString->begin() + pos, pString->begin() + pos + searchLen, pReplace, pReplace + replaceLen );
		}
		else
			ASSERT( !str::IsEmpty( pSearch ) );		// warning assertion

		return count;
	}

	template< str::CaseType caseType, typename CharT, typename SeqCharT >
	size_t GetCountOf( const CharT* pText, const SeqCharT* pSequence )
	{
		size_t count = 0, seqLen = str::GetLength( pSequence );

		if ( !str::IsEmpty( pSequence ) )
			for ( size_t offset = 0;
				  ( offset = str::Find<caseType>( pText, pSequence, seqLen, offset ) ) != std::string::npos;
				  offset += seqLen )
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
			? ( str::Find<str::Case>( pText, pSequence ) != utl::npos )
			: ( str::Find<str::IgnoreCase>( pText, pSequence ) != utl::npos );
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

	typedef EvalMatch<func::ToChar, func::ToLower> TGetMatch;
}


namespace str
{
	// sequence (sub-string) search

	template< typename CharT >
	inline size_t FindSequence( const CharT* pText, const CSequence<CharT>& seq, size_t offset = 0 )
	{
		return Find<str::Case>( pText, seq.m_pSeq, seq.m_length, offset );
	}

	template< typename CharT, typename CharBinPredT >
	size_t FindSequence( const CharT* pText, const CSequence<CharT>& seq, CharBinPredT charEquals, size_t offset = 0 )		// e.g. pred::TCharEqualCase
	{
		ASSERT( pText != 0 && offset <= GetLength( pText ) );
		ASSERT( !seq.IsEmpty() );

		const CharT* pEnd = str::end( pText );
		const CharT* pFound = std::search( pText + offset, pEnd, seq.Begin(), seq.End(), charEquals );

		return pFound != pEnd ? std::distance( pText, pFound ) : std::tstring::npos;
	}

	template< typename CharT >
	inline bool ContainsSequence( const CharT* pText, const CSequence<CharT>& seq ) { return FindSequence( pText, seq ) != std::tstring::npos; }

	template< typename CharT, typename CharBinPredT >
	inline bool ContainsSequence( const CharT* pText, const CSequence<CharT>& seq, CharBinPredT charEquals ) { return FindSequence( pText, seq, charEquals ) != std::tstring::npos; }


	template< typename CharT, typename ContainerT >
	bool AllContain( const ContainerT& items, const str::CSequence<CharT>& seq )
	{
		for ( typename ContainerT::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
			if ( std::tstring::npos == FindSequence( itItem->c_str(), seq ) )
				return false;			// not a match for this item

		return !items.empty();
	}

	template< typename CharT, typename ContainerT, typename CharBinPredT >
	bool AllContain( const ContainerT& items, const str::CSequence<CharT>& seq, CharBinPredT charEquals )		// e.g. pred::TCharEqualCase
	{
		for ( typename ContainerT::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
			if ( std::tstring::npos == FindSequence( itItem->c_str(), seq, charEquals ) )
				return false;			// not a match for this item

		return !items.empty();
	}


	template< typename CharT >
	size_t GetSequenceCount( const CharT* pText, const CSequence<CharT>& seq )
	{
		size_t count = 0;

		if ( !seq.IsEmpty() )
			for ( size_t offset = 0; ( offset = str::FindSequence( pText, seq, offset ) ) != std::string::npos; offset += seq.m_length )
				++count;

		return count;
	}
}


namespace str
{
	template< typename CharT, typename CharBinPredT >
	size_t GetCommonLength( const CharT* pLeft, const CharT* pRight, CharBinPredT charEquals )		// e.g. pred::TCharEqualCase
	{
		size_t len = 0;
		while ( *pLeft != '\0' && charEquals( *pLeft++, *pRight++ ) )
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
