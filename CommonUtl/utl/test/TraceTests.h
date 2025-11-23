#ifndef TraceTests_h
#define TraceTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"


class CTraceTests : public ut::CConsoleTestCase
{
public:
	CTraceTests( void );
public:
	static CTraceTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestTracing( void );
};


#endif //USE_UT


#endif // TraceTests_h
