#ifndef ShellFileSystemTests_h
#define ShellFileSystemTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "ut/UnitTest.h"


class CShellFileSystemTests : public ut::CConsoleTestCase
{
	CShellFileSystemTests( void );
public:
	static CShellFileSystemTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestShellPidl( void );
	void TestShellRelativePidl( void );
};


#endif //_DEBUG


#endif // ShellFileSystemTests_h