#ifndef TreePlusTests_h
#define TreePlusTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CTreePlusTests : public ut::CConsoleTestCase
{
	CTreePlusTests( void );
public:
	static CTreePlusTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	// unit tests
	void Test( void );
};


#endif //USE_UT


#endif // TreePlusTests_h
