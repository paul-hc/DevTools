#ifndef TransferFuncTests_h
#define TransferFuncTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/ut/UnitTest.h"


namespace utl { class CProcess; }


// functional tests that invoke child xfer.exe processes in order to exercise the command line parsing (pretty complex functionality)
//
class CTransferFuncTests : public ut::CConsoleTestCase
{
	CTransferFuncTests( void );
public:
	static CTransferFuncTests& Instance( void );

	void SetDebugChildProcs( bool debugChildProcs ) { m_debugChildProcs = debugChildProcs; }
	void AddExtraParams( utl::CProcess& rXferProcess );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestTransferCopy( void );
	void TestPullLossy( void );
private:
	bool m_debugChildProcs;
	static const TCHAR s_srcFiles[];
};


#endif //_DEBUG


#endif // TransferFuncTests_h
