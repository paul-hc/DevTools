#ifndef ShellPidlTests_h
#define ShellPidlTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "test/UnitTest.h"


class CShellPidlTests : public ut::CConsoleTestCase
{
	CShellPidlTests( void );
public:
	static CShellPidlTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestNullAndEmptyPidl( void );
	void TestShellPidl( void );
	void TestShellRelativePidl( void );
};


#endif //USE_UT


#endif // ShellPidlTests_h
