
#include "stdafx.h"
#include "ut/ThreadingTests.h"
#include "MfcUtilities.h"
#include <boost/thread.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


CThreadingTests::CThreadingTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CThreadingTests& CThreadingTests::Instance( void )
{
	static CThreadingTests testCase;
	return testCase;
}

void CThreadingTests::TestNestedLocking( void )
{
	// code demo - not a real unit test
	{
		CCriticalSection cs;				// serialize cache access for thread safety

		mt::CAutoLock lock1( &cs );
		TRACE( _T(" CCriticalSection: aquire 1st lock...") );
		{
			mt::CAutoLock lock2( &cs );
			TRACE( _T(" aquired nested lock!\n") );
		}
	}

	{
		boost::mutex mtx;
		boost::lock_guard< boost::mutex > lock1( mtx );
		TRACE( _T(" boost::mutex: aquire lock\n") );
	}
}

void CThreadingTests::Run( void )
{
	__super::Run();

	TestNestedLocking();
}


#endif //_DEBUG
