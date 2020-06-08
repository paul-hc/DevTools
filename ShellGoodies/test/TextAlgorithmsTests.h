#ifndef TextAlgorithmsTests_h
#define TextAlgorithmsTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CTextAlgorithmsTests : public ut::CConsoleTestCase
{
	CTextAlgorithmsTests( void );
public:
	static CTextAlgorithmsTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestMakeCase( void );
	void TestCapitalizeWords( void );
	void TestReplaceText( void );
	void TestWhitespace( void );
};


#endif //_DEBUG


#endif // TextAlgorithmsTests_h
