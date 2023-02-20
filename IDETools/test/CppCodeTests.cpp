
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "utl/StringUtilities.h"
#include "test/CppCodeTests.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CCppCodeTests::CCppCodeTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CCppCodeTests& CCppCodeTests::Instance( void )
{
	static CCppCodeTests s_testCase;
	return s_testCase;
}

void CCppCodeTests::Test( void )
{
}


void CCppCodeTests::Run( void )
{
	RUN_TEST( Test );
}


#endif //USE_UT
