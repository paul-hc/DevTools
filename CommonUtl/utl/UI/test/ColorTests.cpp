
#include "pch.h"

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
	COLORREF color = CLR_NONE, colorValue = RGB( 255, 128, 64 );	// 0x004080FF, "#FF8040"

	// RGB colour
	ASSERT( !ui::ParseRgbColor( &color, _T("") ) );
	ASSERT( !ui::ParseRgbColor( &color, _T("text") ) );
	ASSERT( ui::ParseRgbColor( &color, _T(" RGB( 255, 128, 64 ) ") ) );
	ASSERT_EQUAL( colorValue, color );
	color = CLR_NONE;
	ASSERT( ui::ParseRgbColor( &color, _T("RGB(255,128,64)") ) );
	ASSERT_EQUAL( colorValue, color );
	ASSERT( !ui::ParseRgbColor( &color, _T("RGB(255,1289,64)") ) );			// unexpected G extra-digit

	// HTML colour
	ASSERT( !ui::ParseHtmlColor( &color, _T("") ) );
	ASSERT( !ui::ParseHtmlColor( &color, _T("text") ) );
	ASSERT( ui::ParseHtmlColor( &color, _T(" \t #FF8040 \n ") ) );
	ASSERT_EQUAL( colorValue, color );

	// System colour
	ASSERT( !ui::ParseSystemColor( &color, _T("") ) );
	ASSERT( !ui::ParseSystemColor( &color, _T("text") ) );
	ASSERT( ui::ParseSystemColor( &color, _T(" \t SYS(16)  \"Button Shadow\" \n ") ) );
	ASSERT_EQUAL( COLOR_BTNSHADOW, ui::GetSysColorIndex( color ) );
	ASSERT( ui::ParseSystemColor( &color, _T("SYS( 16 )  \"Button Shadow\"") ) );
	ASSERT_EQUAL( COLOR_BTNSHADOW, ui::GetSysColorIndex( color ) );
	ASSERT( !ui::ParseSystemColor( &color, _T("SYS( 48 )  \"?\"") ) );		// invalid system colour index
}


void CColorTests::Run( void )
{
	RUN_TEST( TestColor );
	RUN_TEST( TestColorChannels );
	RUN_TEST( TestColorTransform );
	RUN_TEST( TestFormatParseColor );
}


#endif //USE_UT
