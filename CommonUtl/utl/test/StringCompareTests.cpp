
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "StringCompare.h"
#include "Algorithms.h"
#include "FlexPath.h"
#include "test/StringCompareTests.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CStringCompareTests::CStringCompareTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CStringCompareTests& CStringCompareTests::Instance( void )
{
	static CStringCompareTests s_testCase;
	return s_testCase;
}

void CStringCompareTests::TestIgnoreCase( void )
{
	std::string s( "bcd" );
	ASSERT( "BCD" != s );
	ASSERT( s != "BCD" );

	std::wstring ws( L"bcd" );
	ASSERT( L"BCD" != ws );
	ASSERT( ws != L"BCD" );

	{
		using namespace str::ignore_case;

		ASSERT( "BCD" == s );
		ASSERT( s == "BCD" );
		ASSERT( s != "xy" );

		ASSERT( L"BCD" == ws );
		ASSERT( ws == L"BCD" );
		ASSERT( ws != L"xy" );
	}
}

void CStringCompareTests::TestStringCompare( void )
{
	typedef std::pair<pred::CompareResult, size_t> TResult;

	// case-sensitive compare (str::Case)
	{
		const func::StrCompare<> strCompare;
		const pred::StrEquals<> strEquals;

		ASSERT( pred::Equal == strCompare( "", "" ) );
		ASSERT( pred::Greater == strCompare( "abc", "" ) );
		ASSERT( pred::Less == strCompare( "", "abc" ) );
		ASSERT_EQUAL( TResult( pred::Equal, 0 ), strCompare.Compare( "abc", "", 0 ) );
		ASSERT_EQUAL( TResult( pred::Equal, 0 ), strCompare.Compare( "", "abc", 0 ) );

		ASSERT_EQUAL( TResult( pred::Equal, 1 ), strCompare.Compare( "abc", "a", 1 ) );
		ASSERT( pred::Greater == strCompare( "abc", "a" ) );
		ASSERT( pred::Less == strCompare( "a", "abc" ) );

		ASSERT_EQUAL( TResult( pred::Equal, 2 ), strCompare.Compare( "abc", "ab", 2 ) );
		ASSERT( pred::Greater == strCompare( L"abc", "ab" ) );
		ASSERT( pred::Less == strCompare( L"ab", "abc" ) );

		// count overflow
		ASSERT_EQUAL( TResult( pred::Greater, 2 ), strCompare.Compare( "abc", L"ab", 3 ) );
		ASSERT( pred::Greater == strCompare( "abc", L"ab", 3 ) );
		ASSERT( pred::Less == strCompare( "ab", L"abc", 3 ) );

		// partial match
		ASSERT_EQUAL( TResult( pred::Equal, 1 ), strCompare.Compare( "abc", "ax", 1 ) );
		ASSERT_EQUAL( TResult( pred::Equal, 1 ), strCompare.Compare( "ax", "abc", 1 ) );
		// partial mismatch
		ASSERT_EQUAL( TResult( pred::Less, 1 ), strCompare.Compare( "abc", "ax", 2 ) );
		ASSERT( pred::Less == strCompare( "abc", "ax" ) );
		ASSERT( pred::Greater == strCompare( "ax", "abc" ) );

		// narrow-wide compare
		ASSERT( pred::Equal == strCompare( "abc", "abc" ) );
		ASSERT( pred::Equal == strCompare( "abc", L"abc" ) );
		ASSERT( pred::Equal == strCompare( L"abc", "abc" ) );
		ASSERT( pred::Equal == strCompare( L"abc", L"abc" ) );

		// case mismatch
		ASSERT( pred::Greater == strCompare( L"abc", "ABC" ) );
		ASSERT( pred::Less == strCompare( L"ABC", L"abc" ) );

		// equals:
		ASSERT( strEquals( "", "" ) );
		ASSERT( !strEquals( "abc", "" ) );
		ASSERT( !strEquals( "", "abc" ) );

		// equalsN:
		ASSERT( strEquals( "abc", "", 0 ) );

		ASSERT( strEquals( "abc", L"a", 1 ) );
		ASSERT( strEquals( "a", L"abc", 1 ) );

		ASSERT( strEquals( L"abc", "ab", 2 ) );
		ASSERT( strEquals( L"ab", "abc", 2 ) );

		// mismatch
		ASSERT( !strEquals( L"abc", "ab" ) );
		ASSERT( !strEquals( L"ab", "abc" ) );

		ASSERT( !strEquals( L"abc", "ax", 2 ) );
		ASSERT( !strEquals( L"ax", "abc", 2 ) );

		// partial mismatch
		ASSERT( strEquals( L"abc", "ax", 1 ) );
		ASSERT( strEquals( L"ax", "abc", 1 ) );
	}

	// case-insensitive compare
	{
		const func::StrCompare<str::IgnoreCase> strCompareIC;
		const pred::StrEquals<str::IgnoreCase> strEqualsIC;

		ASSERT( pred::Equal == strCompareIC( "", "" ) );
		ASSERT( pred::Greater == strCompareIC( "abc", "" ) );
		ASSERT( pred::Less == strCompareIC( "", "abc" ) );
		ASSERT_EQUAL( TResult( pred::Equal, 0 ), strCompareIC.Compare( "abc", "", 0 ) );
		ASSERT_EQUAL( TResult( pred::Equal, 0 ), strCompareIC.Compare( "", "abc", 0 ) );

		ASSERT_EQUAL( TResult( pred::Equal, 1 ), strCompareIC.Compare( "abc", "A", 1 ) );
		ASSERT( pred::Greater == strCompareIC( "abc", "A" ) );
		ASSERT( pred::Less == strCompareIC( "A", "abc" ) );

		ASSERT_EQUAL( TResult( pred::Equal, 2 ), strCompareIC.Compare( "abc", "AB", 2 ) );
		ASSERT( pred::Greater == strCompareIC( "abc", "AB" ) );
		ASSERT( pred::Less == strCompareIC( "AB", "abc" ) );

		ASSERT_EQUAL( TResult( pred::Less, 1 ), strCompareIC.Compare( "abc", "AX", 2 ) );
		ASSERT( pred::Less == strCompareIC( "abc", "AX" ) );
		ASSERT( pred::Greater == strCompareIC( "AX", "abc" ) );

		// narrow-wide compare
		ASSERT( pred::Equal == strCompareIC( "abc", L"ABC" ) );
		ASSERT( pred::Equal == strCompareIC( L"abc", "ABC" ) );
		ASSERT( pred::Equal == strCompareIC( L"abc", L"ABC" ) );

		// case mismatch
		ASSERT( pred::Equal == strCompareIC( L"abc", "ABC" ) );
		ASSERT( pred::Equal == strCompareIC( L"ABC", L"abc" ) );

		// equals:
		ASSERT( strEqualsIC( "", "" ) );
		ASSERT( !strEqualsIC( "abc", "" ) );
		ASSERT( !strEqualsIC( "", "abc" ) );

		// equalsN:
		ASSERT( strEqualsIC( "abc", "", 0 ) );

		ASSERT( strEqualsIC( "abc", L"A", 1 ) );
		ASSERT( strEqualsIC( "A", L"abc", 1 ) );

		ASSERT( strEqualsIC( L"abc", "AB", 2 ) );
		ASSERT( strEqualsIC( L"AB", "abc", 2 ) );

		// mismatch
		ASSERT( !strEqualsIC( L"abc", "AB" ) );
		ASSERT( !strEqualsIC( L"ab", "ABC" ) );

		ASSERT( !strEqualsIC( L"abc", "AX", 2 ) );
		ASSERT( !strEqualsIC( L"AX", "abc", 2 ) );

		// partial mismatch
		ASSERT( strEqualsIC( L"abc", "AX", 1 ) );
		ASSERT( strEqualsIC( L"AX", "abc", 1 ) );
	}
}

