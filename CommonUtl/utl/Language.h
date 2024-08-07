#ifndef Language_h
#define Language_h
#pragma once

#include "Code_fwd.h"
#include "StringParsing.h"
#include "Range.h"
#include <vector>


namespace code
{
	bool IsBracket( wchar_t bracket );				// works for both char/wchar_t
	bool IsStartBracket( wchar_t bracket );
	bool IsEndBracket( wchar_t bracket );

	wchar_t _GetMatchingBracket( wchar_t bracket );

	template< typename CharT >
	inline CharT ToMatchingBracket( CharT bracket ) { return static_cast<CharT>( _GetMatchingBracket( bracket ) ); }


	template< typename CharT >
	class CLanguage;

	template< typename CharT >
	const CLanguage<CharT>& GetLangCpp( void );


	class CEscaper;

	const CEscaper& GetEscaperC( void );
}


namespace pred
{
	struct IsBracket : public BaseIsCharPred
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return code::IsBracket( chr ); }
	};

	struct IsStartBracket : public BaseIsCharPred
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return code::IsStartBracket( chr ); }
	};

	struct IsEndBracket : public BaseIsCharPred
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return code::IsEndBracket( chr ); }
	};


	struct ToMatchingBracket : public BaseIsCharPred
	{
		template< typename CharT >
		CharT operator()( CharT chr ) const { return code::ToMatchingBracket( chr ); }
	};
}


namespace str
{
	// sub-string of a range of iterators/positions for a given string:

	template< typename IteratorT >
	std::basic_string<typename IteratorT::value_type> ExtractString( const Range<IteratorT>& itRange )
	{
		std::basic_string<typename IteratorT::value_type> text( itRange.m_start, itRange.m_end );

		if ( ReverseIter == GetIterationDir( itRange.m_start ) )
			std::reverse( text.begin(), text.end() );

		return text;
	}

	template< typename StringT, typename PosT >
	inline StringT ExtractString( const Range<PosT>& posRange, const StringT& text )
	{
		return text.substr( posRange.m_start, posRange.m_end - posRange.m_start );
	}


	// convert from iterator range to position range:

	template< typename IteratorT >
	inline Range<size_t> MakePosRange( const Range<IteratorT>& itRange, IteratorT itBegin )
	{
		REQUIRE( ForwardIter == GetIterationDir( itRange.m_start ) );
		return Range<size_t>( std::distance( itBegin, itRange.m_start ), std::distance( itBegin, itRange.m_end ) );
	}

	template< typename ContainerT >
	inline Range<size_t> MakeFwdPosRange( const Range<typename ContainerT::const_reverse_iterator>& itRange, const ContainerT& items )
	{	// range of FORWARD positions
		REQUIRE( ReverseIter == GetIterationDir( itRange.m_start ) );

		// Note: for semantic equivalence with the forward iteration range, we need to increment both positions by 1.
		// So instead of using utl::FwdPosOfRevIter (which decrements by 1), we compute the raw distance from the REV_END, i.e. FWD_BEGIN.
		// This is to maintain compatibility with ExtractString( const Range<PosT>& posRange, ... ).

		Range<size_t> posRange( std::distance( itRange.m_end, items.rend() ), std::distance( itRange.m_start, items.rend() ) );
		return posRange;
	}
}


namespace code
{
	// A language syntax is defined by pairs of separators that define quoted strings, comments, etc.
	//	note: brackets are excluded since they need special parsing rules (must be avoided inside quoted strings, comments).
	//
	template< typename CharT >
	class CLanguage : private utl::noncopyable
	{
	public:
		typedef typename str::CEnclosedParser<CharT>::TString TString;
		typedef typename str::CEnclosedParser<CharT>::TSepMatchPos TSepMatchPos;
		typedef typename str::CEnclosedParser<CharT>::TSeparatorsPair TSeparatorsPair;
		typedef typename str::CSeparatorsPair<CharT>::TSepVector TSepVector;

		CLanguage( const char* pStartSepList, const char* pEndSepList, const CEscaper* pEscaper = nullptr )		// constructor using only narrow strings for language constructs
			: m_parser( false, pStartSepList, pEndSepList )
			, m_pEscaper( pEscaper )
			, m_skipArgLists( false )						// by default disabled, so the client code has to skip them if needed
		{
			GetSeparatorsPair().EnableLineComments();		// by default assume the last separator pair refers to single-line comments
		}

