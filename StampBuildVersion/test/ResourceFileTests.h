#ifndef ResourceFileTests_h
#define ResourceFileTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/test/UnitTest.h"


namespace utl { class CProcessCmd; }


// functional tests that invoke child xfer.exe processes in order to exercise the command line parsing (pretty complex functionality)
//
class CResourceFileTests : public ut::CConsoleTestCase
{
	CResourceFileTests( void );
public:
	static CResourceFileTests& Instance( void );

	void SetDebugChildProcs( bool debugChildProcs ) { m_debugChildProcs = debugChildProcs; }
	int ExecuteProcess( utl::CProcessCmd& rProcess );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	// unit tests
	void TestStampRcFile( void );

	// functional unit tests (executes the command line process)
	void FuncTest_StampRcFile( void );

	// impl
	void testRcFile_CurrentTimestamp( const std::string& newText, const CTime& baselineTimestamp );
	void testRcFile_RefTimestamp( const std::string& newText );
private:
	bool m_debugChildProcs;

	static const TCHAR s_rcFile[];
	static const std::string s_refTimestamp;
	static const char s_doubleQuote[];
};


#endif //_DEBUG


#endif // ResourceFileTests_h
