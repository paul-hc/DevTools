#ifndef StringParsing_h
#define StringParsing_h
#pragma once

#include "StringCompare.h"
#include "Algorithms_fwd.h"
#include "Range.h"


namespace code
{
	// FWD:
	template< typename StringT >
	size_t FindIdentifierEnd( const StringT& text, size_t identPos );
}


namespace str
{
	template< typename IteratorT >
	inline bool IsReverseIter( const IteratorT& /*it*/ ) { return false; }

	template<> inline bool IsReverseIter( const std::string::const_reverse_iterator& /*it*/ ) { return true; }
	template<> inline bool IsReverseIter( const std::wstring::const_reverse_iterator& /*it*/ ) { return true; }


	enum IterationDir { ForwardIter, ReverseIter };		// iteration direction

	template< typename IteratorT >
	inline IterationDir GetIterationDir( const IteratorT& it ) { return IsReverseIter( it ) ? ReverseIter : ForwardIter; }


	template< typename IteratorT, typename StringT >
	static inline bool EqualsSeq( IteratorT itText, IteratorT itLast, const StringT& sequence )		// can compare wide and narrow either way, using implicit char conversion
	{
		if ( ReverseIter == GetIterationDir( itText ) )
			return utl::Equals( itText, itLast, sequence.rbegin(), sequence.rend() );		// comparison in reverse, starting backwards

		return utl::Equals( itText, itLast, sequence.begin(), sequence.end() );
	}

	template< typename StringT, typename SeqStringT >
	inline bool EqualsSeqAt( const StringT& text, size_t pos, const SeqStringT& sequence )			// can compare wide and narrow either way
	{
		REQUIRE( pos != StringT::npos );

		if ( pos + sequence.length() > text.length() )
			return false;		// overflow

		return str::EqualsSeq( text.begin() + pos, text.begin() + pos + sequence.length(), sequence );
	}
}


namespace str
{
	// a set of sequences to be matched when parsing a string (narrow/wide), typically defined for char
	//
	template< typename CharT >
	class CSequenceSet : private utl::noncopyable
	{
	public:
		typedef std::basic_string<CharT> TString;
		typedef size_t TSeqMatchPos;						// position in m_sequences of the matching sequence

		CSequenceSet( const CharT* pSeqList, const CharT listDelim[] = nullptr /* "|" */ )
		{
			const CharT defaultDelim[] = { '|', '\0' };		// "|"
			if ( nullptr == listDelim )
				listDelim = defaultDelim;

			str::Split( m_sequences, pSeqList, listDelim );
			ENSURE( !m_sequences.empty() );
		}

		const std::vector<TString>& GetSequences( void ) const { return m_sequences; }

		bool IsValidMatchPos( TSeqMatchPos seqMatchPos ) const { return seqMatchPos < m_sequences.size(); }
		size_t GetSequenceLength( TSeqMatchPos seqMatchPos ) const { REQUIRE( IsValidMatchPos( seqMatchPos ) ); return m_sequences[ seqMatchPos ].length(); }


		template< typename IteratorT >
		bool MatchesAnySequence( OUT TSeqMatchPos* pOutSeqMatchPos, IteratorT itText, IteratorT itLast ) const
		{
			REQUIRE( itText != itLast );
			for ( TSeqMatchPos i = 0; i != m_sequences.size(); ++i )
			{
				const TString& seq = m_sequences[ i ];
				if ( str::EqualsSeq( itText, itLast, seq ) )
				{
					utl::AssignPtr( pOutSeqMatchPos, i );
					return true;		// found matching seqarator position
				}
			}
			utl::AssignPtr( pOutSeqMatchPos, utl::npos );
			return false;
		}

		template< typename StringT >
		inline bool MatchesAnySequenceAt( OUT TSeqMatchPos* pOutSeqMatchPos, const StringT& text, size_t pos ) const
		{
			REQUIRE( pos < text.length() );
			return MatchesAnySequence( pOutSeqMatchPos, text.begin() + pos, text.end() );
		}


		template< typename IteratorT >
		bool SkipAnyMatchingSequence( IN OUT IteratorT* pItText, IteratorT itLast, OUT TSeqMatchPos* pSeqMatchPos = nullptr ) const
		{
			ASSERT_PTR( pItText );
			TSeqMatchPos seqMatchPos = utl::npos;

			if ( MatchesAnySequence( &seqMatchPos, *pItText, itLast ) )
				*pItText += m_sequences[ seqMatchPos ].length();

			utl::AssignPtr( pSeqMatchPos, seqMatchPos );
			return seqMatchPos != utl::npos;
		}

