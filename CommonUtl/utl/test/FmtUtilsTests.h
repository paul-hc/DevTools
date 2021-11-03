#ifndef FmtUtilsTests_h
#define FmtUtilsTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"
#include "utl/FileState.h"


class CFmtUtilsTests : public ut::CConsoleTestCase
{
	CFmtUtilsTests( void );
public:
	static CFmtUtilsTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestFileState( void );
	void TestTouchEntry( void );
};


#endif //USE_UT


#endif // FmtUtilsTests_h
