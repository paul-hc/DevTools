#ifndef StructuredStorageTest_h
#define StructuredStorageTest_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


class CStructuredStorageTest : public ut::CConsoleTestCase
{
	CStructuredStorageTest( void );
public:
	static CStructuredStorageTest& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestLongFilenames( void );
	void TestStructuredStorage( void );
};


#endif //_DEBUG


#endif // StructuredStorageTest_h