		template< typename StringT >
		bool SkipAnyMatchingSequenceAt( IN OUT size_t* pPos, const StringT& text, OUT TSeqMatchPos* pSeqMatchPos = nullptr ) const
		{
			ASSERT_PTR( pPos );
			ASSERT( *pPos < text.length() );
			TSeqMatchPos seqMatchPos = utl::npos;

			if ( MatchesAnySequenceAt( &seqMatchPos, text, *pPos ) )
				*pPos += GetSequenceLength( seqMatchPos );

			utl::AssignPtr( pSeqMatchPos, seqMatchPos );

			ENSURE( *pPos <= text.length() );
			return seqMatchPos != utl::npos;
		}
	private:
		std::vector<TString> m_sequences;
	};
}


namespace str
{
	// A set of START and END separators to search for enclosed sequences.
	// Note: single-line comments is defined (usually for languages), it must be the last separator in the list! That is to enable matching of line comments with no ending "\n" in the code text.
	//
	template< typename CharT >
	class CSeparatorsPair : private utl::noncopyable
	{
	public:
		typedef std::basic_string<CharT> TString;
		typedef std::vector<TString> TSepVector;
		typedef size_t TSepMatchPos;						// position in TSepVector of the matching START and END separators

		CSeparatorsPair( const CharT* pStartSepList, const CharT* pEndSepList, const CharT listDelim[] = nullptr /* "|" */ )
			: m_lineCommentSepPos( utl::npos )
		{
			const CharT defaultDelim[] = { '|', '\0' };		// "|"
			if ( nullptr == listDelim )
				listDelim = defaultDelim;

			str::Split( m_sepsPair.first, pStartSepList, listDelim );
			str::Split( m_sepsPair.second, pEndSepList, listDelim );
			StoreLeadingChars();

			ENSURE( !m_sepsPair.first.empty() && m_sepsPair.first.size() == m_sepsPair.second.size() );
			ENSURE( !m_leadStartChars.empty() );
		}

		bool IsValid( void ) const { return !m_sepsPair.first.empty(); }
		const std::pair<TSepVector, TSepVector>& GetSepsPair( void ) const { return m_sepsPair; }

		bool IsValidMatchPos( TSepMatchPos sepMatchPos ) const { return sepMatchPos < m_sepsPair.first.size(); }

		bool IsLineCommentMatchPos( TSepMatchPos sepMatchPos ) const { REQUIRE( IsValidMatchPos( sepMatchPos ) ); return m_lineCommentSepPos == sepMatchPos; }
		void EnableLineComments( bool enable = true ) const { m_lineCommentSepPos = enable ? ( m_sepsPair.first.size() - 1 ) : utl::npos; }


		// FORWARD direction:

		size_t FindAnyStartSep( OUT TSepMatchPos* pOutSepMatchPos, const TString& text, size_t offset = 0 ) const
		{
			ASSERT_PTR( pOutSepMatchPos );

			for ( size_t startSepPos = offset; ( startSepPos = text.find_first_of( m_leadStartChars, startSepPos ) ) != TString::npos; ++startSepPos )
			{
				TSepMatchPos sepMatchPos = FindSepMatchPos( text, startSepPos, m_sepsPair.first );

				if ( sepMatchPos != utl::npos )
				{
					*pOutSepMatchPos = sepMatchPos;
					return startSepPos;
				}
			}

			*pOutSepMatchPos = utl::npos;
			return TString::npos;
		}

		bool MatchesAnyStartSepAt( OUT TSepMatchPos* pOutSepMatchPos, const TString& text, size_t offset ) const	// use this in outside loops
		{
			REQUIRE( offset != text.length() );

			if ( m_leadStartChars.find( text[ offset ] ) != TString::npos )		// quick match of any separator leading character?
			{
				TSepMatchPos sepMatchPos = FindSepMatchPos( text, offset, m_sepsPair.first );

				if ( sepMatchPos != utl::npos )
				{
					utl::AssignPtr( pOutSepMatchPos, sepMatchPos );
					return true;
				}
			}

			return false;
		}


		// FORWARD/REVERSE direction using iterators:

		const TSepVector& GetOpenSeparators( IterationDir iterDir ) const { return ForwardIter == iterDir ? m_sepsPair.first : m_sepsPair.second; }
		const TSepVector& GetCloseSeparators( IterationDir iterDir ) const { return ForwardIter == iterDir ? m_sepsPair.second : m_sepsPair.first; }

