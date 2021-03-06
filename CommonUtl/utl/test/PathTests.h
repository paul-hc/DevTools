#ifndef PathTests_h
#define PathTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


class CPathTests : public ut::CConsoleTestCase
{
	CPathTests( void );
public:
	static CPathTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestPathBasics( void );
	void TestPathIs( void );
	void TestPathUtilities( void );
	void TestPathSort( void );
	void TestPathSortExisting( void );
	void TestPathNaturalSort( void );
	void TestPathCompareFind( void );
	void TestPathWildcardMatch( void );
	void TestHasMultipleDirPaths( void );
	void TestCommonSubpath( void );
	void TestComplexPath( void );
	void TestFlexPath( void );
	void TestPathHashValue( void );
};


#endif //_DEBUG


#endif // PathTests_h
