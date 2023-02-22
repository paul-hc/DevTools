#ifndef CppCodeTests_h
#define CppCodeTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CCppCodeTests : public ut::CConsoleTestCase
{
	CCppCodeTests( void );
public:
	static CCppCodeTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestIterationSlices( void );
};


#endif //USE_UT


#endif // CppCodeTests_h
