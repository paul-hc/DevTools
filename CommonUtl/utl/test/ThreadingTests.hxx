#ifndef ThreadingTests_hxx
#define ThreadingTests_hxx
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"

#if defined(_HAS_CXX17) && !defined(_M_X64)
	#pragma message( __WARN__ "Skipping CThreadingTests - 32-bit Boost libraries are missing for Visual C++ 2022!" )
#else
	#define TEST_BOOST_THREADS		// Boost 32-bit libraries are not available
#endif

#ifdef TEST_BOOST_THREADS
#include "MultiThreading.h"
#include <boost/thread.hpp>
#endif //TEST_BOOST_THREADS


class CThreadingTests : public ut::CConsoleTestCase
{
	CThreadingTests( void )
	{
		ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
	}
public:
	static CThreadingTests& Instance( void )
	{
		static CThreadingTests s_testCase;
		return s_testCase;
	}

	// ut::ITestCase interface
	virtual void Run( void )
	{
		__super::Run();

		TestNestedLocking();
	}
private:
	void TestNestedLocking( void )			// OS sync objects
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
};


#endif //USE_UT


#endif // ThreadingTests_hxx
