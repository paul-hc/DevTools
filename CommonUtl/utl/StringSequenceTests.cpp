
#include "stdafx.h"
#include "StringSequenceTests.h"
#include "LongestCommonSubsequence.h"
#include "LongestCommonDuplicate.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


namespace ut
{
	struct MatchTriplet
	{
		MatchTriplet( str::Match match = str::MatchNotEqual ) : m_match( match ) {}
		MatchTriplet( str::Match match, const char* pSrc, const char* pDest ) : m_match( match ), m_src( pSrc ), m_dest( pDest ) {}

		bool operator==( const MatchTriplet& right ) const
		{
			return
				m_match == right.m_match &&
				m_src == right.m_src &&
				m_dest == right.m_dest;
		}
	public:
		str::Match m_match;
		std::string m_src;
		std::string m_dest;
	};


	template<>
	std::tstring ToString< MatchTriplet >( const MatchTriplet& triplet )
	{
		std::tostringstream os;
		switch ( triplet.m_match )
		{
			case str::MatchEqual: os << "EQ"; break;
			case str::MatchEqualDiffCase: os << "~EQ"; break;
			case str::MatchNotEqual: os << "DIFF"; break;
		}

		os
			<< ':'
			<< " S={" << triplet.m_src << '}'
			<< " D={" << triplet.m_dest << '}';

		return os.str();
	}

	str::Match ToStringMatch( lcs::MatchType matchType )
	{
		switch ( matchType )
		{
			case lcs::Equal:
				return str::MatchEqual;
			case lcs::EqualDiffCase:
				return str::MatchEqualDiffCase;
			default:
				ASSERT( false );
			case lcs::Insert:
			case lcs::Remove:
				return str::MatchNotEqual;
		}
	}

	void PushTriplet( std::vector< ut::MatchTriplet >& rTriplets, const lcs::CResult< char >& lcsResult )
	{
		if ( rTriplets.empty() || rTriplets.back().m_match != ToStringMatch( lcsResult.m_matchType ) )
			rTriplets.push_back( MatchTriplet( ToStringMatch( lcsResult.m_matchType ) ) );

		ut::MatchTriplet& rLast = rTriplets.back();

		switch ( lcsResult.m_matchType )
		{
			case lcs::Equal:
				rLast.m_src += lcsResult.m_srcValue;
				rLast.m_dest += lcsResult.m_destValue;
				break;
			case lcs::EqualDiffCase:
				rLast.m_src += lcsResult.m_srcValue;
				rLast.m_dest += lcsResult.m_destValue;
				break;
			case lcs::Insert:
				rLast.m_dest += lcsResult.m_destValue;
				// discard SRC
				break;
			case lcs::Remove:
				rLast.m_src += lcsResult.m_srcValue;
				// discard DEST
				break;
		}
	}
}


CStringSequenceTests::CStringSequenceTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CStringSequenceTests& CStringSequenceTests::Instance( void )
{
	static CStringSequenceTests testCase;
	return testCase;
}

void CStringSequenceTests::TestCompareSimpleLCS( void )
{
	static const std::string src = "what ABC around", dest = "what XY around";

	lcs::Comparator< char, str::GetMatch > comparator( src, dest );

	std::vector< lcs::CResult< char > > results;
	comparator.Process( results );

	std::vector< ut::MatchTriplet > triplets;

	for ( std::vector< lcs::CResult< char > >::const_iterator itResult = results.begin(); itResult != results.end(); ++itResult )
		ut::PushTriplet( triplets, *itResult );

	ASSERT_EQUAL( 3, triplets.size() );
	ASSERT_EQUAL( ut::MatchTriplet( str::MatchEqual, "what ", "what " ), triplets[ 0 ] );
	ASSERT_EQUAL( ut::MatchTriplet( str::MatchNotEqual, "ABC", "XY" ), triplets[ 1 ] );
	ASSERT_EQUAL( ut::MatchTriplet( str::MatchEqual, " around", " around" ), triplets[ 2 ] );
}

