#ifndef Test_h
#define Test_h
#pragma once


#ifdef USE_UT		// no UT code in release builds


namespace ut
{
	interface ITestCase
	{
		virtual void Run( void ) = 0;
	};

	abstract class CConsoleTestCase : public ITestCase
	{
	public:
		// base overrides
		virtual void Run( void ) = 0;		// pure with implementation; must be called for tracing execution
	};

	abstract class CGraphicTestCase : public ITestCase
	{
	public:
		// base overrides
		virtual void Run( void ) = 0;		// pure with implementation; must be called for tracing execution
	};


	class CTestSuite
	{
		~CTestSuite();
	public:
		static CTestSuite& Instance( void );

		void RunTests( void );

		bool IsEmpty( void ) const { return m_testCases.empty(); }
		bool RegisterTestCase( ut::ITestCase* pTestCase );

		void QueryTestNames( std::vector< std::tstring >& rTestNames ) const;
	private:
		std::vector< ITestCase* > m_testCases;
	};

	void RunAllTests( void );				// main entry point for running all unit tests
}


#endif //USE_UT


#endif // Test_h
