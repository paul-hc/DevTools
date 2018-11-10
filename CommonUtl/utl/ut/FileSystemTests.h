#ifndef FileSystemTests_h
#define FileSystemTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "ut/UnitTest.h"


class CFileSystemTests : public ut::CConsoleTestCase
{
	CFileSystemTests( void );
public:
	static CFileSystemTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestFileSystem( void );
	void TestFileEnum( void );
	void TestStgShortFilenames( void );
	void TestTempFilePool( void );
	void TestFileAndDirectoryState( void );

	void TestShellPidl( void );
	void TestShellRelativePidl( void );
};


#endif //_DEBUG


#endif // FileSystemTests_h
