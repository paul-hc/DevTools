#ifndef CodeParserExTests_h
#define CodeParserExTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CCodeParserExTests : public ut::CConsoleTestCase
{
	CCodeParserExTests( void );
public:
	static CCodeParserExTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestBraceParityStatus( void );
	void TestMethodComponents( void );
};


#endif //USE_UT


#endif // CodeParserExTests_h
