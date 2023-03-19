#ifndef StringCompareTests_h
#define StringCompareTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"


class CStringCompareTests : public ut::CConsoleTestCase
{
	CStringCompareTests( void );
public:
	static CStringCompareTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestIgnoreCase( void );
	void TestStringCompare( void );
	void TestStringCompare2( void );
	void TestStringFind( void );
	void TestStringFindSequence( void );
	void TestStringOccurenceCount( void );
	void TestStringMatch( void );
	void TestStringSorting( void );
	void TestIntuitiveSort( void );
	void TestIntuitiveSortPunctuation( void );
};


#endif //USE_UT


#endif // StringCompareTests_h
