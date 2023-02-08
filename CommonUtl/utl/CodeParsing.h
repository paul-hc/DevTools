#ifndef CodeParsing_h
#define CodeParsing_h
#pragma once

#include "StringParsing.h"
#include <vector>


namespace code
{
	bool IsBrace( wchar_t brace );				// works for both char/wchar_t
	bool IsOpenBrace( wchar_t brace );
	bool IsCloseBrace( wchar_t brace );

	wchar_t _GetMatchingBrace( wchar_t brace );

	template< typename CharT >
	inline CharT GetMatchingBrace( CharT brace ) { return static_cast<CharT>( _GetMatchingBrace( brace ) ); }


	template< typename CharT >
	class CLanguage;

	template< typename CharT >
	const CLanguage<CharT>& GetCppLanguage( void );
}


namespace pred
{
	struct IsChar
	{
		IsChar( wchar_t chr ) : m_chr( chr ) {}

		template< typename CharT >
		bool operator()( CharT chr ) const { return m_chr == chr; }
	private:
		wchar_t m_chr;		// wchar_t is also compatible with char
	};
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


		// brace lookup methods

		template< typename IteratorT >
		IteratorT SkipBrace( IteratorT itBrace, IteratorT itLast, str::IterationDir iterDir = str::ForwardIter, IteratorT* pItBraceMismatch = nullptr ) const
		{	// find the closing brace matching the current brace;  skip language-specific comments, quoted strings, etc
			REQUIRE( itBrace != itLast );
			REQUIRE( IsBrace( *itBrace ) );
			utl::AssignPtr( pItBraceMismatch, itLast );				// i.e. no mismatch syntax error

			std::vector< std::pair<CharT, IteratorT> > braceStack;	// <original_brace, origin_iter>

			braceStack.push_back( std::make_pair( *itBrace, itBrace ) );
			IteratorT it = itBrace + 1;		// skip the opening brace

			CharT topMatchingBrace = GetMatchingBrace( braceStack.back().first );
			const TSeparatorsPair& sepsPair = m_parser.GetSeparators();

			for ( TSepMatchPos sepMatchPos = TString::npos; it != itLast; )
			{
				if ( IsBrace( *it ) )
				{
					if ( topMatchingBrace == *it )
					{
						braceStack.pop_back();		// exit one brace nesting level

						if ( braceStack.empty() )
							return it;				// found the matching brace of the originating brace
					}
					else
						braceStack.push_back( std::make_pair( *it, it ) );						// go deeper with the nested brace

					topMatchingBrace = GetMatchingBrace( braceStack.back().first );
				}
				else if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, it, itLast, iterDir ) )	// matches a quoted string, comment, etc?
				{
					sepsPair.SkipMatchingSpec( it /*in-out*/, itLast, sepMatchPos, iterDir );	// skip the entire spec
					continue;
				}

				++it;
			}

			// matching brace not found
			ENSURE( !braceStack.empty() );
			TRACE( " (?) CLanguage::SkipBrace() - syntax error in code: no matching brace found for the origin brace at:\n\tcode: '%s'\n",
				   str::ValueToString<std::string>( &*braceStack.front().second ).c_str() );

			utl::AssignPtr( pItBraceMismatch, braceStack.front().second );		// point to the originating brace mismatch (the cause of syntax error)
			return itLast;
		}

		template< typename IteratorT >
		IteratorT FindNextBrace( IteratorT itCode, IteratorT itLast, str::IterationDir iterDir = str::ForwardIter ) const
		{
			return FindNextCharThat( itCode, itLast, &code::IsBrace, iterDir );
		}

		template< typename IteratorT, typename IsCharPred >
		IteratorT FindNextCharThat( IteratorT itCode, IteratorT itLast, IsCharPred isCharPred, str::IterationDir iterDir = str::ForwardIter ) const
		{
			REQUIRE( itCode != itLast );

			const TSeparatorsPair& sepsPair = m_parser.GetSeparators();

			for ( TSepMatchPos sepMatchPos = TString::npos; itCode != itLast; )
			{
				if ( isCharPred( *itCode ) )
					return itCode;				// found the next brace
				else if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, itCode, itLast, iterDir ) )	// matches a quoted string, comment, etc?
				{
					sepsPair.SkipMatchingSpec( itCode /*in-out*/, itLast, sepMatchPos, iterDir );	// skip the entire spec
					continue;
				}

				++itCode;
			}
			return itLast;
		}
	private:
		const str::CEnclosedParser<CharT> m_parser;		// contains pairs of START/END separators of specific language constructs (for skipping)
	};
}


#endif // CodeParsing_h
