#ifndef TransferFuncTests_h
#define TransferFuncTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/test/UnitTest.h"


namespace utl { class CProcessCmd; }


// functional tests that invoke child xfer.exe processes in order to exercise the command line parsing (pretty complex functionality)
//
class CTransferFuncTests : public ut::CConsoleTestCase
{
	CTransferFuncTests( void );
public:
	static CTransferFuncTests& Instance( void );

	void SetDebugChildProcs( bool debugChildProcs ) { m_debugChildProcs = debugChildProcs; }
	int ExecuteProcess( utl::CProcessCmd& rProcess );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestCopy( void );
	void TestMove( void );
	void TestBackup( void );
	void TestPullLossy( void );
private:
	bool m_debugChildProcs;

	static const TCHAR s_srcFiles[];
	static const fs::CPath s_lossyFilter;
};


#endif //_DEBUG


#endif // TransferFuncTests_h
