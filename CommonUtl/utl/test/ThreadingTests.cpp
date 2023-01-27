
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/ThreadingTests.h"

#if defined(_HAS_CXX17) && !defined(_M_X64)
	#pragma message( __WARN__ "Skipping CThreadingTests - 32-bit Boost libraries are missing for Visual C++ 2022!" )
#else
	#define TEST_BOOST_THREADS		// Boost 32-bit libraries are not available
#endif

#ifdef TEST_BOOST_THREADS
#include "MultiThreading.h"
#include <boost/thread.hpp>
#endif //TEST_BOOST_THREADS

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CThreadingTests::CThreadingTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CThreadingTests& CThreadingTests::Instance( void )
{
	static CThreadingTests s_testCase;
	return s_testCase;
}

void CThreadingTests::TestNestedLocking( void )
{
#ifdef TEST_BOOST_THREADS
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
		boost::lock_guard<boost::mutex> lock1( mtx );
		TRACE( _T(" boost::mutex: aquire lock\n") );
	}
#endif //TEST_BOOST_THREADS
}

void CThreadingTests::Run( void )
{
	__super::Run();

	TestNestedLocking();
}


#endif //USE_UT