		const TString& GetMatchingOpenSep( TSepMatchPos sepMatchPos, IterationDir iterDir ) const { REQUIRE( IsValidMatchPos( sepMatchPos ) ); return GetOpenSeparators( iterDir )[ sepMatchPos ]; }
		const TString& GetMatchingCloseSep( TSepMatchPos sepMatchPos, IterationDir iterDir ) const { REQUIRE( IsValidMatchPos( sepMatchPos ) ); return GetCloseSeparators( iterDir )[ sepMatchPos ]; }

		template< typename IteratorT >
		bool MatchesAnyOpenSepAt( OUT TSepMatchPos* pOutSepMatchPos, IteratorT itText, IteratorT itLast ) const
		{
			return MatchesAnySepAt( pOutSepMatchPos, itText, itLast, GetOpenSeparators( GetIterationDir( itText ) ) );
		}

		template< typename IteratorT >
		bool MatchesAnyCloseSepAt( TSepMatchPos sepMatchPos, IteratorT itText, IteratorT itLast ) const
		{
			REQUIRE( IsValidMatchPos( sepMatchPos ) );

			const TString& sep = GetMatchingCloseSep( sepMatchPos, GetIterationDir( itText ) );

			return str::EqualsSeq( itText, itLast, sep );
		}


		// parsing methods:

		template< typename IteratorT >
		bool SkipMatchingSpec( IN OUT IteratorT* pItText, IteratorT itLast, TSepMatchPos sepMatchPos ) const
		{
			// skip the code sequence "<openSep>item<closeSep>"
			ASSERT_PTR( pItText );
			IterationDir iterDir = GetIterationDir( *pItText );

			REQUIRE( *pItText != itLast );
			REQUIRE( str::EqualsSeq( *pItText, itLast, GetMatchingOpenSep( sepMatchPos, iterDir ) ) );		// expected to be on the OPEN separator

			for ( IteratorT it = *pItText + GetMatchingOpenSep( sepMatchPos, iterDir ).length();			// skip the open sep
				  it != itLast; ++it )
				if ( MatchesAnyCloseSepAt( sepMatchPos, it, itLast ) )
				{
					*pItText = it + GetMatchingCloseSep( sepMatchPos, iterDir ).length();
					return true;			// we have a spec match
				}

			if ( ForwardIter == iterDir )
				if ( IsLineCommentMatchPos( sepMatchPos ) )		// assume is closing of a single-line comment?  (single-line comments separator must be the last!)
				{	// allow single-line comments without the terminating "\n" => so skip to itLast as end-of-comment
					*pItText = itLast;
					return true;
						// for reverse iteration, single-line comments have to be ended in "\n"
				}

			return false;
		}
	private:
		static TSepMatchPos FindSepMatchPos( const TString& text, size_t offset, const TSepVector& separators )
		{	// by position
			for ( TSepMatchPos i = 0; i != separators.size(); ++i )
			{
				const TString& startSep = separators[ i ];

				if ( str::EqualsAt( text, offset, startSep ) )
					return i;		// found matching separator position
			}

			return utl::npos;
		}

		template< typename IteratorT >
		static TSepMatchPos FindSepMatchPos( IteratorT itText, IteratorT itLast, const TSepVector& separators )
		{
			for ( TSepMatchPos i = 0; i != separators.size(); ++i )
			{
				const TString& sep = separators[ i ];

				if ( str::EqualsSeq( itText, itLast, sep ) )
					return i;		// found matching separator position
			}

			return utl::npos;
		}

		template< typename IteratorT >
		bool MatchesAnySepAt( OUT TSepMatchPos* pOutSepMatchPos, IteratorT itText, IteratorT itLast, const TSepVector& separators ) const
		{
			REQUIRE( itText != itLast );

			TSepMatchPos sepMatchPos = FindSepMatchPos( itText, itLast, separators );

			if ( sepMatchPos != utl::npos )
			{
				utl::AssignPtr( pOutSepMatchPos, sepMatchPos );
				return true;
			}

			return false;
		}

