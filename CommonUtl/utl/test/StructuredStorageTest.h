#ifndef StructuredStorageTest_h
#define StructuredStorageTest_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"
#include "utl/FlexPath.h"


namespace fs { class CStructuredStorage; }


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
	void _TestEnumerateElements( fs::CStructuredStorage* pDocStorage );
	void _TestFindElements( fs::CStructuredStorage* pDocStorage );
	void _TestOpenSharedStreams( fs::CStructuredStorage* pDocStorage, const fs::TEmbeddedPath& streamPath );
};


#endif //USE_UT


#endif // StructuredStorageTest_h
