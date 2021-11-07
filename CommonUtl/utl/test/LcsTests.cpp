
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/LcsTests.h"
#include "LongestCommonSubsequence.h"
#include "LongestCommonDuplicate.h"
#include "RandomUtilities.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	inline std::vector< std::string > Split( const char* pSource, const char sep[] = "|" )
	{
		std::vector< std::string > items;
		str::Split( items, pSource, sep );
		return items;
	}

	inline std::vector< std::wstring > Split( const wchar_t* pSource, const wchar_t sep[] = _T("|") )
	{
		std::vector< std::wstring > items;
		str::Split( items, pSource, sep );
		return items;
	}

	template< typename CharType >
	void MakeRandomItems( std::vector< std::basic_string< CharType > >& rItems, const CharType fragment[], size_t minItems, size_t maxItems, size_t maxLen, const Range<CharType>& charRange )
	{
		utl::GenerateRandomStrings( rItems, utl::GetRandomValue( minItems, maxItems ), maxLen - str::GetLength( fragment ), charRange );
		utl::InsertFragmentRandomly( rItems, fragment );
	}


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
		std::tostringstream oss;
		switch ( triplet.m_match )
		{
			case str::MatchEqual:			oss << "EQ"; break;
			case str::MatchEqualDiffCase:	oss << "~EQ"; break;
			case str::MatchNotEqual:		oss << "DIFF"; break;
		}

		oss
			<< ':'
			<< " S={" << triplet.m_src << '}'
			<< " D={" << triplet.m_dest << '}';

		return oss.str();
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


// CLcsTests implementation

CLcsTests::CLcsTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CLcsTests::~CLcsTests()
{
}

CLcsTests& CLcsTests::Instance( void )
{
	static CLcsTests s_testCase;
	return s_testCase;
}

