
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/Test.h"
#include "AppTools.h"
#include "Algorithms.h"
#include "FileSystem.h"
#include "Logger.h"
#include "Path.h"
#include "RuntimeException.h"
#include "RandomUtilities.h"
#include "StringUtilities.h"
#include "Timer.h"
#include <math.h>
#include <fstream>
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	class CTestRunnerReport
	{
	public:
		CTestRunnerReport( void );
		~CTestRunnerReport();
	private:
		CTimer m_timer;
	};


	void RunAllTests( void )				// main entry point for running all unit tests
	{
		CTestRunnerReport report;			// scoped output

		utl::SetRandomSeed();				// ensure randomness on std::random_shuffle calls
		CTestSuite::Instance().RunTests();
	}


	// CTestSuite implementation

	CTestSuite::CTestSuite( void )
	{
	}

	CTestSuite::~CTestSuite()
	{
		ClearTests();
	}

	CTestSuite& CTestSuite::Instance( void )
	{
		static CTestSuite s_singleton;
		return s_singleton;
	}

	void CTestSuite::ClearTests( void )
	{
		m_testCases.clear();
	}

	void CTestSuite::RunTests( void )
	{
		ASSERT( !IsEmpty() );
		for ( std::vector<ITestCase*>::const_iterator itTestCase = m_testCases.begin(); itTestCase != m_testCases.end(); ++itTestCase )
			( *itTestCase )->Run();
	}

	bool CTestSuite::RegisterTestCase( ut::ITestCase* pTestCase )
	{
		ASSERT_PTR( pTestCase );
		utl::PushUnique( m_testCases, pTestCase );
		return true;
	}

	void CTestSuite::QueryTestNames( std::vector<std::tstring>& rTestNames ) const
	{
		rTestNames.reserve( rTestNames.size() + m_testCases.size() );

		for ( std::vector<ITestCase*>::const_iterator itTestCase = m_testCases.begin(); itTestCase != m_testCases.end(); ++itTestCase )
			rTestNames.push_back( str::GetTypeName( typeid( **itTestCase ) ) );
	}

} //namespace ut


namespace ut
{
	std::tstring FormatTestMethod( const ITestCase* pTestCase, const char* pTestMethod, bool qualifyGraphicTest )
	{
		ASSERT_PTR( pTestCase );

		const char* pTestCaseName = str::GetTypeNamePtr( *pTestCase );

		std::tostringstream oss;
		oss << pTestCaseName << "::" << pTestMethod;

		if ( qualifyGraphicTest && is_a<CGraphicTestCase>( pTestCase ) )
			oss << " (graphic) : ";

		return oss.str();
	}
}


namespace ut
{
	// CTestRunnerReport implementation

	CTestRunnerReport::CTestRunnerReport()
	{
		std::ostringstream os;
		os << "RUNNING UNIT TESTS:   " << CAppTools::Instance()->GetModulePath().GetFilenamePtr() << std::endl;

		std::string text = os.str();

		std::clog << text;
		OutputDebugStringA( text.c_str() );
	}

	CTestRunnerReport::~CTestRunnerReport()
	{
		std::ostringstream os;

		if ( 0 == CScopedTestMethod::GetFailedTestCount() )
			os << "OK (" << CScopedTestMethod::GetTestCount() << ')';
		else
			os << "FAILED (" << CScopedTestMethod::GetFailedTestCount() << "),  PASSED (" << CScopedTestMethod::GetPassedTestCount() << ')';

		if ( CScopedTestMethod::GetSkippedTestCount() != 0 )
			os << ",  SKIPPED (" << CScopedTestMethod::GetSkippedTestCount() << ')';

		os << _T("   Elapsed: ") << m_timer.FormatElapsedSeconds() << std::endl;

		std::string text = os.str();

		std::clog << text;
		OutputDebugStringA( text.c_str() );
	}


	// CScopedTestMethod implementation

	size_t CScopedTestMethod::s_testCount = 0;
	size_t CScopedTestMethod::s_failedTestCount = 0;
	size_t CScopedTestMethod::s_skippedTestCount = 0;

	CScopedTestMethod::CScopedTestMethod( const ITestCase* pTestCase, const char* pTestMethod )
		: m_oldErrorCount( CAppTools::GetMainResultCode() )
		, m_skipped( false )
	{
		m_testMethod = ut::FormatTestMethod( pTestCase, pTestMethod, false );

		{
			std::tstring qualifTestName = ut::FormatTestMethod( pTestCase, pTestMethod, true );

			OutputDebugString( qualifTestName.c_str() );		// don't use TRACE to avoid the ATL overhead
			std::clog << qualifTestName;
		}

		++s_testCount;
	}

	CScopedTestMethod::~CScopedTestMethod()
	{
		std::string text = m_skipped ? "SKIPPED!\n" : "OK\n";

		if ( int failedCount = CAppTools::GetMainResultCode() - m_oldErrorCount )
		{
			std::ostringstream os;
			os << failedCount << ( 1 == failedCount ? " ERROR!" : " ERRORS!" ) << std::endl;
			text = os.str();

			std::cerr << text;

			++s_failedTestCount;
		}
		else
			std::clog << text;

		OutputDebugStringA( text.c_str() );
	}
}


#endif //USE_UT