void CStringSequenceTests::TestCompareDiffCaseLCS( void )
{
	static const std::string src = "what ABC around", dest = "what abc around";

	lcs::Comparator< char, str::GetMatch > comparator( src, dest );

	std::vector< lcs::CResult< char > > results;
	comparator.Process( results );

	std::vector< ut::MatchTriplet > triplets;

	for ( std::vector< lcs::CResult< char > >::const_iterator itResult = results.begin(); itResult != results.end(); ++itResult )
		ut::PushTriplet( triplets, *itResult );

	ASSERT_EQUAL( 3, triplets.size() );
	ASSERT_EQUAL( ut::MatchTriplet( str::MatchEqual, "what ", "what " ), triplets[ 0 ] );
	ASSERT_EQUAL( ut::MatchTriplet( str::MatchEqualDiffCase, "ABC", "abc" ), triplets[ 1 ] );
	ASSERT_EQUAL( ut::MatchTriplet( str::MatchEqual, " around", " around" ), triplets[ 2 ] );
}

void CStringSequenceTests::TestCompareMidSubseqLCS( void )
{
	static const std::string src = "what goes around", dest = "what comes around";

	lcs::Comparator< char, str::GetMatch > comparator( src, dest );

	std::vector< lcs::CResult< char > > results;
	comparator.Process( results );

	std::vector< ut::MatchTriplet > triplets;

	for ( std::vector< lcs::CResult< char > >::const_iterator itResult = results.begin(); itResult != results.end(); ++itResult )
		ut::PushTriplet( triplets, *itResult );

	ASSERT_EQUAL( 5, triplets.size() );
	ASSERT_EQUAL( ut::MatchTriplet( str::MatchEqual, "what ", "what " ), triplets[ 0 ] );
	ASSERT_EQUAL( ut::MatchTriplet( str::MatchNotEqual, "g", "c" ), triplets[ 1 ] );
	ASSERT_EQUAL( ut::MatchTriplet( str::MatchEqual, "o", "o" ), triplets[ 2 ] );			// innner common sequence
	ASSERT_EQUAL( ut::MatchTriplet( str::MatchNotEqual, "", "m" ), triplets[ 3 ] );
	ASSERT_EQUAL( ut::MatchTriplet( str::MatchEqual, "es around", "es around" ), triplets[ 4 ] );
}

void CStringSequenceTests::TestFindLongestDuplicatedString( void )
{
	ASSERT_EQUAL( "", str::FindLongestDuplicatedString( std::string(), pred::CompareCase() ) );
	ASSERT_EQUAL( " the people, ", str::FindLongestDuplicatedString( std::string( "of the people, by the people, for the people," ), pred::CompareCase() ) );
	ASSERT_EQUAL( "he people, ", str::FindLongestDuplicatedString( std::string( "of the people, by The people, for tHE people," ), pred::CompareCase() ) );
	ASSERT_EQUAL( " THE people, ", str::FindLongestDuplicatedString( std::string( "of THE people, by The people, for tHE people," ), pred::CompareNoCase() ) );		// first match wins
}

void CStringSequenceTests::TestFindLongestDuplicatedMultiSource( void )
{
	std::vector< std::wstring > items;
	items.push_back( L"of the people from Italy" );
	items.push_back( L"by the people of Italy" );
	items.push_back( L"for the people in Italy" );
	ASSERT_EQUAL( L" the people ", str::FindLongestCommonSubstring( items, pred::CompareCase() ) );
}

void CStringSequenceTests::Run( void )
{
	TRACE( _T("-- UTL String Sequence tests (LongestCommonSubsequence) --\n") );

	TestCompareSimpleLCS();
	TestCompareDiffCaseLCS();
	TestCompareMidSubseqLCS();
	TestFindLongestDuplicatedString();
	TestFindLongestDuplicatedMultiSource();
}


#endif //_DEBUG
