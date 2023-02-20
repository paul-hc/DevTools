
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
	RUN_TEST( TestStringSorting );
	RUN_TEST( TestIntuitiveSort );
	RUN_TEST( TestIntuitiveSortPunctuation );
}


#endif //USE_UT
