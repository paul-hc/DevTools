#ifndef LanguageTests_h
#define LanguageTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CLanguageTests : public ut::CConsoleTestCase
{
	CLanguageTests( void );
public:
	static CLanguageTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestMyLanguage( void );
	void TestMyLanguageSingleLine( void );
	void TestBracketParity( void );
	void TestBracketMismatch( void );
	void TestCodeDetails( void );

	void TestUntabify( void );
};


#endif //USE_UT


#endif // LanguageTests_h
