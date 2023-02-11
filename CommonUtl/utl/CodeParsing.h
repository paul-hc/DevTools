#ifndef CodeParsing_h
#define CodeParsing_h
#pragma once

#include "StringParsing.h"
#include "Range.h"
#include <vector>


namespace code
{
	bool IsBrace( wchar_t brace );				// works for both char/wchar_t
	bool IsStartBrace( wchar_t brace );
	bool IsEndBrace( wchar_t brace );

	wchar_t _GetMatchingBrace( wchar_t brace );

	template< typename CharT >
	inline CharT ToMatchingBrace( CharT brace ) { return static_cast<CharT>( _GetMatchingBrace( brace ) ); }


	template< typename CharT >
	class CLanguage;

	template< typename CharT >
	const CLanguage<CharT>& GetCppLanguage( void );
}


namespace pred
{
	struct IsBrace : public BaseIsCharPred
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return code::IsBrace( chr ); }
	};

	struct IsStartBrace : public BaseIsCharPred
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return code::IsStartBrace( chr ); }
	};

	struct IsEndBrace : public BaseIsCharPred
	{
		template< typename CharT >
		bool operator()( CharT chr ) const { return code::IsEndBrace( chr ); }
	};


	struct ToMatchingBrace : public BaseIsCharPred
	{
		template< typename CharT >
		CharT operator()( CharT chr ) const { return code::ToMatchingBrace( chr ); }
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
		}

		const str::CEnclosedParser<CharT>& GetParser( void ) const { return m_parser; }


		// iterator search

		template< typename IteratorT >
		IteratorT FindNextSequence( IteratorT itCode, IteratorT itLast, const TString& sequence ) const
		{
			str::IterationDir iterDir = str::GetIterationDir( itCode );
			const TSeparatorsPair& sepsPair = m_parser.GetSeparators();

			for ( TSepMatchPos sepMatchPos = TString::npos; itCode != itLast; )
			{
				if ( sepsPair.MatchesSequenceAt( itCode, itLast, sequence ) )
				{
					if ( str::ReverseIter == iterDir )
						itCode += sequence.length() - 1;	// advance to beginning of the match

					return itCode;				// found the next brace
				}
				else if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, itCode, itLast ) )	// matches a quoted string, comment, etc?
				{
					sepsPair.SkipMatchingSpec( &itCode /*in-out*/, itLast, sepMatchPos );	// skip the entire spec
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
					sepsPair.SkipMatchingSpec( &itCode /*in-out*/, itLast, sepMatchPos );	// skip the entire spec
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
		bool SkipWhile( IteratorT* pItCode /*in-out*/, IteratorT itLast, IsCharPred isCharPred ) const
		{	// skips to the next position that does not satisfy the predicate
			ASSERT_PTR( pItCode );

			IteratorT itFound = FindNextCharThatNot( *pItCode, itLast, isCharPred );
			if ( itFound == itLast )
				return false;

			*pItCode = itFound;
			return true;					// true if found non-isCharPred
		}


		template< typename IteratorT >
		inline bool SkipWhitespace( IteratorT* pItCode /*in-out*/, IteratorT itLast ) const { return SkipWhile( pItCode, itLast, pred::IsSpace() ); }

		template< typename IteratorT >
		inline bool SkipIdentifier( IteratorT* pItCode /*in-out*/, IteratorT itLast ) const { return SkipWhile( pItCode, itLast, pred::IsIdentifier() ); }


		// brace lookup

		template< typename IteratorT >
		IteratorT FindMatchingBrace( IteratorT itBrace, IteratorT itLast, IteratorT* pItBraceMismatch = nullptr ) const
		{	// find the closing brace matching the current brace;  skip language-specific comments, quoted strings, etc
			REQUIRE( itBrace != itLast && IsBrace( *itBrace ) );

			utl::AssignPtr( pItBraceMismatch, itLast );				// i.e. no mismatch syntax error

			std::vector< std::pair<CharT, IteratorT> > braceStack;	// <original_brace, origin_iter>

			braceStack.push_back( std::make_pair( *itBrace, itBrace ) );
			IteratorT it = itBrace + 1;		// skip the opening brace

			CharT topMatchingBrace = ToMatchingBrace( braceStack.back().first );
			const TSeparatorsPair& sepsPair = m_parser.GetSeparators();

			for ( TSepMatchPos sepMatchPos = TString::npos; it != itLast; )
			{
				if ( IsBrace( *it ) )
				{
					if ( topMatchingBrace == *it )
					{
						braceStack.pop_back();			// exit one brace nesting level

						if ( braceStack.empty() )
							return it;					// on the matching brace of the originating brace
					}
					else
						braceStack.push_back( std::make_pair( *it, it ) );				// go deeper with the nested brace

					topMatchingBrace = ToMatchingBrace( braceStack.back().first );
				}
				else if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, it, itLast ) )	// matches a quoted string, comment, etc?
				{
					sepsPair.SkipMatchingSpec( &it /*in-out*/, itLast, sepMatchPos );	// skip the entire spec
					continue;
				}

				++it;
			}

			// matching brace not found
			ENSURE( !braceStack.empty() );
			TRACE( " (?) CLanguage::FindMatchingBrace() - syntax error in code: no matching brace found for the origin brace at:\n\tcode: '%s'\n",
				   str::ValueToString<std::string>( &*braceStack.front().second ).c_str() );

			utl::AssignPtr( pItBraceMismatch, braceStack.front().second );		// point to the originating brace mismatch (the cause of syntax error)
			return itLast;
		}

		template< typename IteratorT >
		bool SkipPastMatchingBrace( IteratorT* pItBrace /*in-out*/, IteratorT itLast, IteratorT* pItBraceMismatch = nullptr ) const
		{	// find past the closing brace matching the current brace - compatible with found range bounds
			ASSERT_PTR( pItBrace );
			*pItBrace = FindMatchingBrace( *pItBrace, itLast, pItBraceMismatch );
			if ( *pItBrace == itLast )
				return false;

			++*pItBrace;		// advance past the found matching brace (for range computation)
			return true;
		}

		template< typename IteratorT >
		IteratorT FindNextBrace( IteratorT itCode, IteratorT itLast ) const
		{
			return FindNextCharThat( itCode, itLast, &code::IsBrace );
		}
	private:
		const str::CEnclosedParser<CharT> m_parser;		// contains pairs of START/END separators of specific language constructs (for skipping)
	};
}


#endif // CodeParsing_h
