#ifndef TestCase_h
#define TestCase_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/UnitTest.h"


class CTestCase : public ut::CConsoleTestCase
{
	CTestCase( void );
public:
	static CTestCase& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestPullLossy( void );
};


#endif //_DEBUG


#endif // TestCase_h
