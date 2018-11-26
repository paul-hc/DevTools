
#include "stdafx.h"
#include "ut/ColorTests.h"
#include "StringUtilities.h"
#include "Color.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


CColorTests::CColorTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CColorTests& CColorTests::Instance( void )
{
	static CColorTests testCase;
	return testCase;
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


#endif //_DEBUG
