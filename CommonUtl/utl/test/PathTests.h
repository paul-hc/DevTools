#ifndef PathTests_h
#define PathTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

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
	void TestPathConversion( void );
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

	void TestPathHashing( void );
	void TestPathEqualTo( void );
	void TestPathSet( void );
	void TestPathHashValue( void );
};


#endif //USE_UT


#endif // PathTests_h
