#ifndef DuplicateFilesTests2_h
#define DuplicateFilesTests2_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CDuplicateFilesTests : public ut::CConsoleTestCase
{
	CDuplicateFilesTests( void );
public:
	static CDuplicateFilesTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestDuplicateFiles( void );
};


#endif //USE_UT


#endif // DuplicateFilesTests2_h
