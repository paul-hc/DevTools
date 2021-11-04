#ifndef DuplicateFilesTests_h
#define DuplicateFilesTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"


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


#endif // DuplicateFilesTests_h