		const str::CEnclosedParser<CharT>& GetParser( void ) const { return m_parser; }
		const TSeparatorsPair& GetSeparatorsPair( void ) const { return m_parser.GetSeparators(); }
		const CEscaper* GetEscaper( void ) const { return m_pEscaper; }

		bool& RefSkipArgLists( void ) const { return m_skipArgLists; }		// mutable data-member


		// iterator search

		template< typename SeqStringT, typename IteratorT >
		IteratorT FindNextSequenceT( IteratorT itCode, IteratorT itLast, const SeqStringT& sequence ) const
		{	// note: sequence can be of a different string type - for parsing TCHAR code with char language elements (operators, etc)
			str::IterationDir iterDir = str::GetIterationDir( itCode );
			const TSeparatorsPair& sepsPair = m_parser.GetSeparators();

			for ( TSepMatchPos sepMatchPos = TString::npos; itCode != itLast; )
			{
				if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, itCode, itLast ) )	// matches a quoted string, comment, etc?
					sepsPair.SkipMatchingSpec( &itCode, itLast, sepMatchPos );	// skip the entire spec
				else if ( str::EqualsSeq( itCode, itLast, sequence ) )
				{
					if ( str::ReverseIter == iterDir )
						itCode += sequence.length() - 1;	// advance to r-beginning of the match

					return itCode;				// found the next bracket
				}
				else if ( m_skipArgLists && IsBracket( *itCode ) )
					SkipPastMatchingBracket( &itCode, itLast );
				else
					++itCode;
			}
			return itLast;
		}

		template< typename IteratorT >
		inline IteratorT FindNextSequence( IteratorT itCode, IteratorT itLast, const TString& sequence ) const
		{	// sequence of the same string type
			return FindNextSequenceT<TString>( itCode, itLast, sequence );
		}


		template< typename IteratorT >
		inline IteratorT FindNextChar( IteratorT itCode, IteratorT itLast, wchar_t chr ) const
		{
			return FindNextCharThat( itCode, itLast, pred::IsChar<>( chr ) );
		}

		template< typename IteratorT, typename IsCharPred >
		IteratorT FindNextCharThat( IteratorT itCode, IteratorT itLast, IsCharPred isCharPred ) const
		{
			const TSeparatorsPair& sepsPair = m_parser.GetSeparators();

			for ( TSepMatchPos sepMatchPos = TString::npos; itCode != itLast; )
			{
				if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, itCode, itLast ) )		// matches a quoted string, comment, etc?
					sepsPair.SkipMatchingSpec( &itCode, itLast, sepMatchPos );			// skip the entire spec
				else if ( isCharPred( *itCode ) )
					return itCode;				// found the next match
				else if ( m_skipArgLists && IsBracket( *itCode ) )
					SkipPastMatchingBracket( &itCode, itLast );
				else
					++itCode;
			}
			return itLast;
		}

		template< typename IteratorT, typename IsCharPred >
		inline IteratorT FindNextCharThatNot( IteratorT itCode, IteratorT itLast, IsCharPred isCharPred ) const
		{
			return FindNextCharThat( itCode, itLast, std::not1( isCharPred ) );
		}


		// skipping methods

		template< typename IteratorT, typename IsCharPred >
		bool SkipWhile( IN OUT IteratorT* pItCode, IteratorT itLast, IsCharPred isCharPred ) const
		{	// skips to the next position that does not satisfy the predicate
			ASSERT_PTR( pItCode );

			IteratorT itFound = FindNextCharThatNot( *pItCode, itLast, isCharPred );
			if ( itFound == *pItCode )
				return false;				// no advance

			*pItCode = itFound;
			return true;					// true if found non-isCharPred
		}

		template< typename IteratorT, typename IsCharPred >
		bool SkipUntil( IN OUT IteratorT* pItCode, IteratorT itLast, IsCharPred isCharPred ) const
		{	// skips to the next position that satisfies the predicate
			ASSERT_PTR( pItCode );

			IteratorT itFound = FindNextCharThat( *pItCode, itLast, isCharPred );
			if ( itFound == *pItCode )
				return false;				// no advance

			*pItCode = itFound;
			return true;					// true if found isCharPred
		}

		template< typename IteratorT, typename IsCharPred >
		size_t SkipBackWhile( IN OUT IteratorT* pItCode, IteratorT itFirst, IsCharPred isCharPred ) const
		{	// skips to the first previous position that satisfies the predicate (e.g. is space) => Note: no separators parsing, no smarties, so use it for skiping spaces, not semantically rich stuff!
			ASSERT_PTR( pItCode );
			size_t count = 0;

			for ( ; *pItCode != itFirst && isCharPred( *( *pItCode - 1 ) ); ++count )
				--*pItCode;

			return count;
		}


		template< typename IteratorT >
		inline bool SkipWhitespace( IN OUT IteratorT* pItCode, IteratorT itLast ) const { return SkipWhile( pItCode, itLast, pred::IsSpace() ); }

		template< typename IteratorT >
		inline bool SkipBlank( IN OUT IteratorT* pItCode, IteratorT itLast ) const { return SkipWhile( pItCode, itLast, pred::IsBlank() ); }

		template< typename IteratorT >
		inline bool SkipIdentifier( IN OUT IteratorT* pItCode, IteratorT itLast ) const { return SkipWhile( pItCode, itLast, pred::IsIdentifier() ); }


		// bracket lookup

		template< typename IteratorT >
		IteratorT FindMatchingBracket( IteratorT itBracket, IteratorT itLast, OUT IteratorT* pItBracketMismatch = nullptr ) const throws_cond( code::TSyntaxError );

		template< typename IteratorT >
		bool SkipPastMatchingBracket( IN OUT IteratorT* pItBracket, IteratorT itLast, OUT IteratorT* pItBracketMismatch = nullptr ) const throws_cond( code::TSyntaxError );

		template< typename IteratorT >
		IteratorT FindNextBracket( IteratorT itCode, IteratorT itLast ) const
		{
			return FindNextCharThat( itCode, itLast, &code::IsBracket );
		}
	public:
		class CScopedSkipArgLists
		{
		public:
			CScopedSkipArgLists( const CLanguage* pLang, bool skipArgLists = true )
				: m_pLang( safe_ptr( pLang ) )
				, m_origSkipArgLists( m_pLang->RefSkipArgLists() )
			{
				m_pLang->RefSkipArgLists() = skipArgLists;
			}

			~CScopedSkipArgLists()
			{
				m_pLang->RefSkipArgLists() = m_origSkipArgLists;
			}
		private:
			const CLanguage* m_pLang;
			bool m_origSkipArgLists;					// when searching for sequences or characters, parses and skips the embedded bracket-delimited arg-lists
		};
	private:
		const str::CEnclosedParser<CharT> m_parser;		// contains pairs of START/END separators of specific language constructs (for skipping)
		const CEscaper* m_pEscaper;
		mutable bool m_skipArgLists;					// if true: when searching for sequences or characters, parses and skips the embedded bracket-delimited arg-lists
	};
}


