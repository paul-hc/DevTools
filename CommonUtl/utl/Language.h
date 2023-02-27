#ifndef Language_h
#define Language_h
#pragma once

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
	const CLanguage<CharT>& GetCppLang( void );
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

		CLanguage( const char* pStartSepList, const char* pEndSepList )		// constructor using only narrow strings for language constructs
			: m_parser( false, pStartSepList, pEndSepList )
		{
			GetSeparatorsPair().EnableLineComments();		// by default assume the last separator pair refers to single-line comments
		}

		const str::CEnclosedParser<CharT>& GetParser( void ) const { return m_parser; }
		const TSeparatorsPair& GetSeparatorsPair( void ) const { return m_parser.GetSeparators(); }


		// iterator search

		template< typename IteratorT >
		IteratorT FindNextSequence( IteratorT itCode, IteratorT itLast, const TString& sequence ) const
		{
			str::IterationDir iterDir = str::GetIterationDir( itCode );
			const TSeparatorsPair& sepsPair = m_parser.GetSeparators();

			for ( TSepMatchPos sepMatchPos = TString::npos; itCode != itLast; )
			{
				if ( str::EqualsSeq( itCode, itLast, sequence ) )
				{
					if ( str::ReverseIter == iterDir )
						itCode += sequence.length() - 1;	// advance to r-beginning of the match

					return itCode;				// found the next bracket
				}
				else if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, itCode, itLast ) )	// matches a quoted string, comment, etc?
				{
					sepsPair.SkipMatchingSpec( &itCode, itLast, sepMatchPos );	// skip the entire spec
					continue;
				}

				++itCode;
			}
			return itLast;
		}

		template< typename IteratorT, typename IsCharPred >
		IteratorT FindNextCharThat( IteratorT itCode, IteratorT itLast, IsCharPred isCharPred ) const
		{
			const TSeparatorsPair& sepsPair = m_parser.GetSeparators();

			for ( TSepMatchPos sepMatchPos = TString::npos; itCode != itLast; )
			{
				if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, itCode, itLast ) )			// matches a quoted string, comment, etc?
				{
					sepsPair.SkipMatchingSpec( &itCode, itLast, sepMatchPos );	// skip the entire spec
					continue;
				}
				else if ( isCharPred( *itCode ) )
					return itCode;				// found the next match

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
		bool SkipWhile( IteratorT* pItCode _in_out_, IteratorT itLast, IsCharPred isCharPred ) const
		{	// skips to the next position that does not satisfy the predicate
			ASSERT_PTR( pItCode );

			IteratorT itFound = FindNextCharThatNot( *pItCode, itLast, isCharPred );
			if ( itFound == *pItCode )
				return false;				// no advance

			*pItCode = itFound;
			return true;					// true if found non-isCharPred
		}

		template< typename IteratorT, typename IsCharPred >
		bool SkipUntil( IteratorT* pItCode _in_out_, IteratorT itLast, IsCharPred isCharPred ) const
		{	// skips to the next position that satisfies the predicate
			ASSERT_PTR( pItCode );

			IteratorT itFound = FindNextCharThat( *pItCode, itLast, isCharPred );
			if ( itFound == *pItCode )
				return false;				// no advance

			*pItCode = itFound;
			return true;					// true if found isCharPred
		}


		template< typename IteratorT >
		inline bool SkipWhitespace( IteratorT* pItCode _in_out_, IteratorT itLast ) const { return SkipWhile( pItCode, itLast, pred::IsSpace() ); }

		template< typename IteratorT >
		inline bool SkipLiteral( IteratorT* pItCode _in_out_, IteratorT itLast ) const { return SkipWhile( pItCode, itLast, pred::IsLiteral() ); }


		// bracket lookup

		template< typename IteratorT >
		IteratorT FindMatchingBracket( IteratorT itBracket, IteratorT itLast, IteratorT* pItBracketMismatch = nullptr _out_ ) const
		{	// find the closing bracket matching the current bracket;  skip language-specific comments, quoted strings, etc
			REQUIRE( itBracket != itLast && IsBracket( *itBracket ) );

			utl::AssignPtr( pItBracketMismatch, itLast );				// i.e. no mismatch syntax error

			std::vector< std::pair<CharT, IteratorT> > bracketStack;	// <original_bracket, origin_iter>

			bracketStack.push_back( std::make_pair( *itBracket, itBracket ) );
			IteratorT it = itBracket + 1;		// skip the opening bracket

			CharT topMatchingBracket = ToMatchingBracket( bracketStack.back().first );
			const TSeparatorsPair& sepsPair = m_parser.GetSeparators();

			for ( TSepMatchPos sepMatchPos = TString::npos; it != itLast; )
			{
				if ( IsBracket( *it ) )
				{
					if ( topMatchingBracket == *it )
					{
						bracketStack.pop_back();		// exit one bracket nesting level

						if ( bracketStack.empty() )
							return it;					// on the matching bracket of the originating bracket
					}
					else
						bracketStack.push_back( std::make_pair( *it, it ) );			// go deeper with the nested bracket

					topMatchingBracket = ToMatchingBracket( bracketStack.back().first );
				}
				else if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, it, itLast ) )	// matches a quoted string, comment, etc?
				{
					sepsPair.SkipMatchingSpec( &it, itLast, sepMatchPos );	// skip the entire spec
					continue;
				}

				++it;
			}

			// matching bracket not found
			ENSURE( !bracketStack.empty() );
			TRACE( " (?) CLanguage::FindMatchingBracket() - syntax error in code: no matching bracket found for the origin bracket at:\n\tcode: '%s'\n",
				   str::ValueToString<std::string>( &*bracketStack.front().second ).c_str() );

			utl::AssignPtr( pItBracketMismatch, bracketStack.front().second );		// point to the originating bracket mismatch (the cause of syntax error)
			return itLast;
		}

		template< typename IteratorT >
		bool SkipPastMatchingBracket( IteratorT* pItBracket _in_out_, IteratorT itLast, IteratorT* pItBracketMismatch = nullptr _out_ ) const
		{	// find past the closing bracket matching the current bracket - compatible with found range bounds
			ASSERT_PTR( pItBracket );
			*pItBracket = FindMatchingBracket( *pItBracket, itLast, pItBracketMismatch );
			if ( *pItBracket == itLast )
				return false;

			++*pItBracket;		// advance past the found matching bracket (for range computation)
			return true;
		}

		template< typename IteratorT >
		IteratorT FindNextBracket( IteratorT itCode, IteratorT itLast ) const
		{
			return FindNextCharThat( itCode, itLast, &code::IsBracket );
		}
	private:
		const str::CEnclosedParser<CharT> m_parser;		// contains pairs of START/END separators of specific language constructs (for skipping)
	};
}


#endif // Language_h