namespace ut
{
	template< typename CharT >
	inline std::pair<pred::CompareResult, size_t> BaseCompare( const CharT* pLeft, const CharT* pRight, size_t count = std::tstring::npos )
	{
		return str::_BaseCompare( pLeft, pRight, func::ToCharValue<>(), count );
	}

	template< typename CharT >
	inline std::pair<pred::CompareResult, size_t> BaseCompareI( const CharT* pLeft, const CharT* pRight, size_t count = std::tstring::npos )
	{
		return str::_BaseCompare( pLeft, pRight, func::ToCharValue<str::IgnoreCase>(), count );
	}

	template< typename CharT >
	inline bool BaseEquals( const CharT* pLeft, const CharT* pRight, size_t count = std::tstring::npos )
	{
		return pred::Equal == str::_BaseCompare( pLeft, pRight, func::ToCharValue<>(), count ).first;
	}

	template< typename CharT >
	inline bool BaseEqualsI( const CharT* pLeft, const CharT* pRight, size_t count = std::tstring::npos )
	{
		return pred::Equal == str::_BaseCompare( pLeft, pRight, func::ToCharValue<str::IgnoreCase>(), count ).first;
	}
}

void CStringCompareTests::TestStringCompare2( void )
{
	typedef std::pair<pred::CompareResult, size_t> TResult;

	// case-sensitive compare (str::Case)
	{
		ASSERT( pred::Equal == ut::BaseCompare( "", "" ).first );
		ASSERT( pred::Greater == ut::BaseCompare( "abc", "" ).first );
		ASSERT( pred::Less == ut::BaseCompare( "", "abc" ).first );
		ASSERT_EQUAL( TResult( pred::Equal, 0 ), ut::BaseCompare( "abc", "", 0 ) );
		ASSERT_EQUAL( TResult( pred::Equal, 0 ), ut::BaseCompare( "", "abc", 0 ) );

		ASSERT_EQUAL( TResult( pred::Equal, 1 ), ut::BaseCompare( "abc", "a", 1 ) );
		ASSERT( pred::Greater == ut::BaseCompare( "abc", "a" ).first );
		ASSERT( pred::Less == ut::BaseCompare( "a", "abc" ).first );

		ASSERT_EQUAL( TResult( pred::Equal, 2 ), ut::BaseCompare( "abc", "ab", 2 ) );
		ASSERT( pred::Greater == ut::BaseCompare( "abc", "ab" ).first );
		ASSERT( pred::Less == ut::BaseCompare( "ab", "abc" ).first );

		ASSERT_EQUAL( TResult( pred::Less, 1 ), ut::BaseCompare( "abc", "ax", 2 ) );
		ASSERT( pred::Less == ut::BaseCompare( "abc", "ax" ).first );
		ASSERT( pred::Greater == ut::BaseCompare( "ax", "abc" ).first );

		// narrow-wide compare
		ASSERT( pred::Equal == ut::BaseCompare( "abc", "abc" ).first );
		ASSERT( pred::Equal == ut::BaseCompare( L"abc", L"abc" ).first );

		// case mismatch
		ASSERT( pred::Greater == ut::BaseCompare( "abc", "ABC" ).first );
		ASSERT( pred::Less == ut::BaseCompare( L"ABC", L"abc" ).first );

		// equals:
		ASSERT( ut::BaseEquals( "", "" ) );
		ASSERT( !ut::BaseEquals( "abc", "" ) );
		ASSERT( !ut::BaseEquals( "", "abc" ) );

		// equalsN:
		ASSERT( ut::BaseEquals( "abc", "", 0 ) );

		ASSERT( ut::BaseEquals( "abc", "a", 1 ) );
		ASSERT( ut::BaseEquals( L"a", L"abc", 1 ) );

		ASSERT( ut::BaseEquals( "abc", "ab", 2 ) );
		ASSERT( ut::BaseEquals( L"ab", L"abc", 2 ) );

		ASSERT( !ut::BaseEquals( "abc", "ax", 2 ) );
		ASSERT( !ut::BaseEquals( L"ax", L"abc", 2 ) );
	}

	// case-insensitive compare
	{
		ASSERT( pred::Equal == ut::BaseCompareI( "", "" ).first );
		ASSERT( pred::Greater == ut::BaseCompareI( "abc", "" ).first );
		ASSERT( pred::Less == ut::BaseCompareI( "", "abc" ).first );
		ASSERT_EQUAL( TResult( pred::Equal, 0 ), ut::BaseCompareI( "abc", "", 0 ) );
		ASSERT_EQUAL( TResult( pred::Equal, 0 ), ut::BaseCompareI( "", "abc", 0 ) );

		ASSERT_EQUAL( TResult( pred::Equal, 1 ), ut::BaseCompareI( "abc", "A", 1 ) );
		ASSERT( pred::Greater == ut::BaseCompareI( "abc", "A" ).first );
		ASSERT( pred::Less == ut::BaseCompareI( "A", "abc" ).first );

		ASSERT_EQUAL( TResult( pred::Equal, 2 ), ut::BaseCompareI( "abc", "AB", 2 ) );
		ASSERT( pred::Greater == ut::BaseCompareI( "abc", "AB" ).first );
		ASSERT( pred::Less == ut::BaseCompareI( "AB", "abc" ).first );

		ASSERT_EQUAL( TResult( pred::Less, 1 ), ut::BaseCompareI( "abc", "AX", 2 ) );
		ASSERT( pred::Less == ut::BaseCompareI( "abc", "AX" ).first );
		ASSERT( pred::Greater == ut::BaseCompareI( "AX", "abc" ).first );

		// narrow-wide compare
		ASSERT( pred::Equal == ut::BaseCompareI( "abc", "ABC" ).first );
		ASSERT( pred::Equal == ut::BaseCompareI( L"abc", L"ABC" ).first );

		// case mismatch
		ASSERT( pred::Equal == ut::BaseCompareI( "abc", "ABC" ).first );
		ASSERT( pred::Equal == ut::BaseCompareI( L"ABC", L"abc" ).first );

		// equals:
		ASSERT( ut::BaseEqualsI( "", "" ) );
		ASSERT( !ut::BaseEqualsI( "abc", "" ) );
		ASSERT( !ut::BaseEqualsI( "", "abc" ) );

		// equalsN:
		ASSERT( ut::BaseEqualsI( "abc", "", 0 ) );

		ASSERT( ut::BaseEqualsI( "abc", "A", 1 ) );
		ASSERT( ut::BaseEqualsI( "A", "abc", 1 ) );

		ASSERT( ut::BaseEqualsI( L"abc", L"AB", 2 ) );
		ASSERT( ut::BaseEqualsI( L"AB", L"abc", 2 ) );

		ASSERT( !ut::BaseEqualsI( "abc", "AX", 2 ) );
		ASSERT( !ut::BaseEqualsI( L"AX", L"abc", 2 ) );
	}
}

