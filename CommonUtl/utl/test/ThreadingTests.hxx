#ifndef ThreadingTests_hxx
#define ThreadingTests_hxx
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"

#include "MultiThreading.h"
#include "StdThread.h"


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
		RUN_TEST( TestNestedLocking );
	}
private:
	void TestNestedLocking( void )			// OS sync objects
	{
		// code demo - not a real unit test

		{	// MFC locking:
			CCriticalSection cs;			// serialize cache access for thread safety

			mt::CAutoLock lock1( &cs );
			TRACE( _T(" CCriticalSection: aquire 1st lock...") );
			{
				mt::CAutoLock lock2( &cs );
				TRACE( _T(" aquired nested lock!\n") );
			}
		}

		{	// STL locking:
			std::mutex mtx;
			std::lock_guard<std::mutex> lock1( mtx );
			TRACE( _T(" std::mutex: aquire lock\n") );
		}
	}
};


#endif //USE_UT


#endif // ThreadingTests_hxx