		void StoreLeadingChars( void )
		{
			for ( size_t i = 0; i != m_sepsPair.first.size(); ++i )
			{
				REQUIRE( !m_sepsPair.first[i].empty() );		// START and END separators must be valid

				CharT leadingStart = utl::Front( m_sepsPair.first[ i ] );

				if ( TString::npos == m_leadStartChars.find( leadingStart ) )	// unique?
					m_leadStartChars.push_back( leadingStart );
			}
		}
	private:
		std::pair<TSepVector, TSepVector> m_sepsPair;	// <START seps, END seps>
		TString m_leadStartChars;						// for quick matching of the first letter when searching for START separators (by position only)
		mutable TSepMatchPos m_lineCommentSepPos;		// separator pair position for single-line comments (by default the last)
	};
}


namespace str
{
	// Provides parsing a string for enclosed items delimited by any of the multiple START/END separator pairs (e.g. environment variables).
	// For parsing in forward direction, by position.
	//
	template< typename CharT >
	class CEnclosedParser : private utl::noncopyable
	{
	public:
		typedef std::basic_string<CharT> TString;
		typedef CSeparatorsPair<CharT> TSeparatorsPair;
		typedef typename TSeparatorsPair::TSepMatchPos TSepMatchPos;	// position in TSepVector of the matching START and END separators
		typedef std::pair<size_t, size_t> TSpecPair;					// START and END positions of a spec: full enclosed item "<startSep>item<endSep>"

		CEnclosedParser( const CharT* pStartSepList, const CharT* pEndSepList, bool matchIdentifier = true, const CharT listDelim[] = nullptr /* "|" */ )
			: m_matchIdentifier( matchIdentifier )
			, m_seps( pStartSepList, pEndSepList, listDelim )
		{
		}

		CEnclosedParser( bool matchIdentifier, const char* pStartSepList, const char* pEndSepList )
			: m_matchIdentifier( matchIdentifier )
			, m_seps( str::ValueToString<TString>( pStartSepList ).c_str(), str::ValueToString<TString>( pEndSepList ).c_str() )
		{	// narrow seps constructor, works for both char/wchar_t parsers
		}

		bool IsValid( void ) const { return m_seps.IsValid(); }
		const TSeparatorsPair& GetSeparators( void ) const { return m_seps; }

		bool IsValidMatchPos( TSepMatchPos sepMatchPos ) const { return sepMatchPos < m_seps.GetSepsPair().first.size(); }
		bool IsValidIdentifier( TSepMatchPos sepMatchPos, const TSpecPair& specBounds ) const { return GetItemLength( sepMatchPos, specBounds ) != 0; }
		const TString& GetStartSep( TSepMatchPos sepMatchPos ) const { REQUIRE( IsValidMatchPos( sepMatchPos ) ); return m_seps.GetSepsPair().first[ sepMatchPos ]; }
		const TString& GetEndSep( TSepMatchPos sepMatchPos ) const { REQUIRE( IsValidMatchPos( sepMatchPos ) ); return m_seps.GetSepsPair().second[ sepMatchPos ]; }

		TString MakeSpec( const TSpecPair& specBounds, const TString& text ) const
		{
			REQUIRE( specBounds.first != TString::npos && specBounds.second >= specBounds.first );
			return text.substr( specBounds.first, specBounds.second - specBounds.first );
		}

		TString MakeItem( const Range<size_t>& itemRange, const TString& text ) const
		{
			return text.substr( itemRange.m_start, itemRange.GetSpan<size_t>() );
		}

		TString ExtractItem( TSepMatchPos sepMatchPos, const TSpecPair& specBounds, const TString& text ) const		// item from spec-bounds
		{
			return MakeItem( GetItemRange( sepMatchPos, specBounds, &text ), text );
		}

		TString Make( TSepMatchPos sepMatchPos, const TSpecPair& specBounds, const TString& text, bool keepSeps = true ) const
		{
			return keepSeps
				? MakeSpec( specBounds, text )						// "$(VAR1)"
				: ExtractItem( sepMatchPos, specBounds, text );		// "VAR1"
		}

		Range<size_t> GetItemRange( TSepMatchPos sepMatchPos, const TSpecPair& specBounds, const TString* pText = nullptr ) const
		{
			Range<size_t> itemRange( specBounds.first, specBounds.second );

			itemRange.m_start += GetStartSep( sepMatchPos ).length();
			itemRange.m_end -= GetEndSep( sepMatchPos ).length();

			if ( pText != nullptr && m_seps.IsLineCommentMatchPos( sepMatchPos ) )
			{
				const TString& endSep = GetEndSep( sepMatchPos );
				if ( !str::EqualsSeqAt( *pText, itemRange.m_end, endSep ) )	// no trailing "\n"?
					itemRange.m_end += endSep.length();						// un-shrink right
			}

			if ( itemRange.m_end < itemRange.m_start )
				itemRange.m_end = itemRange.m_start;		// clamp to empty range; could happen for empty line comments "//"

			REQUIRE( itemRange.IsNormalized() );
			return itemRange;
		}

