#ifndef ResourceFileTests_h
#define ResourceFileTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


// functional tests that invoke child xfer.exe processes in order to exercise the command line parsing (pretty complex functionality)
//
class CResourceFileTests : public ut::CConsoleTestCase
{
	CResourceFileTests( void );
public:
	static CResourceFileTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	// unit tests
	void TestStampRcFile( void );

	// functional unit tests (executes the command line process)
	void FuncTest_StampRcFile( void );
};


#endif //USE_UT


#endif // ResourceFileTests_h
