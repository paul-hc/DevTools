#ifndef RenameFilesTests_h
#define RenameFilesTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CRenameFilesTests : public ut::CConsoleTestCase
{
	CRenameFilesTests( void );
public:
	static CRenameFilesTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestRenameSimple( void );
	void TestRenameCollisionExisting( void );
	void TestRenameChangeCase( void );
};


#endif //USE_UT


#endif // RenameFilesTests_h
