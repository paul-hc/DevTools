#ifndef MethodPrototypeTests_h
#define MethodPrototypeTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CMethodPrototypeTests : public ut::CConsoleTestCase
{
	CMethodPrototypeTests( void );
public:
	static CMethodPrototypeTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestParse_GlobalFunction( void );
	void TestParse_ClassMethodImpl( void );
	void TestParse_TemplateMethodImpl( void );
};


#endif //USE_UT


#endif // MethodPrototypeTests_h
