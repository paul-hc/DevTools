#ifndef FileSystemTests_h
#define FileSystemTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


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
	void TestNumericFilename( void );
	void TestTempFilePool( void );
	void TestFileAndDirectoryState( void );
	void TestTouchFile( void );
	void TestFileTransferMatch( void );
	void TestBackupFile( void );
};


#endif //_DEBUG


#endif // FileSystemTests_h
