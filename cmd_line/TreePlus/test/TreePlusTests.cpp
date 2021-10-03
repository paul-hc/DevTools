
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds

#include "TreePlusTests.h"
#include "utl/AppTools.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTreePlusTests::CTreePlusTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CTreePlusTests& CTreePlusTests::Instance( void )
{
	static CTreePlusTests s_testCase;
	return s_testCase;
}

void CTreePlusTests::Test( void )
{
}


void CTreePlusTests::Run( void )
{
	__super::Run();

	Test();
}


#endif //USE_UT
