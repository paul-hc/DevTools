#ifndef MethodPrototypeTests_h
#define MethodPrototypeTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


namespace code
{
	struct CMethodPrototype;
}


class CMethodPrototypeTests : public ut::CConsoleTestCase
{
	CMethodPrototypeTests( void );
public:
	static CMethodPrototypeTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestParse_GlobalFunction( code::CMethodPrototype& proto );
	void TestParse_ClassMethodImpl( void );
	void TestParse_TemplateMethodImpl( void );
	void TestImplementMethodBlock( void );
};


#endif //USE_UT


#endif // MethodPrototypeTests_h
