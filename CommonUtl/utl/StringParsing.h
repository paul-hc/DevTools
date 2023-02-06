#ifndef StringParsing_h
#define StringParsing_h
#pragma once

#include "StringUtilities.h"


namespace code
{
	// FWD:
	template< typename StringT >
	size_t FindIdentifierEnd( const StringT& text, size_t identPos );
}


namespace str
{
	// A set of START and END separators to search for enclosed identifiers, in forward direction, by position.
	// Provides parsing a string for enclosed items delimited by any of the multiple START/END separator pairs (e.g. environment variables).
	//
	template< typename CharT >
	class CEnclosedParser : private utl::noncopyable
	{
	public:
		typedef std::basic_string<CharT> TString;
		typedef std::vector<TString> TSepVector;
		typedef size_t TSepMatchPos;						// position in TSepVector of the matching START and END separators
		typedef std::pair<size_t, size_t> TIdentSpecPair;	// START and END positions of a full identifier spec "<startSep>identifier<endSep>"

		CEnclosedParser( const CharT* pStartSepList, const CharT* pEndSepList, bool matchIdentifier = true, const CharT listDelim[] = nullptr /* "|" */ )
			: m_matchIdentifier( matchIdentifier )
		{
			InitSeparators( pStartSepList, pEndSepList, listDelim );
		}

		CEnclosedParser( bool matchIdentifier, const char* pStartSepList, const char* pEndSepList, const char listDelim[] = "|" )		// narrow seps constructor
			: m_matchIdentifier( matchIdentifier )
		{
			InitSeparators( str::ValueToString<TString>( pStartSepList ).c_str(), str::ValueToString<TString>( pEndSepList ).c_str(), listDelim );
		}

		bool IsValid( void ) const { return !m_sepsPair.first.empty(); }
		const std::pair<TSepVector, TSepVector>& GetSepsPair( void ) const { return m_sepsPair; }

		bool IsValidMatchPos( TSepMatchPos sepMatchPos ) const { return sepMatchPos < m_sepsPair.first.size(); }
		bool IsValidIdentifier( TSepMatchPos sepMatchPos, const TIdentSpecPair& specBounds ) const { return GetIdentLength( sepMatchPos, specBounds ) != 0; }
		const TString& GetStartSep( TSepMatchPos sepMatchPos ) const { REQUIRE( IsValidMatchPos( sepMatchPos ) ); return m_sepsPair.first[ sepMatchPos ]; }
		const TString& GetEndSep( TSepMatchPos sepMatchPos ) const { REQUIRE( IsValidMatchPos( sepMatchPos ) ); return m_sepsPair.second[ sepMatchPos ]; }

		TString MakeSpec( const TIdentSpecPair& specBounds, const TString& text ) const
		{
			REQUIRE( specBounds.first != TString::npos && specBounds.second >= specBounds.first );
			return text.substr( specBounds.first, specBounds.second - specBounds.first );
		}

		TString MakeIdentifier( TSepMatchPos sepMatchPos, const TIdentSpecPair& specBounds, const TString& text ) const
		{
			const size_t identLen = GetIdentLength( sepMatchPos, specBounds );
			return text.substr( specBounds.first + GetStartSep( sepMatchPos ).length(), identLen );
		}

		TString Make( TSepMatchPos sepMatchPos, const TIdentSpecPair& specBounds, const TString& text, bool keepSeps = true ) const
		{
			return keepSeps
				? MakeSpec( specBounds, text )							// "$(VAR1)"
				: MakeIdentifier( sepMatchPos, specBounds, text );		// "VAR1"
		}

		size_t GetIdentLength( TSepMatchPos sepMatchPos, const TIdentSpecPair& specBounds ) const
		{
			size_t sepLen = GetStartSep( sepMatchPos ).length() + GetEndSep( sepMatchPos ).length();

			REQUIRE( specBounds.first != TString::npos && specBounds.second != TString::npos && specBounds.second >= specBounds.first + sepLen );	// valid inner normalized range
			return specBounds.second - specBounds.first - sepLen;
		}

		TIdentSpecPair FindItem( TSepMatchPos* pOutSepMatchPos, const TString& text, size_t offset = 0 ) const
		{	// look for "<startSep>identifier<endSep>" beginning at offset
			ASSERT_PTR( pOutSepMatchPos );
			REQUIRE( IsValid() && offset <= text.length() );

			size_t startSepPos = FindAnyStartSep( pOutSepMatchPos, text, offset );
			size_t endPos = TString::npos;

			if ( startSepPos != utl::npos )
			{
				size_t identPos = startSepPos + GetStartSep( *pOutSepMatchPos ).length();
				const TString& endSep = GetEndSep( *pOutSepMatchPos );

				if ( m_matchIdentifier )
					endPos = code::FindIdentifierEnd( text, identPos );		// skip identifier (should not start with a digit)
				else
					endPos = text.find( endSep, identPos );						// find END separator

				if ( TString::npos == endPos || endPos == text.length() )		// not ended in endSep?
					startSepPos = utl::npos;
				else if ( str::EqualsAt( text, endPos, endSep ) )
					endPos += endSep.length();			// skip past endSep
				else
					return FindItem( pOutSepMatchPos, text, identPos );			// continue searching past startSep for next enclosed identifier
			}

			return std::make_pair( startSepPos, endPos );
		}

