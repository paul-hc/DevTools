#ifndef PathAlgorithmsTests_h
#define PathAlgorithmsTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/ut/UnitTest.h"


class CPathAlgorithmsTests : public ut::CConsoleTestCase
{
	CPathAlgorithmsTests( void );
public:
	static CPathAlgorithmsTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestMakeCase( void );
	void TestCapitalizeWords( void );
	void TestReplaceText( void );
	void TestWhitespace( void );
};


#endif //_DEBUG


#endif // PathAlgorithmsTests_h