		inline size_t GetItemLength( TSepMatchPos sepMatchPos, const TSpecPair& specBounds ) const
		{
			return GetItemRange( sepMatchPos, specBounds ).GetSpan<size_t>();
		}

		TSpecPair FindItemSpec( OUT TSepMatchPos* pOutSepMatchPos, const TString& text, size_t offset = 0 ) const
		{	// look for "<startSep>item<endSep>" beginning at offset
			ASSERT_PTR( pOutSepMatchPos );
			REQUIRE( IsValid() && offset <= text.length() );

			size_t startSepPos = m_seps.FindAnyStartSep( pOutSepMatchPos, text, offset );
			size_t endPos = TString::npos;

			if ( startSepPos != utl::npos )
			{
				size_t itemPos = startSepPos + GetStartSep( *pOutSepMatchPos ).length();
				const TString& endSep = GetEndSep( *pOutSepMatchPos );

				if ( m_matchIdentifier )
					endPos = code::FindIdentifierEnd( text, itemPos );			// skip identifier (should not start with a digit)
				else
					endPos = text.find( endSep, itemPos );						// find END separator

				if ( TString::npos == endPos || endPos == text.length() )		// not ended in endSep?
				{
					if ( m_seps.IsLineCommentMatchPos( *pOutSepMatchPos ) )		// single-line comment without a terminating "\n"?
						endPos = text.length();									// valid match of a SL-comment at the end of code text
					else
						startSepPos = utl::npos;
				}
				else if ( str::EqualsAt( text, endPos, endSep ) )
					endPos += endSep.length();			// skip past endSep
				else
					return FindItemSpec( pOutSepMatchPos, text, itemPos );		// continue searching past startSep for next enclosed item
			}

			return std::make_pair( startSepPos, endPos );
		}

		void QueryItems( OUT std::vector<TString>& rItems, const TString& text, bool keepSeps = true ) const
		{
			rItems.clear();

			TSepMatchPos sepMatchPos;

			for ( TSpecPair specBounds( 0, 0 );
				  ( specBounds = FindItemSpec( &sepMatchPos, text, specBounds.first ) ).first != TString::npos;
				  specBounds.first = specBounds.second )
				rItems.push_back( Make( sepMatchPos, specBounds, text, keepSeps ) );
		}

		size_t ReplaceSeparators( OUT TString& rText, const CharT* pStartSep, const CharT* pEndSep, size_t maxCount = TString::npos ) const
		{
			REQUIRE( pStartSep != nullptr && pEndSep != nullptr );

			CSequence<CharT> startSeq( pStartSep ), endSeq( pEndSep );
			TSepMatchPos sepMatchPos;
			size_t itemCount = 0;

			for ( TSpecPair specBounds( 0, 0 );
				  itemCount != maxCount && ( specBounds = FindItemSpec( &sepMatchPos, rText, specBounds.first ) ).first != TString::npos;
				  ++itemCount, specBounds.first = specBounds.second )
			{
				const TString& startSep = GetStartSep( sepMatchPos );
				const TString& endSep = GetEndSep( sepMatchPos );

				rText.replace( specBounds.first, startSep.length(), startSeq.m_pSeq, startSeq.m_length );
				specBounds.second -= startSep.length() - startSeq.m_length;

				rText.replace( specBounds.second - endSep.length(), endSep.length(), endSeq.m_pSeq, endSeq.m_length );
				specBounds.second -= endSep.length() - endSeq.m_length;
			}

			return itemCount;
		}
	private:
		bool m_matchIdentifier;			// if true matches only identifiers "VAR1", otherwise matches anything "VAR.2"
		TSeparatorsPair m_seps;			// <START seps, END seps>
	};
}


namespace str
{
	// ex: query quoted sub-strings, or environment variables "lead_%VAR1%_mid_%VAR2%_trail" => { "VAR1", "VAR2" }

	template< typename CharT >
	void QueryEnclosedItems( OUT std::vector< std::basic_string<CharT> >& rItems, const std::basic_string<CharT>& text,
							 const CharT* pStartSeps, const CharT* pEndSeps, bool keepSeps = true, bool matchIdentifier = true )
	{
		str::CEnclosedParser<CharT> parser( pStartSeps, pEndSeps, matchIdentifier );
		parser.QueryItems( rItems, text, keepSeps );
	}


