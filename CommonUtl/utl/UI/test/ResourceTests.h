#ifndef ResourceTests_h
#define ResourceTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "test/UnitTest.h"


class CResourceTests : public ut::CConsoleTestCase
{
	CResourceTests( void );
public:
	static CResourceTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestDeepPopupIndex( void );
	void TestLoadPopupMenu( void );
};


#endif //USE_UT


#endif // ResourceTests_h
