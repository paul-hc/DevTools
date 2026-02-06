#ifndef ShellPidlTests_h
#define ShellPidlTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "test/UnitTest.h"


namespace ut
{
	int TrackContextMenu( IContextMenu* pCtxMenu );
}


class CShellPidlTests : public ut::CConsoleTestCase
{
	CShellPidlTests( void );
public:
	static CShellPidlTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestNullAndEmptyPidl( void );
	void TestCreateFromPath( void );
	void TestPidlBasics( void );
	void TestFolderRelativePidls( void );
	void TestPidl_FileSystem( void );
	void TestPidl_FileSystemNonExistent( void );
	void TestPidl_GuidSpecial( void );
	void TestParentPidl( void );
	void TestCommonAncestorPidl( void );
};


#endif //USE_UT


#endif // ShellPidlTests_h