void CLcsTests::TestSuffixTreeGutsAnsi( void )
{
	lcs::CSuffixTree< char, pred::TCompareCase > suffixTree( "of the people, by the people, for the people," );
	std::vector< const char* >::const_iterator itSuffix = suffixTree.m_suffixes.begin();

	ASSERT_EQUAL( _T(" by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T(" for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T(" people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T(" people, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T(" people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T(" the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T(" the people, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T(" the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T(","), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T(", by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T(", for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("e people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("e people, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("e people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("e,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("e, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("e, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("eople,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("eople, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("eople, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("f the people, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("he people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("he people, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("he people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("le,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("le, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("le, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("of the people, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("ople,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("ople, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("ople, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("or the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("people, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("ple,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("ple, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("ple, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("r the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("the people, by the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
	ASSERT_EQUAL( _T("y the people, for the people,"), str::FromAnsi( *itSuffix++ ) );
}

void CLcsTests::TestSuffixTreeGutsWide( void )
{
	lcs::CSuffixTree< TCHAR, pred::TCompareCase > suffixTree( _T("of the people, by the people, for the people,") );
	std::vector< const TCHAR* >::const_iterator itSuffix = suffixTree.m_suffixes.begin();

	ASSERT_EQUAL_STR( _T(" by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T(" for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T(" people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T(" people, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T(" people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T(" the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T(" the people, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T(" the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T(","), *itSuffix++ );
	ASSERT_EQUAL_STR( _T(", by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T(", for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("e people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("e people, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("e people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("e,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("e, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("e, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("eople,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("eople, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("eople, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("f the people, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("he people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("he people, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("he people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("le,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("le, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("le, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("of the people, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("ople,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("ople, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("ople, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("or the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("people, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("ple,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("ple, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("ple, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("r the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("the people, by the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("the people, for the people,"), *itSuffix++ );
	ASSERT_EQUAL_STR( _T("y the people, for the people,"), *itSuffix++ );
}

void CLcsTests::TestFindLongestDuplicatedString( void )
{
	ASSERT_EQUAL( "", str::FindLongestDuplicatedString( std::string(), pred::TCompareCase() ) );
	ASSERT_EQUAL( L"", str::FindLongestDuplicatedString( std::wstring(), pred::TCompareCase() ) );

	ASSERT_EQUAL( " the people, ", str::FindLongestDuplicatedString( std::string( "of the people, by the people, for the people," ), pred::TCompareCase() ) );
	ASSERT_EQUAL( "he people, ", str::FindLongestDuplicatedString( std::string( "of the people, by The people, for tHE people," ), pred::TCompareCase() ) );
	ASSERT_EQUAL( " THE people, ", str::FindLongestDuplicatedString( std::string( "of THE people, by The people, for tHE people," ), pred::TCompareNoCase() ) );		// first match wins

	ASSERT_EQUAL( L"abcd", str::FindLongestDuplicatedString( std::wstring( L"abcdabcdbcd" ) ) );
	ASSERT_EQUAL( L"ABCD", str::FindLongestDuplicatedString( std::wstring( L"ABCDabcdbcd" ), pred::TCompareNoCase() ) );		// first match wins
	ASSERT_EQUAL( L"abcd", str::FindLongestDuplicatedString( std::wstring( L"abcdABCDbcd" ), pred::TCompareNoCase() ) );		// first match wins
}

void CLcsTests::TestFindLongestCommonSubstring( void )
{
	{
		std::vector< std::wstring > items;
		ASSERT_EQUAL( L"", str::FindLongestCommonSubstring( items ) );

		items.push_back( std::wstring() );
		ASSERT_EQUAL( L"", str::FindLongestCommonSubstring( items ) );
	}

	{
		ASSERT_EQUAL( "bcd", str::FindLongestCommonSubstring( ut::Split( "abcdabcd|bcd" ) ) );		// ensure it doesn't return "abcd"
		ASSERT_EQUAL( "bcd", str::FindLongestCommonSubstring( ut::Split( "bcd|abcdabcd" ) ) );
		ASSERT_EQUAL( "bcd", str::FindLongestCommonSubstring( ut::Split( "PbcdQ|XabcdabcdY" ) ) );
		ASSERT_EQUAL( "", str::FindLongestCommonSubstring( ut::Split( "bcd|abcdabcd|" ) ) );
		ASSERT_EQUAL( "", str::FindLongestCommonSubstring( ut::Split( "bcd|abcdabcd|xy" ) ) );
		ASSERT_EQUAL( "cd", str::FindLongestCommonSubstring( ut::Split( "xcd|bcd|abcdabcd|cd|ycd" ) ) );
		ASSERT( str::Equals< str::IgnoreCase >( "cd", str::FindLongestCommonSubstring( ut::Split( "xcd|bcd|abcdabcd|cd|yCD" ), pred::TCompareNoCase() ).c_str() ) );

		ASSERT_EQUAL_IGNORECASE( "cd", str::FindLongestCommonSubstring( ut::Split( "xcd|bcd|abcdabcd|cd|yCD" ), pred::TCompareNoCase() ) );
	}

	{
		std::vector< std::wstring > items;
		ASSERT_EQUAL( L"", str::FindLongestCommonSubstring( items, pred::TCompareCase() ) );

		items.push_back( L"of the people from Italy" );
		ASSERT_EQUAL( L"of the people from Italy", str::FindLongestCommonSubstring( items, pred::TCompareCase() ) );

		items.push_back( L"by the people of Italy" );
		items.push_back( L"for the people in Italy" );
		ASSERT_EQUAL( L" the people ", str::FindLongestCommonSubstring( items, pred::TCompareCase() ) );
	}
}

void CLcsTests::TestRandomLongestCommonSubstring( void )
{
	const Range<wchar_t> charRange = utl::GetRangeLowerLetters< wchar_t >();
	static const wchar_t fragment[] = L" XYZ ";

	for ( size_t i = 0; i != 5; ++i )
	{
		std::vector< std::wstring > items;
		ut::MakeRandomItems( items, fragment, 2, 16, 32, charRange );

		ASSERT( str::ContainsPart( str::FindLongestCommonSubstring( items ).c_str(), str::MakePart( fragment ) ) );
	}
}


void CLcsTests::TestMatchingSequenceSimple( void )
{
	static const std::string src = "what ABC around", dest = "what XY around";

	lcs::Comparator<char, str::TGetMatch> comparator( src, dest );

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

void CLcsTests::TestMatchingSequenceDiffCase( void )
{
	static const std::string src = "what ABC around", dest = "what abc around";

	lcs::Comparator<char, str::TGetMatch> comparator( src, dest );

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

void CLcsTests::TestMatchingSequenceMidCommon( void )
{
	static const std::string src = "what goes around", dest = "what comes around";

	lcs::Comparator<char, str::TGetMatch> comparator( src, dest );

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

void CLcsTests::Run( void )
{
	__super::Run();

	TestSuffixTreeGutsAnsi();
	TestSuffixTreeGutsWide();
	TestFindLongestDuplicatedString();
	TestFindLongestCommonSubstring();
	TestRandomLongestCommonSubstring();

	TestMatchingSequenceSimple();
	TestMatchingSequenceDiffCase();
	TestMatchingSequenceMidCommon();
}


#endif //USE_UT
