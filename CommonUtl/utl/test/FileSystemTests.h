#ifndef FileSystemTests_h
#define FileSystemTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

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
	void TestFileEnumFilter( void );
	void TestFileEnumHidden( void );
	void TestNumericFilename( void );
	void TestTempFilePool( void );
	void TestFileAndDirectoryState( void );
	void TestTouchFile( void );
	void TestFileTransferMatch( void );
	void TestBackupFile( void );
};


#endif //USE_UT


#endif // FileSystemTests_h
