#ifndef StringRangeTests_h
#define StringRangeTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"


class CStringRangeTests : public ut::CConsoleTestCase
{
	CStringRangeTests( void );
public:
	static CStringRangeTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestInit( void );
	void TestFind( void );
	void TestTrim( void );
	void TestStrip( void );
	void TestSplit( void );
};


#endif //USE_UT


#endif // StringRangeTests_h