		void QueryItems( std::vector<TString>& rItems, const TString& text, bool keepSeps = true ) const
		{
			rItems.clear();

			TSepMatchPos sepMatchPos;

			for ( TIdentSpecPair specBounds( 0, 0 );
				  ( specBounds = FindItem( &sepMatchPos, text, specBounds.first ) ).first != TString::npos;
				  specBounds.first = specBounds.second )
				rItems.push_back( Make( sepMatchPos, specBounds, text, keepSeps ) );
		}

		size_t ReplaceSeparators( TString& rText, const CharT* pStartSep, const CharT* pEndSep, size_t maxCount = TString::npos ) const
		{
			REQUIRE( pStartSep != nullptr && pEndSep != nullptr );

			CSequence<CharT> startSeq( pStartSep ), endSeq( pEndSep );
			TSepMatchPos sepMatchPos;
			size_t itemCount = 0;

			for ( TIdentSpecPair specBounds( 0, 0 );
				  itemCount != maxCount && ( specBounds = FindItem( &sepMatchPos, rText, specBounds.first ) ).first != TString::npos;
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
		size_t FindAnyStartSep( TSepMatchPos* pOutSepMatchPos, const TString& text, size_t offset = 0 ) const
		{
			ASSERT_PTR( pOutSepMatchPos );

			for ( size_t startSepPos = offset; ( startSepPos = text.find_first_of( m_leadingChars, startSepPos ) ) != TString::npos; ++startSepPos )
			{
				TSepMatchPos sepMatchPos = FindMatchPos( text, startSepPos );

				if ( sepMatchPos != utl::npos )
				{
					utl::AssignPtr( pOutSepMatchPos, sepMatchPos );
					return startSepPos;
				}
			}

			return TString::npos;
		}

		TSepMatchPos FindMatchPos( const TString& text, size_t offset ) const
		{
			for ( TSepMatchPos i = 0; i != m_sepsPair.first.size(); ++i )
			{
				const TString& startSep = m_sepsPair.first[ i ];

				if ( str::EqualsAt( text, offset, startSep ) )
					return i;		// found matching separator position
			}

			return utl::npos;
		}

		void StoreLeadingChars( void )
		{
			for ( size_t i = 0; i != m_sepsPair.first.size(); ++i )
			{
				REQUIRE( !m_sepsPair.first[i].empty() && !m_sepsPair.second[i].empty() );		// START and END separators must be valid

				CharT leadingCh = m_sepsPair.first[ i ][ 0 ];

				if ( TString::npos == m_leadingChars.find( leadingCh ) )		// is it unique?
					m_leadingChars.push_back( leadingCh );
			}
		}

		void InitSeparators( const CharT* pStartSepList, const CharT* pEndSepList, const CharT listDelim[] )
		{
			const CharT defaultDelim[] = { '|', '\0' };		// "|"
			if ( nullptr == listDelim )
				listDelim = defaultDelim;

			str::Split( m_sepsPair.first, pStartSepList, listDelim );
			str::Split( m_sepsPair.second, pEndSepList, listDelim );
			StoreLeadingChars();

			ENSURE( !m_sepsPair.first.empty() && m_sepsPair.first.size() == m_sepsPair.second.size() );
			ENSURE( !m_leadingChars.empty() );
		}
	private:
		std::pair<TSepVector, TSepVector> m_sepsPair;	// <START seps, END seps>
		bool m_matchIdentifier;							// if true matches only identifiers "VAR1", otherwise matches anything "VAR.2"
		TString m_leadingChars;							// for quick matching of the first letter when searching for START separators
	};
}


namespace str
{
	// ex: query quoted sub-strings, or environment variables "lead_%VAR1%_mid_%VAR2%_trail" => { "VAR1", "VAR2" }

	template< typename CharT >
	void QueryEnclosedItems( std::vector< std::basic_string<CharT> >& rItems, const std::basic_string<CharT>& text,
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

		for ( str::const_iterator itStart = str::begin( pSource ), itEnd = str::end( pSource ); ; )
		{
			str::const_iterator itKeyStart = std::search( itStart, itEnd, sepStart.Begin(), sepStart.End() ), itKeyEnd = itEnd;
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
	size_t ReplaceEnvVar_VcMacroToWindows( StringT& rText, size_t varMaxCount = StringT::npos )
	{
		// replaces multiple occurences of e.g. "$(UTL_INCLUDE)" to "%UTL_INCLUDE%" - only for literals that resemble a C/C++ identifier
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
	template< typename CharT >
	inline void QueryEnclosedIdentifiers( std::vector< std::basic_string<CharT> >& rIdents, const std::basic_string<CharT>& text,
										  const CharT* pStartSeps, const CharT* pEndSeps, bool keepSeps = true )
	{
		str::QueryEnclosedItems( rIdents, text, pStartSeps, pEndSeps, keepSeps, true );
	}
}


#endif // StringParsing_h
