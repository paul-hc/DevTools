#ifndef Test_h
#define Test_h
#pragma once


#ifdef USE_UT		// no UT code in release builds


#define RUN_TEST( testMethod )\
	do { ut::CScopedTestMethod test( this, #testMethod ); (testMethod)(); } while ( false )

#define RUN_CONDITIONAL_TEST( condTestMethod )\
	do { ut::CScopedTestMethod test( this, #condTestMethod ); test.SetSkipped( !(condTestMethod)() ); } while ( false )

#define RUN_TEST1( testMethod, arg1 )\
	do { ut::CScopedTestMethod test( this, #testMethod ); (testMethod)( (arg1) ); } while ( false )

#define RUN_TEST2( testMethod, arg1, arg2 )\
	do { ut::CScopedTestMethod test( this, #testMethod ); (testMethod)( (arg1), (arg2) ); } while ( false )


namespace ut
{
	interface ITestCase
	{
		virtual void Run( void ) = 0;
	};

	abstract class CConsoleTestCase : public ITestCase
	{
	};

	abstract class CGraphicTestCase : public ITestCase
	{
	};


	class CTestSuite
	{
		CTestSuite( void );
		~CTestSuite();
	public:
		static CTestSuite& Instance( void );

		void RunTests( void );
		void ClearTests( void );

		bool IsEmpty( void ) const { return m_testCases.empty(); }
		bool RegisterTestCase( ut::ITestCase* pTestCase );

		void QueryTestNames( std::vector<std::tstring>& rTestNames ) const;
	private:
		std::vector<ITestCase*> m_testCases;
	};

	void RunAllTests( void );				// main entry point for running all unit tests
}


namespace ut
{
	class CScopedTestMethod
	{
	public:
		CScopedTestMethod( const ITestCase* pTestCase, const char* pTestMethod );
		~CScopedTestMethod();

		void SetSkipped( bool skipped )
		{
			m_skipped = skipped;

			if ( m_skipped )
				++s_skippedTestCount;
		}

		static size_t GetTestCount( void ) { return s_testCount; }
		static size_t GetFailedTestCount( void ) { return s_failedTestCount; }
		static size_t GetPassedTestCount( void ) { return s_testCount - s_failedTestCount; }
		static size_t GetSkippedTestCount( void ) { return s_skippedTestCount; }
	private:
		int m_oldErrorCount;
		bool m_skipped;						// controls test result for conditional tests

		static size_t s_testCount;			// total test methods executed
		static size_t s_failedTestCount;	// failed test methods
		static size_t s_skippedTestCount;	// skipped conditional test methods
	};
}


#endif //USE_UT


#endif // Test_h
