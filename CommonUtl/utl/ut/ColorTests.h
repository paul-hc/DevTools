#ifndef ColorTests_h
#define ColorTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "ut/UnitTest.h"


class CColorTests : public ut::CConsoleTestCase
{
	CColorTests( void );
public:
	static CColorTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestColor( void );
	void TestColorChannels( void );
	void TestColorTransform( void );
	void TestFormatParseColor( void );
};


#endif //_DEBUG


#endif // ColorTests_h
