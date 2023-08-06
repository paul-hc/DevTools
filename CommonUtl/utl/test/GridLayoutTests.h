#ifndef GridLayoutTests_h
#define GridLayoutTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"


class CGridLayoutTests : public ut::CConsoleTestCase
{
	CGridLayoutTests( void );
public:
	static CGridLayoutTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestGridLayout( void );
	void TestTransposeGridLayout( void );
};


#endif //USE_UT


#endif // GridLayoutTests_h
