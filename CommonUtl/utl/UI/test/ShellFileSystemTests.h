#ifndef ShellFileSystemTests_h
#define ShellFileSystemTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "test/UnitTest.h"


class CShellFileSystemTests : public ut::CConsoleTestCase
{
	CShellFileSystemTests( void );
public:
	static CShellFileSystemTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestPathShellApi( void );
	void TestPathExplorerSort( void );
	void TestRecycler( void );
	void TestMultiFileContextMenu( void );
};


#endif //USE_UT


#endif // ShellFileSystemTests_h