namespace code
{
	class CEscaper
	{
	public:
		CEscaper( char escSeqLead, const char* pEscaped, const char* pActual, const char* pEncNewLineContinue, const char* pEncRetain, char hexPrefix, bool parseOctalLiteral );

		template< typename StringT >
		StringT Decode( const StringT& srcLiteral ) const;													// parse to actual code

		template< typename StringT >
		StringT Encode( const StringT& actualCode, bool enquote, const char* pRetain = nullptr ) const;		// format to literal text

		// pointer interface:
		template< typename CharT >
		typename CharT DecodeCharAdvance( IN OUT const CharT** ppSrc ) const;

		template< typename CharT >
		typename CharT DecodeChar( OUT const CharT* pSrc, size_t* pLength = nullptr ) const;

		// iterator interface:
		template< typename IteratorT >
		typename IteratorT::value_type DecodeCharAdvance( IN OUT IteratorT* pIt ) const;

		template< typename IteratorT >
		typename IteratorT::value_type DecodeChar( IteratorT it, OUT size_t* pLength = nullptr ) const;


		// character encoding:
		template< typename OutIteratorT, typename CharT >
		void AppendEncodedChar( OUT OutIteratorT itLiteral, CharT actualCh, bool enquote, const char* pRetain = nullptr ) const;
	private:
		std::string FormatHexCh( char chr ) const;
		std::wstring FormatHexCh( wchar_t chr ) const;
	private:
		char m_escSeqLead;				// e.g. C/C++: '\'
		std::string m_escaped;			// e.g. C/C++: abfnrtv'"\?
		std::string m_actual;			// e.g. C/C++: '\a' '\b' '\f' '\n' '\r' '\t' '\v' '\'' '\"' '\\' '\?'
		std::string m_encLineContinue;	// e.g. C/C++: '\' in string literals, to continue string literal on next line
		std::string m_encRetain;		// e.g. C/C++: '\?' to avoid trigraphs - avoid encoding, since it's a marginal escape sequence
		char m_hexPrefix;				// e.g. C/C++: 'x'
		bool m_parseOctalLiteral;		// e.g. C/C++: true

