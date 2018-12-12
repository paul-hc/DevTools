
#include "stdafx.h"
#include "ut/Test.h"
#include "ContainerUtilities.h"
#include "FileSystem.h"
#include "Logger.h"
#include "Path.h"
#include "RuntimeException.h"
#include "RandomUtilities.h"
#include "StringUtilities.h"
#include <math.h>
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


namespace ut
{
	const std::string& GetTestCaseName( const ITestCase* pTestCase )
	{
		ASSERT_PTR( pTestCase );

		static std::string testName;
		testName = typeid( *pTestCase ).name();		// name of the derived concrete class (dereferenced pointer)
		str::StripPrefix( testName, "class " );
		return testName;
	}

	void CConsoleTestCase::Run( void )
	{
		TRACE( "-- %s console test case --\n", GetTestCaseName( this ).c_str() );
	}

	void CGraphicTestCase::Run( void )
	{
		TRACE( "-- %s graphic test case --\n", GetTestCaseName( this ).c_str() );
	}
}


namespace ut
{
	void RunAllTests( void )				// main entry point for running all unit tests
	{
		TRACE( "\nRUNNING UNIT TESTS:\n" );

		utl::SetRandomSeed();				// ensure randomness on std::random_shuffle calls
		CTestSuite::Instance().RunTests();

		TRACE( "\nEND UNIT TESTS\n" );
	}


	// CTestSuite implementation

	CTestSuite::~CTestSuite()
	{
	}

	CTestSuite& CTestSuite::Instance( void )
	{
		static CTestSuite singleton;
		return singleton;
	}

	void CTestSuite::RunTests( void )
	{
		ASSERT( !IsEmpty() );
		for ( std::vector< ITestCase* >::const_iterator itTestCase = m_testCases.begin(); itTestCase != m_testCases.end(); ++itTestCase )
			( *itTestCase )->Run();
	}

	bool CTestSuite::RegisterTestCase( ut::ITestCase* pTestCase )
	{
		ASSERT_PTR( pTestCase );
		utl::PushUnique( m_testCases, pTestCase );
		return true;
	}

} //namespace ut


#endif //_DEBUG
