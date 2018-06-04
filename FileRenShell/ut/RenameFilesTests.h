#ifndef RenameFilesTests_h
#define RenameFilesTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/ut/UnitTest.h"


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
};


#endif //_DEBUG


#endif // RenameFilesTests_h