		static const char s_unknownChr = '�';	// error placeholder character: upside-down question mark
	};
}


namespace cpp
{
	enum SepMatch { CharQuote, StringQuote, Comment, LineComment };		// TSepMatchPos indexes in GetParser().GetSeparators()
	enum NumLiteral { BadNumLiteral, DecimalLiteral, HexLiteral, OctalLiteral, BinaryLiteral, FloatingPointLiteral };


	template< typename SignedIntT, typename CharT >
	std::pair<SignedIntT, NumLiteral> ParseIntegerLiteral( const CharT* pSrc, OUT CharT** ppNumEnd )
	{
		ASSERT_PTR( pSrc );

		const CharT numPrefix[] = { ' ', '\t', '\r', '\n', '+', '-', '\0' };
		const CharT* pNum = str::SkipWhitespace( pSrc, numPrefix );			// skip whitespace and sign

		NumLiteral numLiteral = DecimalLiteral;

		if ( '0' == *pNum )
		{
			CharT nextCh = pNum[ 1 ];

			switch ( func::ToLower()( nextCh ) )
			{
				case 'b':
					return std::make_pair( num::StringToInteger<SignedIntT>( pNum + 2, ppNumEnd, 2 ), BinaryLiteral );		// skip non-parsable "0b" prefix
				case 'x':
					numLiteral = HexLiteral;
					break;
				case '\0':
					break;		// EOS
				default:
					if ( pred::IsDigit()( nextCh ) )
						numLiteral = OctalLiteral;
			}
		}

		SignedIntT integer = num::StringToInteger<SignedIntT>( pSrc, ppNumEnd, 0 );

		if ( *ppNumEnd == pSrc )
			numLiteral = BadNumLiteral;
		else if ( '.' == **ppNumEnd )		// decimal point in C locale?  => will parse for double
			numLiteral = FloatingPointLiteral;

		return std::make_pair( integer, numLiteral );
	}

	template< typename SignedIntT, typename CharT >
	std::pair<SignedIntT, NumLiteral> ParseIntegerLiteral( const CharT* pSrc, OUT size_t* pNumLength = nullptr )
	{
		CharT* pNumEnd;
		std::pair<SignedIntT, NumLiteral> integerPair = ParseIntegerLiteral<SignedIntT>( pSrc, &pNumEnd );

		utl::AssignPtr( pNumLength, static_cast<size_t>( pNumEnd - pSrc ) );
		return integerPair;
	}

	template< typename CharT >
	std::pair<double, NumLiteral> ParseDoubleLiteral( const CharT* pSrc, OUT size_t* pNumLength = nullptr )
	{
		ASSERT_PTR( pSrc );
		CharT* pNumEnd;
		double realNumber = num::StringToDouble( pSrc, &pNumEnd );

		utl::AssignPtr( pNumLength, static_cast<size_t>( pNumEnd - pSrc ) );
		return std::pair<double, NumLiteral>( realNumber, pNumEnd != pSrc ? FloatingPointLiteral : BadNumLiteral );
	}


	template< typename CharT >
	bool IsValidNumericLiteral( const CharT* pSrc, OUT size_t* pNumLength = nullptr )
	{
		CharT* pNumEnd;
		std::pair<long long, NumLiteral> integerPair = ParseIntegerLiteral<long long>( pSrc, &pNumEnd );

		if ( FloatingPointLiteral == integerPair.second )
		{
			double realNumber = num::StringToDouble( pSrc, &pNumEnd );
			UNUSED_ALWAYS( realNumber );
		}

		utl::AssignPtr( pNumLength, static_cast<size_t>( pNumEnd - pSrc ) );
		return pNumEnd != pSrc;
	}
}


#endif // Language_h
