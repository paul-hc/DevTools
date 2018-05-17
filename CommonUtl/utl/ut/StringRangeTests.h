#ifndef StringRangeTests_h
#define StringRangeTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "ut/UnitTest.h"


class CStringRangeTests : public ut::CConsoleTestCase
{
	CStringRangeTests( void );
public:
	static CStringRangeTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestInit( void );
	void TestTrim( void );
	void TestStrip( void );
	void TestFind( void );
};


#endif //_DEBUG


#endif // StringRangeTests_h