	// expand enclosed tags using KeyToValueFunc

	template< typename CharT, typename KeyToValueFunc >
	std::basic_string<CharT> ExpandKeysToValues( const CharT* pSource, const CharT* pStartSep, const CharT* pEndSep,
												 KeyToValueFunc func, bool keepSeps = false )
	{
		ASSERT_PTR( pSource );
		ASSERT( !str::IsEmpty( pStartSep ) );

		const str::CSequence<CharT> sepStart( pStartSep );
		const str::CSequence<CharT> sepEnd( str::IsEmpty( pEndSep ) ? pStartSep : pEndSep );

		std::basic_string<CharT> output; output.reserve( str::GetLength( pSource ) * 2 );
		typedef const CharT* TConstIterator;

		for ( TConstIterator itStart = str::begin( pSource ), itEnd = str::end( pSource ); ; )
		{
			TConstIterator itKeyStart = std::search( itStart, itEnd, sepStart.Begin(), sepStart.End() ), itKeyEnd = itEnd;
			if ( itKeyStart != itEnd )
				itKeyEnd = std::search( itKeyStart + sepStart.m_length, itEnd, sepEnd.Begin(), sepEnd.End() );

			if ( itKeyStart != itEnd && itKeyEnd != itEnd )
			{
				output += std::basic_string<CharT>( itStart, std::distance( itStart, itKeyStart ) );		// add leading text
				output += func( keepSeps
								? std::basic_string<CharT>( itKeyStart, itKeyEnd + sepEnd.m_length )
								: std::basic_string<CharT>( itKeyStart + sepStart.m_length, itKeyEnd ) );
			}
			else
				output += std::basic_string<CharT>( itStart, std::distance( itStart, itKeyEnd ) );

			itStart = itKeyEnd + sepEnd.m_length;
			if ( itStart >= itEnd )
				break;
		}
		return output;
	}
}


namespace env
{
	// environment variables parsing

	template< typename StringT >
	size_t ReplaceEnvVar_VcMacroToWindows( IN OUT StringT& rText, size_t varMaxCount = StringT::npos )
	{
		// replaces multiple occurences of e.g. "$(UTL_INCLUDE)" to "%UTL_INCLUDE%" - only for identifiers that resemble a C/C++ identifier
		typedef typename StringT::value_type TChar;

		const TChar startSep[] = { '$', '(', '\0' };		// "$("
		const TChar endSep[] = { ')', '\0' };				// ")"
		const TChar winVarSep[] = { '%', '\0' };			// "%"

		str::CEnclosedParser<TChar> parser( startSep, endSep, true );
		return parser.ReplaceSeparators( rText, winVarSep, winVarSep, varMaxCount );
	}

	template< typename CharT >
	std::basic_string<CharT> GetReplaceEnvVar_VcMacroToWindows( const CharT* pSource, size_t varMaxCount = utl::npos )
	{
		std::basic_string<CharT> text( pSource );
		ReplaceEnvVar_VcMacroToWindows( text, varMaxCount );
		return text;
	}
}


namespace code
{
	template< typename StringT >
	size_t FindIdentifierEnd( const StringT& text, size_t identPos )
	{	// identifier should not start with a digit
		size_t endPos = identPos, length = text.length();
		pred::IsIdentifier isIdentifierPred;

		if ( endPos != length && pred::IsIdentifierLead()( text[ endPos ] ) )	// not starting with a digit?
		{	// skip first char, check the inner identifier criteria
			++endPos;

			while ( endPos != length && isIdentifierPred( text[ endPos ] ) )
				++endPos;
		}

		return endPos;
	}

	template< typename StringT >
	inline StringT ExtractIdentifier( const StringT& text, size_t identPos )
	{
		REQUIRE( pred::IsIdentifierLead()( text[ identPos ] ) );
		return text.substr( identPos, FindIdentifierEnd( text, identPos ) - identPos );
	}


	template< typename CharT >
	inline void QueryEnclosedIdentifiers( OUT std::vector< std::basic_string<CharT> >& rIdentifiers, const std::basic_string<CharT>& text,
										  const CharT* pStartSeps, const CharT* pEndSeps, bool keepSeps = true )
	{
		str::QueryEnclosedItems( rIdentifiers, text, pStartSeps, pEndSeps, keepSeps, true /*matchIdentifier*/ );
	}
}


#endif // StringParsing_h
