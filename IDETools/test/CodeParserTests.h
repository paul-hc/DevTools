#ifndef CodeParserTests_h
#define CodeParserTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CCodeParserTests : public ut::CConsoleTestCase
{
	CCodeParserTests( void );
public:
	static CCodeParserTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestBraceParityStatus( void );
	void TestMethodComponents( void );
};


#endif //USE_UT


#endif // CodeParserTests_h
