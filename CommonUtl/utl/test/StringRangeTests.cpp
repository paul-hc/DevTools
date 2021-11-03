
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/StringRangeTests.h"
#include "StringRange.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CStringRangeTests::CStringRangeTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CStringRangeTests& CStringRangeTests::Instance( void )
{
	static CStringRangeTests s_testCase;
	return s_testCase;
}

void CStringRangeTests::TestInit( void )
{
	{
		static const std::tstring text;
		str::range::CStringRange< wchar_t > textRange( text );
		ASSERT( textRange.InBounds() );
		ASSERT( textRange.IsEmpty() );
		ASSERT_EQUAL( 0, textRange.GetLength() );
		ASSERT_EQUAL( 0, textRange.GetPos().m_start );
		ASSERT_EQUAL( 0, textRange.GetPos().m_end );
		ASSERT_EQUAL( _T(""), textRange.Extract() );

		++textRange.RefPos().m_end;
		ASSERT( !textRange.InBounds() );
	}

	{
		static const std::tstring text = _T("abcd");
		str::range::CStringRange< wchar_t > textRange( text );
		ASSERT( textRange.InBounds() );
		ASSERT( !textRange.IsEmpty() );
		ASSERT_EQUAL( 4, textRange.GetLength() );
		ASSERT_EQUAL( 0, textRange.GetPos().m_start );
		ASSERT_EQUAL( 4, textRange.GetPos().m_end );
		ASSERT_EQUAL( _T("abcd"), textRange.Extract() );

		++textRange.RefPos().m_start;
		--textRange.RefPos().m_end;
		ASSERT( textRange.InBounds() );
		ASSERT( !textRange.IsEmpty() );
		ASSERT_EQUAL( _T("bc"), textRange.Extract() );

		textRange.Reset();
		ASSERT_EQUAL( _T("abcd"), textRange.Extract() );

		ASSERT_EQUAL( _T("ab"), textRange.ExtractLead( 2 ) );
		ASSERT_EQUAL( _T("cd"), textRange.ExtractTrail( 2 ) );
	}
}

void CStringRangeTests::TestFind( void )
{
	static const std::string s_text = "of the people, by the People, for the PEOPLE,";
	str::range::CStringRange< char > textRange( s_text );

	Range< size_t > foundPos;
	ASSERT( !textRange.Find( foundPos, 'F' ) );
	ASSERT( textRange.Find( foundPos, 'F', str::IgnoreCase ) );
	ASSERT_EQUAL( "f", str::range::Extract( foundPos, s_text ) );

	ASSERT( !textRange.Find( foundPos, "PEople" ) );
	ASSERT( textRange.Find( foundPos, "PEople", str::IgnoreCase ) );
	ASSERT_EQUAL( "people", str::range::Extract( foundPos, s_text ) );
	ASSERT_EQUAL( 7, foundPos.m_start );

	textRange.RefPos().m_start = foundPos.m_end;			// skip past first match
	ASSERT( textRange.Find( foundPos, "People" ) );
	ASSERT_EQUAL( "People", str::range::Extract( foundPos, s_text ) );
	ASSERT_EQUAL( 22, foundPos.m_start );

	textRange.RefPos().m_start = foundPos.m_end;			// skip past second match
	ASSERT( textRange.Find( foundPos, "people", str::IgnoreCase ) );
	ASSERT_EQUAL( "PEOPLE", str::range::Extract( foundPos, s_text ) );
	ASSERT_EQUAL( 38, foundPos.m_start );
}

void CStringRangeTests::TestTrim( void )
{
	{
		static const std::string text = " \t  abcd  \t ";
		str::range::CStringRange< char > textRange( text );

		ASSERT( textRange.Trim() );
		ASSERT_EQUAL( "abcd", textRange.Extract() );

		ASSERT( textRange.Trim( " \tad" ) );
		ASSERT_EQUAL( "bc", textRange.Extract() );
	}

	// nested trim
	{
		static const std::tstring text = _T(" \t  -_-abcd__-  \t ");
		str::range::CStringRange< wchar_t > textRange( text );

		ASSERT( textRange.Trim() );
		ASSERT_EQUAL( _T("-_-abcd__-"), textRange.Extract() );

		ASSERT( textRange.TrimLeft( _T("-_") ) );
		ASSERT_EQUAL( _T("abcd__-"), textRange.Extract() );

		ASSERT( textRange.TrimRight( _T("-_") ) );
		ASSERT_EQUAL( _T("abcd"), textRange.Extract() );

		ASSERT( textRange.Trim( _T("da") ) );
		ASSERT_EQUAL( _T("bc"), textRange.Extract() );
	}
}

void CStringRangeTests::TestStrip( void )
{
	static const std::string text = "preABCDpost";
	str::range::CStringRange< char > textRange( text );

	ASSERT( textRange.HasPrefix( "" ) );
	ASSERT( textRange.HasPrefix( '\0' ) );
	ASSERT( textRange.HasSuffix( "" ) );
	ASSERT( textRange.HasSuffix( '\0' ) );

	ASSERT( textRange.StripPrefix( '\0' ) );
	ASSERT( textRange.StripSuffix( '\0' ) );
	ASSERT( textRange.Strip( "", "" ) );
	ASSERT( textRange.Strip( '\0', '\0' ) );
	ASSERT_EQUAL( text, textRange.Extract() );

	ASSERT( textRange.HasPrefix( "pre" ) );
	ASSERT( !textRange.HasPrefix( "xy" ) );
	ASSERT( textRange.HasSuffix( "post" ) );
	ASSERT( !textRange.HasSuffix( "xy" ) );

	ASSERT( textRange.StripPrefix( "pre" ) );
	ASSERT( !textRange.StripPrefix( "xy" ) );
	ASSERT_EQUAL( "ABCDpost", textRange.Extract() );

	ASSERT( textRange.StripSuffix( "post" ) );
	ASSERT( !textRange.StripPrefix( "xy" ) );
	ASSERT_EQUAL( "ABCD", textRange.Extract() );

	ASSERT( textRange.StripPrefix( 'A' ) );
	ASSERT( !textRange.StripPrefix( 'x' ) );
	ASSERT_EQUAL( "BCD", textRange.Extract() );

	ASSERT( textRange.StripSuffix( 'D' ) );
	ASSERT( !textRange.StripPrefix( 'y' ) );
	ASSERT_EQUAL( "BC", textRange.Extract() );

	ASSERT( textRange.Equals( "BC" ) );
	ASSERT( textRange.Equals( "bc", str::IgnoreCase ) );
}

void CStringRangeTests::TestSplit( void )
{
	static const std::string s_text = " \t  item A  \t";
	str::range::CStringRange< char > textRange( s_text );
	ASSERT( textRange.Trim() );
	ASSERT_EQUAL( "item A", textRange.Extract() );

	Range< size_t > sepPos;
	ASSERT( !textRange.Find( sepPos, _T('\t') ) );
	ASSERT( textRange.Find( sepPos, _T(' ') ) );

	// did it split in the right place (inner space)
	std::string leading, trailing;
	textRange.SplitPair( leading, trailing, sepPos );
	ASSERT_EQUAL( "item", leading );
	ASSERT_EQUAL( "A", trailing );
}

void CStringRangeTests::Run( void )
{
	__super::Run();

	TestInit();
	TestFind();
	TestTrim();
	TestStrip();
	TestSplit();
}


#endif //USE_UT
