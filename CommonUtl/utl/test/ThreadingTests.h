#ifndef ThreadingTests_h
#define ThreadingTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"


class CThreadingTests : public ut::CConsoleTestCase
{
	CThreadingTests( void );
public:
	static CThreadingTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestNestedLocking( void );			// OS sync objects
};


#endif //USE_UT


#endif // ThreadingTests_h
