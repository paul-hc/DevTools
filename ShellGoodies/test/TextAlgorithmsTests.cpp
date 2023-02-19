
#include "pch.h"
#include "GeneralOptions.h"
#include "utl/ScopedValue.h"

#ifdef USE_UT		// no UT code in release builds
#include "TextAlgorithmsTests.h"
#include "TextAlgorithms.h"

#define new DEBUG_NEW


namespace ut
{
	template< typename Func >
	std::tstring Transform( const std::tstring& filePath, const Func& func )
	{
		fs::CPathParts parts( filePath );
		func( parts );
		return parts.m_fname + parts.m_ext;
	}
}


CTextAlgorithmsTests::CTextAlgorithmsTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CTextAlgorithmsTests& CTextAlgorithmsTests::Instance( void )
{
	static CTextAlgorithmsTests s_testCase;
	return s_testCase;
}

void CTextAlgorithmsTests::TestMakeCase( void )
{
	ASSERT_EQUAL( _T("my file.txt"), ut::Transform( _T("  My File  .tXT"), func::MakeCase( LowerCase ) ) );
	ASSERT_EQUAL( _T("MY FILE.TXT"), ut::Transform( _T("My File.tXT"), func::MakeCase( UpperCase ) ) );
	ASSERT_EQUAL( _T("my file.tXT"), ut::Transform( _T("My File.tXT"), func::MakeCase( FnameLowerCase ) ) );
	ASSERT_EQUAL( _T("MY FILE.tXT"), ut::Transform( _T("My File.tXT"), func::MakeCase( FnameUpperCase ) ) );
	ASSERT_EQUAL( _T("My File.txt"), ut::Transform( _T("My File.tXT"), func::MakeCase( ExtLowerCase ) ) );
	ASSERT_EQUAL( _T("My File.TXT"), ut::Transform( _T("My File.tXT"), func::MakeCase( ExtUpperCase ) ) );
	ASSERT_EQUAL( _T("My File"), ut::Transform( _T("My File.tXT"), func::MakeCase( NoExt ) ) );
}

void CTextAlgorithmsTests::TestCapitalizeWords( void )
{
	static const CCapitalizeOptions options;
	const CTitleCapitalizer capitalizer( &options );

	ASSERT_EQUAL( _T("Of This and of That.txt"), ut::Transform( _T("  of this and of that .TXT"), func::CapitalizeWords( &capitalizer ) ) );
	ASSERT_EQUAL( _T("At McDonald's vs O'Connor w. ABBA feat. AC-DC in 3D-Studio.txt"), ut::Transform( _T("at mcdonald's VS o'connor W. abba FEAT. ac-dc IN 3d-studio.TXT"), func::CapitalizeWords( &capitalizer ) ) );

	ASSERT_EQUAL( _T("W,w,with,feat,featuring,aka,vs.txt"), ut::Transform( _T(" w,W,WITH,FEAT,FEATURING,AKA,VS .TXT"), func::CapitalizeWords( &capitalizer ) ) );
	ASSERT_EQUAL( _T("This Is(Some)[Version]{Upon}.txt"), ut::Transform( _T(" this\tis(some)[version]{upon} .TXT"), func::CapitalizeWords( &capitalizer ) ) );
}

void CTextAlgorithmsTests::TestReplaceText( void )
{
	ASSERT_EQUAL( _T("of--this--and--of that.txt"), ut::Transform( _T("of_this.and-of   that.TXT"), func::ReplaceDelimiterSet( _T("-._"), _T("--") ) ) );

	ASSERT_EQUAL( _T("For the CITY, of the People, by the PEOPLE.TXT"), ut::Transform( _T("For the people, of the People, by the PEOPLE.TXT"), func::ReplaceText( _T("people"), _T("CITY"), true ) ) );
	ASSERT_EQUAL( _T("For the CITY, of the CITY, by the CITY.TXT"), ut::Transform( _T("For the people, of the People, by the PEOPLE.TXT"), func::ReplaceText( _T("people"), _T("CITY"), false ) ) );

	ASSERT_EQUAL( _T("of--this--and--of that.TXT"), ut::Transform( _T("of_this.and-of   that.TXT"), func::ReplaceCharacters( _T("-._"), _T("--"), true ) ) );
	ASSERT_EQUAL( _T("Ano_her brillian_ con_ribu_ion _o _his endless deba_e_.txt"), ut::Transform( _T("Another brilliant contribution to this endless debate!.txt"), func::ReplaceCharacters( _T("tL!"), _T("_"), true ) ) );
	ASSERT_EQUAL( _T("Ano_her bri__ian_ con_ribu_ion _o _his end_ess deba_e_.txt"), ut::Transform( _T("Another brilliant contribution to this endless debate!.txt"), func::ReplaceCharacters( _T("tL!"), _T("_"), false ) ) );
}

void CTextAlgorithmsTests::TestWhitespace( void )
{
	ASSERT_EQUAL( _T("For the people, of the people.txt"), ut::Transform( _T("  For   the    people,    of\tthe    people  .TXT"), func::SingleWhitespace() ) );
	ASSERT_EQUAL( _T("ForThePeople,OfThePeople.txt"), ut::Transform( _T("  For   The    People,    Of\tThe    People .TXT"), func::RemoveWhitespace() ) );
}


void CTextAlgorithmsTests::Run( void )
{
	// temporary store the filename adjustments defaults
	CGeneralOptions* pOptions = &CGeneralOptions::Instance();
	CScopedValue< bool > scopedTrimFname( &pOptions->m_trimFname, true );
	CScopedValue< bool > scopedNormalizeWhitespace( &pOptions->m_normalizeWhitespace, true );

	RUN_TEST( TestMakeCase );
	RUN_TEST( TestCapitalizeWords );
	RUN_TEST( TestReplaceText );
	RUN_TEST( TestWhitespace );
}


#endif //USE_UT
