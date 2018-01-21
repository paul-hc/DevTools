#ifndef LcsTests_h
#define LcsTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


class CLcsTests : public ut::ITestCase
{
	CLcsTests( void );
public:
	static CLcsTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestCompareSimple( void );
	void TestCompareDiffCase( void );
	void TestCompareDiffOverlap( void );
};


#endif //_DEBUG


#endif // LcsTests_h
