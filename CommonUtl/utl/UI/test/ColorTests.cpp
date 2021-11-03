
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/ColorTests.h"
#include "StringUtilities.h"
#include "Color.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CColorTests::CColorTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CColorTests& CColorTests::Instance( void )
{
	static CColorTests s_testCase;
	return s_testCase;
}

void CColorTests::TestColor( void )
{
	ASSERT_EQUAL( 255, ui::GetLuminance( 255, 255, 255 ) );
	ASSERT_EQUAL( 254, ui::GetLuminance( 255, 255, 254 ) );
	ASSERT_EQUAL( 254, ui::GetLuminance( 254, 255, 255 ) );
	ASSERT_EQUAL( 254, ui::GetLuminance( 254, 254, 254 ) );
}

void CColorTests::TestColorChannels( void )
{
}

void CColorTests::TestColorTransform( void )
{
}

void CColorTests::TestFormatParseColor( void )
{
}


void CColorTests::Run( void )
{
	__super::Run();

	TestColor();
	TestColorChannels();
	TestColorTransform();
	TestFormatParseColor();
}


#endif //USE_UT
