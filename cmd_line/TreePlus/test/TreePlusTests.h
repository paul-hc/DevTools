#ifndef TreePlusTests_h
#define TreePlusTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CTextCell;


class CTreePlusTests : public ut::CConsoleTestCase
{
	CTreePlusTests( void );
public:
	static CTreePlusTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void testTable( const CTextCell* pTableRoot );

	// unit tests
	void TestOnlyDirectories( void );
	void TestFilesAndDirectories( void );
	void TestTableInput( void );
};


#endif //USE_UT


#endif // TreePlusTests_h