void CStringCompareTests::TestStringFind( void )
{
	// str::Contains, str::ContainsI
	{
		ASSERT( !str::Contains( "", L'a' ) );
		ASSERT( str::Contains( L"abc", 'a' ) && str::Contains( L"abc", 'b' ) && str::Contains( L"abc", 'c' ) );
		ASSERT( !str::Contains( L"abc", 'A' ) );
		ASSERT( !str::Contains( "abc", L'x' ) );

		ASSERT( !str::ContainsI( "", L'a' ) );
		ASSERT( str::ContainsI( L"abc", 'a' ) && str::ContainsI( L"abc", 'b' ) && str::ContainsI( L"abc", 'c' ) );
		ASSERT( str::ContainsI( L"abc", 'A' ) );
		ASSERT( str::ContainsI( L"ABC", 'a' ) );
		ASSERT( !str::ContainsI( "abc", L'x' ) && !str::ContainsI( "abc", L'X' ) );
	}

	// str::Search, str::SearchI
	{	// narrow text
		std::string text = "Some text";

		ASSERT_EQUAL_STR( text, &*str::Search( text.begin(), text.end(), std::string( "Some" ) ) );
		ASSERT_EQUAL_STR( "text", &*str::Search( text.begin(), text.end(), std::string( "text" ) ) );

		ASSERT_EQUAL_STR( text, &*str::Search( text.begin(), text.end(), std::wstring( L"Some" ) ) );
		ASSERT_EQUAL_STR( "text", &*str::Search( text.begin(), text.end(), std::wstring( L"text" ) ) );

		ASSERT_EQUAL_STR( text, &*str::SearchI( text.begin(), text.end(), std::string( "sOME" ) ) );
		ASSERT_EQUAL_STR( "text", &*str::SearchI( text.begin(), text.end(), std::string( "TEXT" ) ) );

		ASSERT_EQUAL_STR( text, &*str::SearchI( text.begin(), text.end(), std::wstring( L"sOME" ) ) );
		ASSERT_EQUAL_STR( "text", &*str::SearchI( text.begin(), text.end(), std::wstring( L"TEXT" ) ) );

		// pointer version:
		ASSERT_EQUAL_STR( text, str::Search( text.c_str(), text.c_str() + text.length(), std::string("Some") ) );
		ASSERT_EQUAL_STR( "text", str::Search( text.c_str(), text.c_str() + text.length(), std::string( "text" ) ) );
	}
	{	// wide text
		std::wstring text = L"Some text";

		ASSERT_EQUAL_STR( text, &*str::Search( text.begin(), text.end(), std::string( "Some" ) ) );
		ASSERT_EQUAL_STR( L"text", &*str::Search( text.begin(), text.end(), std::string( "text" ) ) );

		ASSERT_EQUAL_STR( text, &*str::Search( text.begin(), text.end(), std::wstring( L"Some" ) ) );
		ASSERT_EQUAL_STR( L"text", &*str::Search( text.begin(), text.end(), std::wstring( L"text" ) ) );

		ASSERT_EQUAL_STR( text, &*str::SearchI( text.begin(), text.end(), std::string( "sOME" ) ) );
		ASSERT_EQUAL_STR( L"text", &*str::SearchI( text.begin(), text.end(), std::string( "TEXT" ) ) );

		ASSERT_EQUAL_STR( text, &*str::SearchI( text.begin(), text.end(), std::wstring( L"sOME" ) ) );
		ASSERT_EQUAL_STR( L"text", &*str::SearchI( text.begin(), text.end(), std::wstring( L"TEXT" ) ) );
	}

	// str::Find character
	{
		{	// narrow text
			ASSERT_EQUAL( 0, str::Find<str::Case>( "Some text", 'S' ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::Case>( "Some text", 's' ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::Case>( "Some text", 'S', 3 ) );		// offset past match

			ASSERT_EQUAL( 0, str::Find<str::Case>( "Some text", L'S' ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::Case>( "Some text", L's' ) );

			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( "Some text", 't' ) );
			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( "Some text", 'T' ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::IgnoreCase>( "Some text", 'q' ) );

			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( "Some text", L't' ) );
			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( "Some text", L'T' ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::IgnoreCase>( "Some text", L'q' ) );
		}
		{	// wide text
			ASSERT_EQUAL( 0, str::Find<str::Case>( L"Some text", 'S' ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::Case>( L"Some text", 's' ) );

			ASSERT_EQUAL( 0, str::Find<str::Case>( L"Some text", L'S' ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::Case>( L"Some text", L's' ) );

			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( L"Some text", 't' ) );
			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( L"Some text", 'T' ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::IgnoreCase>( L"Some text", 'q' ) );

			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( L"Some text", L't' ) );
			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( L"Some text", L'T' ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::IgnoreCase>( L"Some text", L'q' ) );
		}
	}

	// str::Find sequence (sub-string)
	{
		{	// narrow text
			ASSERT_EQUAL( 0, str::Find<str::Case>( "Some text", "" ) );
			ASSERT_EQUAL( 0, str::Find<str::IgnoreCase>( "Some text", "" ) );

			ASSERT_EQUAL( 0, str::Find<str::Case>( "Some text", "Some" ) );
			ASSERT_EQUAL( 0, str::Find<str::Case>( "Some text", "SoXY", 0, 2 ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::Case>( "Some text", "some" ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::Case>( "Some text", "Some", 1 ) );	// offset past match

			ASSERT_EQUAL( 0, str::Find<str::Case>( "Some text", L"Some" ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::Case>( "Some text", L"SOME" ) );

			ASSERT_EQUAL( 0, str::Find<str::IgnoreCase>( "Some text", L"sOME" ) );
			ASSERT_EQUAL( 0, str::Find<str::IgnoreCase>( "Some text", L"SOME" ) );

			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( "Some text", "text" ) );
			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( "Some text", "TEXT" ) );
			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( "Some text", "TEab", 0, 2 ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::IgnoreCase>( "Some text", "q" ) );

			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( "Some text", L"text" ) );
			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( "Some text", L"TEXT" ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::IgnoreCase>( "Some text", L"q" ) );
		}
		{	// wide text
			ASSERT_EQUAL( 0, str::Find<str::Case>( L"Some text", "" ) );
			ASSERT_EQUAL( 0, str::Find<str::IgnoreCase>( L"Some text", "" ) );

			ASSERT_EQUAL( 0, str::Find<str::Case>( L"Some text", "Some" ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::Case>( L"Some text", "some" ) );

			ASSERT_EQUAL( 0, str::Find<str::Case>( L"Some text", L"Some" ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::Case>( L"Some text", L"SOME" ) );

			ASSERT_EQUAL( 0, str::Find<str::IgnoreCase>( L"Some text", L"sOME" ) );
			ASSERT_EQUAL( 0, str::Find<str::IgnoreCase>( L"Some text", L"SOME" ) );

			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( L"Some text", "text" ) );
			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( L"Some text", "TEXT" ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::IgnoreCase>( "Some text", "q" ) );

			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( L"Some text", L"text" ) );
			ASSERT_EQUAL( 5, str::Find<str::IgnoreCase>( L"Some text", L"TEXT" ) );
			ASSERT_EQUAL( utl::npos, str::Find<str::IgnoreCase>( "Some text", L"q" ) );
		}
	}

	ASSERT_EQUAL_STR( _T(";mn"), str::FindTokenEnd( _T("abc;mn"), _T(",;") ) );
	ASSERT_EQUAL_STR( _T(",xy"), str::FindTokenEnd( _T("abc;mn,xy"), _T(",") ) );
	ASSERT_EQUAL_STR( _T(""), str::FindTokenEnd( _T("abc;mn,xy"), _T(">") ) );
}

void CStringCompareTests::TestStringFindSequence( void )
{
	ASSERT_EQUAL( std::tstring::npos, str::FindSequence( "", str::CSequence<char>( "a text", 1 ) ) );
	ASSERT_EQUAL( 2, str::FindSequence( L"a line", str::CSequence<wchar_t>( L"liquid", 2 ) ) );
	ASSERT_EQUAL( std::tstring::npos, str::FindSequence( L"a line", str::CSequence<wchar_t>( L"liquid", 3 ) ) );

	ASSERT_EQUAL( 2, str::FindSequence( "a line", str::CSequence<char>( "LIQUID", 2 ), pred::TCompareNoCase() ) );
	ASSERT_EQUAL( 2, str::FindSequence( L"a line", str::CSequence<wchar_t>( L"LIQUID", 2 ), pred::TCompareNoCase() ) );
	ASSERT_EQUAL( std::tstring::npos, str::FindSequence( "a line", str::CSequence<char>( "LIQUID", 2 ), pred::TCompareCase() ) );
	ASSERT_EQUAL( std::tstring::npos, str::FindSequence( "a line", str::CSequence<char>( "LIQUID", 2 ) ) );

	std::vector<std::string> items;
	ASSERT( !AllContain( items, str::CSequence<char>( "liquid", 2 ) ) );

	items.push_back( "a line" );
	ASSERT( AllContain( items, str::CSequence<char>( "liquid", 2 ) ) );
	ASSERT( !AllContain( items, str::CSequence<char>( "LIQUID", 2 ) ) );
	ASSERT( AllContain( items, str::CSequence<char>( "LIQUID", 2 ), pred::TCompareNoCase() ) );

	items.push_back( "OS linux" );
	ASSERT( AllContain( items, str::CSequence<char>( "liquid", 2 ) ) );
	ASSERT( !AllContain( items, str::CSequence<char>( "LIQUID", 2 ) ) );
	ASSERT( AllContain( items, str::CSequence<char>( "LIQUID", 2 ), pred::TCompareNoCase() ) );

	items.push_back( "Red Hat Linux" );
	ASSERT( AllContain( items, str::CSequence<char>( "LIQUID", 2 ), pred::TCompareNoCase() ) );
}

void CStringCompareTests::TestStringOccurenceCount( void )
{
	ASSERT_EQUAL( 0, str::GetCountOf<str::Case>( "abcde", "" ) );
	ASSERT_EQUAL( 0, str::GetCountOf<str::Case>( "abcde", " " ) );
	ASSERT_EQUAL( 1, str::GetCountOf<str::Case>( "abcde", "a" ) );
	ASSERT_EQUAL( 1, str::GetCountOf<str::Case>( "abcdeABC", "a" ) );
	ASSERT_EQUAL( 1, str::GetCountOf<str::Case>( "abcdeABC", "ab" ) );
	ASSERT_EQUAL( 2, str::GetCountOf<str::IgnoreCase>( "abcdeABC", "a" ) );
	ASSERT_EQUAL( 2, str::GetCountOf<str::IgnoreCase>( _T("abcdeABC"), _T("a") ) );
	ASSERT_EQUAL( 2, str::GetCountOf<str::IgnoreCase>( "abcdeABC", "AB" ) );

	ASSERT_EQUAL( 0, str::GetSequenceCount( "abc", str::MakeSequence( "" ) ) );
	ASSERT_EQUAL( 1, str::GetSequenceCount( _T("abc"), str::MakeSequence( _T("b") ) ) );
	ASSERT_EQUAL( 1, str::GetSequenceCount( _T("abcA"), str::MakeSequence( _T("a") ) ) );

	static const TCHAR* sepArray[] = { _T(";"), _T("|"), _T("\r\n"), _T("\n") };
	ASSERT_EQUAL_STR( _T(";"), *std::max_element( sepArray, sepArray + COUNT_OF( sepArray ), pred::LessSequenceCount<TCHAR>( _T("ABC") ) ) );
	ASSERT_EQUAL_STR( _T("\n"), *std::max_element( sepArray, sepArray + COUNT_OF( sepArray ), pred::LessSequenceCount<TCHAR>( _T("A\nB\nC") ) ) );
	ASSERT_EQUAL_STR( _T("\r\n"), *std::max_element( sepArray, sepArray + COUNT_OF( sepArray ), pred::LessSequenceCount<TCHAR>( _T("A\r\nB\r\nC") ) ) );
	ASSERT_EQUAL_STR( _T(";"), *std::max_element( sepArray, sepArray + COUNT_OF( sepArray ), pred::LessSequenceCount<TCHAR>( _T("A|B;C|D;E;F") ) ) );
}

void CStringCompareTests::TestStringMatch( void )
{
	ASSERT_EQUAL_STR( _T("Text"), str::SkipPrefix<str::Case>( _T("abcText"), _T("abc") ) );
	ASSERT_EQUAL_STR( _T("ABcText"), str::SkipPrefix<str::Case>( _T("ABcText"), _T("aBC") ) );

	ASSERT_EQUAL_STR( _T("Text"), str::SkipPrefix<str::IgnoreCase>( _T("abcText"), _T("abc") ) );
	ASSERT_EQUAL_STR( _T("Text"), str::SkipPrefix<str::IgnoreCase>( _T("ABcText"), _T("aBC") ) );

	str::TGetMatch getMatchFunc;
	ASSERT_EQUAL( str::MatchEqual, getMatchFunc( _T(""), _T("") ) );
	ASSERT_EQUAL( str::MatchEqual, getMatchFunc( _T("SomeText"), _T("SomeText") ) );
	ASSERT_EQUAL( str::MatchEqualDiffCase, getMatchFunc( _T("SomeText"), _T("sometext") ) );
	ASSERT_EQUAL( str::MatchNotEqual, getMatchFunc( _T("Some"), _T("Text") ) );
}

void CStringCompareTests::TestStringSorting( void )
{
	static const char s_src[] = "a,ab,abc,abcd,A-,AB-,ABC-,ABCD-";		// add trailing '-' to avoid arbitrary order on case-insensitive comparison

	std::vector<std::string> items;
	str::Split( items, s_src, "," );
	ASSERT_EQUAL( "A-,AB-,ABC-,ABCD-,a,ab,abc,abcd", ut::ShuffleSortJoin( items, ",", pred::TLess_StringyCase() ) );
	ASSERT_EQUAL( "a,A-,ab,AB-,abc,ABC-,abcd,ABCD-", ut::ShuffleSortJoin( items, ",", pred::TLess_StringyNoCase() ) );

	std::vector<std::wstring> wItems;
	str::Split( wItems, str::FromAnsi( s_src ).c_str(), L"," );
	ASSERT_EQUAL( L"A-,AB-,ABC-,ABCD-,a,ab,abc,abcd", ut::ShuffleSortJoin( wItems, L",", pred::TLess_StringyCase() ) );
	ASSERT_EQUAL( L"a,A-,ab,AB-,abc,ABC-,abcd,ABCD-", ut::ShuffleSortJoin( wItems, L",", pred::TLess_StringyNoCase() ) );

	std::vector<fs::CPath> paths;
	str::Split( paths, str::FromAnsi( s_src ).c_str(), L"," );
	ASSERT_EQUAL( _T("A-,AB-,ABC-,ABCD-,a,ab,abc,abcd"), ut::ShuffleSortJoin( paths, _T(","), pred::TLess_StringyCase() ) );
	ASSERT_EQUAL( _T("a,A-,ab,AB-,abc,ABC-,abcd,ABCD-"), ut::ShuffleSortJoin( paths, _T(","), pred::TLess_StringyNoCase() ) );

	std::vector<fs::CFlexPath> flexPaths;
	str::Split( flexPaths, str::FromAnsi( s_src ).c_str(), L"," );
	ASSERT_EQUAL( _T("A-,AB-,ABC-,ABCD-,a,ab,abc,abcd"), ut::ShuffleSortJoin( flexPaths, _T(","), pred::TLess_StringyCase() ) );
	ASSERT_EQUAL( _T("a,A-,ab,AB-,abc,ABC-,abcd,ABCD-"), ut::ShuffleSortJoin( flexPaths, _T(","), pred::TLess_StringyNoCase() ) );

	// vector of pointers
	{
		std::vector<const char*> ptrItems;
		utl::Assign( ptrItems, items, func::ToCharPtr() );

		ASSERT_EQUAL( "A-,AB-,ABC-,ABCD-,a,ab,abc,abcd", ut::ShuffleSortJoin( ptrItems, ",", pred::LessPtr<pred::TCompareCase>() ) );
		ASSERT_EQUAL( "a,A-,ab,AB-,abc,ABC-,abcd,ABCD-", ut::ShuffleSortJoin( ptrItems, ",", pred::LessPtr<pred::TCompareNoCase>() ) );
	}

	{
		std::vector<const wchar_t*> wPtrItems;
		utl::Assign( wPtrItems, wItems, func::ToCharPtr() );

		ASSERT_EQUAL( L"A-,AB-,ABC-,ABCD-,a,ab,abc,abcd", ut::ShuffleSortJoin( wPtrItems, L",", pred::LessPtr<pred::TCompareCase>() ) );
		ASSERT_EQUAL( L"a,A-,ab,AB-,abc,ABC-,abcd,ABCD-", ut::ShuffleSortJoin( wPtrItems, L",", pred::LessPtr<pred::TCompareNoCase>() ) );
	}
}

void CStringCompareTests::TestIntuitiveSort( void )
{
	const char s_srcItems[] = "st3Ring,2string,st2ring,STRING20,string2,3String,20STRING,st20RING,String3";

	{	// NARROW
		std::vector<std::string> items;
		str::Split( items, s_srcItems, "," );
		std::random_shuffle( items.begin(), items.end() );

		// sort intuitive via pred::CompareValue that uses pred::Compare_Scalar() specialization
		ASSERT_EQUAL(
			"2string,3String,20STRING,st2ring,st3Ring,st20RING,string2,String3,STRING20",
			ut::ShuffleSortJoin( items, ",", pred::LessValue<pred::CompareValue>() ) );

		// sort intuitive via pred::TLess_StringyIntuitive (equivalent with pred::CompareValue)
		ASSERT_EQUAL(
			"2string,3String,20STRING,st2ring,st3Ring,st20RING,string2,String3,STRING20",
			ut::ShuffleSortJoin( items, ",", pred::TLess_StringyIntuitive() ) );

		// sort case-insensitive
		ASSERT_EQUAL(
			"20STRING,2string,3String,st20RING,st2ring,st3Ring,string2,STRING20,String3",
			ut::ShuffleSortJoin( items, ",", pred::TLess_StringyNoCase() ) );

		// sort case-sensitive
		ASSERT_EQUAL(
			"20STRING,2string,3String,STRING20,String3,st20RING,st2ring,st3Ring,string2",
			ut::ShuffleSortJoin( items, ",", pred::TLess_StringyCase() ) );
	}

	{	// WIDE
		std::vector<std::wstring> items;
		str::Split( items, str::FromAnsi( s_srcItems ).c_str(), L"," );
		std::random_shuffle( items.begin(), items.end() );

		// sort intuitive via pred::CompareValue that uses pred::Compare_Scalar() specialization
		ASSERT_EQUAL(
			L"2string,3String,20STRING,st2ring,st3Ring,st20RING,string2,String3,STRING20",
			ut::ShuffleSortJoin( items, L",", pred::LessValue<pred::CompareValue>() ) );

		// sort intuitive via pred::TStringyCompareIntuitive (equivalent with pred::CompareValue)
		ASSERT_EQUAL(
			L"2string,3String,20STRING,st2ring,st3Ring,st20RING,string2,String3,STRING20",
			ut::ShuffleSortJoin( items, L",", pred::LessValue<pred::TStringyCompareIntuitive>() ) );

		// sort case-insensitive
		ASSERT_EQUAL(
			L"20STRING,2string,3String,st20RING,st2ring,st3Ring,string2,STRING20,String3",
			ut::ShuffleSortJoin( items, L",", pred::TLess_StringyNoCase() ) );

		// sort case-sensitive
		ASSERT_EQUAL(
			L"20STRING,2string,3String,STRING20,String3,st20RING,st2ring,st3Ring,string2",
			ut::ShuffleSortJoin( items, L",", pred::TLess_StringyCase() ) );
	}
}

void CStringCompareTests::TestIntuitiveSortPunctuation( void )
{
	const char s_srcItems[] =
		"1254 Biertan{DUP}.jpg|"
		"1254 Biertan-DUP.jpg|"
		"1254 Biertan~DUP.jpg|"
		"1254 biertan[DUP].jpg|"
		"1254 biertan_DUP.jpg|"
		"1254 Biertan(DUP).jpg|"
		"1254 Biertan+DUP.jpg|"
		"1254 Biertan_noDUP.jpg|"
		"1254 Biertan.jpg";

	// intuitive: case-insensitive, numbers by value, default punctuation order
	{
		std::vector<std::string> items;
		str::Split( items, s_srcItems, "|" );

		ASSERT_EQUAL(
			"1254 Biertan(DUP).jpg|"
			"1254 Biertan+DUP.jpg|"
			"1254 Biertan-DUP.jpg|"
			"1254 Biertan.jpg|"
			"1254 biertan[DUP].jpg|"
			"1254 biertan_DUP.jpg|"
			"1254 Biertan_noDUP.jpg|"
			"1254 Biertan{DUP}.jpg|"
			"1254 Biertan~DUP.jpg"
			, ut::ShuffleSortJoin( items, "|", pred::LessValue<pred::CompareValue>() ) );
	}

	{
		std::vector<std::wstring> items;
		str::Split( items, str::FromUtf8( s_srcItems ).c_str(), L"|" );

		ASSERT_EQUAL(
			L"1254 Biertan(DUP).jpg|"
			L"1254 Biertan+DUP.jpg|"
			L"1254 Biertan-DUP.jpg|"
			L"1254 Biertan.jpg|"
			L"1254 biertan[DUP].jpg|"
			L"1254 biertan_DUP.jpg|"
			L"1254 Biertan_noDUP.jpg|"
			L"1254 Biertan{DUP}.jpg|"
			L"1254 Biertan~DUP.jpg"
			, ut::ShuffleSortJoin( items, L"|", pred::LessValue<pred::CompareValue>() ) );
	}
}


void CStringCompareTests::Run( void )
{
	RUN_TEST( TestIgnoreCase );
	RUN_TEST( TestStringCompare );
	RUN_TEST( TestStringCompare2 );
	RUN_TEST( TestStringFind );
	RUN_TEST( TestStringFindSequence );
	RUN_TEST( TestStringOccurenceCount );
	RUN_TEST( TestStringMatch );

	RUN_TEST( TestStringSorting );
	RUN_TEST( TestIntuitiveSort );
	RUN_TEST( TestIntuitiveSortPunctuation );
}


#endif //USE_UT
