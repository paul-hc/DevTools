#ifndef FmtUtilsTests_h
#define FmtUtilsTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "ut/UnitTest.h"
#include "FileState.h"


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


#endif //_DEBUG


#endif // FmtUtilsTests_h
