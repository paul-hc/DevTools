#ifndef PathTests_h
#define PathTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "ut/UnitTest.h"


class CPathTests : public ut::CConsoleTestCase
{
	CPathTests( void );
public:
	static CPathTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestPathUtilities( void );
	void TestPathCompareFind( void );
	void TestPathWildcardMatch( void );
	void TestComplexPath( void );
	void TestFlexPath( void );
};


#endif //_DEBUG


#endif // PathTests_h
