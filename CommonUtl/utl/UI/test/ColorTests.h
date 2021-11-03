#ifndef ColorTests_h
#define ColorTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "test/UnitTest.h"


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


#endif //USE_UT


#endif // ColorTests_h
