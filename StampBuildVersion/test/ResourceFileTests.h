#ifndef ResourceFileTests_h
#define ResourceFileTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


namespace utl { class CProcessCmd; }


// functional tests that invoke child xfer.exe processes in order to exercise the command line parsing (pretty complex functionality)
//
class CResourceFileTests : public ut::CConsoleTestCase
{
	CResourceFileTests( void );
public:
	static CResourceFileTests& Instance( void );

	int ExecuteProcess( utl::CProcessCmd& rProcess );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	// unit tests
	void TestStampRcFile( void );

	// functional unit tests (executes the command line process)
	void FuncTest_StampRcFile( void );

	// impl
	void testEach_StampRcFile( const TCHAR* pRcFilePath );
	void testRcFile_CurrentTimestamp( const std::string& newText, const CTime& baselineTimestamp );
	void testRcFile_RefTimestamp( const std::string& newText );
private:
	static const TCHAR s_rcFile[];
	static const std::string s_refTimestamp;
	static const char s_doubleQuote[];
};


#endif //USE_UT


#endif // ResourceFileTests_h
