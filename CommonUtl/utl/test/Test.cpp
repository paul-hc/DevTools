
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "test/Test.h"
#include "ContainerUtilities.h"
#include "FileSystem.h"
#include "Logger.h"
#include "Path.h"
#include "RuntimeException.h"
#include "RandomUtilities.h"
#include "StringUtilities.h"
#include <math.h>
#include <fstream>

#define new DEBUG_NEW


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

	void CTestSuite::QueryTestNames( std::vector< std::tstring >& rTestNames ) const
	{
		rTestNames.reserve( rTestNames.size() + m_testCases.size() );

		for ( std::vector< ITestCase* >::const_iterator itTestCase = m_testCases.begin(); itTestCase != m_testCases.end(); ++itTestCase )
			rTestNames.push_back( str::GetTypeName( typeid( **itTestCase ) ) );
	}

} //namespace ut


#endif //_DEBUG
