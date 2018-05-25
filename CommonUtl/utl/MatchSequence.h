#ifndef MatchSequence_h
#define MatchSequence_h
#pragma once

#include "StringCompare.h"


namespace str
{
	template< typename CharType >
	struct CMatchSequence
	{
		typedef std::basic_string< CharType > StringType;

		CMatchSequence( void ) : m_match( MatchEqual ) {}
		CMatchSequence( const StringType& srcText, const StringType& destText ) : m_textPair( srcText, destText ), m_match( MatchNotEqual ) {}

		template< typename MatchFunc >							// could use str::GetMatch(), path::GetMatch(), etc
		void Init( const StringType& srcText, const StringType& destText, MatchFunc getMatchFunc )
		{
			 m_textPair.first = srcText;
			 m_textPair.second = destText;
			 ComputeMatchSeq( getMatchFunc );
		}

		template< typename MatchFunc >							// could use str::GetMatch(), path::GetMatch(), etc
		void ComputeMatchSeq( MatchFunc getMatchFunc );
	public:
		std::pair< StringType, StringType > m_textPair;								// <SRC, DEST> text pair of subitems

		Match m_match;
		std::pair< std::vector< Match >, std::vector< Match > > m_matchSeqPair;		// <SRC, DEST> - same size with pair of subitems text
	};


	typedef CMatchSequence< TCHAR > TMatchSequence;


	// CMatchSequence template code

	template< typename CharType >
	template< typename MatchFunc >
	void CMatchSequence< CharType >::ComputeMatchSeq( MatchFunc getMatchFunc )
	{
		m_match = getMatchFunc( m_textPair.first.c_str(), m_textPair.second.c_str() );
		m_matchSeqPair.first.clear();
		m_matchSeqPair.second.clear();

		if ( m_textPair.second.empty() || MatchEqual == m_match )
			return;

		std::vector< lcs::CResult< CharType > > lcsSequence;
		lcs::CompareStringPair( lcsSequence, m_textPair, getMatchFunc );

		m_matchSeqPair.first.reserve( m_textPair.first.size() );
		m_matchSeqPair.second.reserve( m_textPair.second.size() );

		for ( std::vector< lcs::CResult< CharType > >::const_iterator itResult = lcsSequence.begin(); itResult != lcsSequence.end(); ++itResult )
			switch ( itResult->m_matchType )
			{
				case lcs::Equal:
					m_matchSeqPair.first.push_back( MatchEqual );
					m_matchSeqPair.second.push_back( MatchEqual );
					break;
				case lcs::EqualDiffCase:
					m_matchSeqPair.first.push_back( MatchEqualDiffCase );
					m_matchSeqPair.second.push_back( MatchEqualDiffCase );
					break;
				case lcs::Insert:
					m_matchSeqPair.second.push_back( MatchNotEqual );
					// discard SRC
					break;
				case lcs::Remove:
					m_matchSeqPair.first.push_back( MatchNotEqual );
					// discard DEST
					break;
			}

		ENSURE( m_textPair.first.size() == m_matchSeqPair.first.size() );
		ENSURE( m_textPair.second.size() == m_matchSeqPair.second.size() );
	}
}


#endif // MatchSequence_h
