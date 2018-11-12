
#include "stdafx.h"
#include "ut/ShellFileSystemTests.h"
#include "ShellTypes.h"
#include "WinExplorer.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


CShellFileSystemTests::CShellFileSystemTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CShellFileSystemTests& CShellFileSystemTests::Instance( void )
{
	static CShellFileSystemTests testCase;
	return testCase;
}

void CShellFileSystemTests::TestShellPidl( void )
{
	ut::CTempFilePool pool( _T("fa.txt|d1\\fb.txt") );
	const fs::CPath& poolDirPath = pool.GetPoolDirPath();

	CComPtr< IShellFolder > pDesktopFolder = shell::GetDesktopFolder();
	ASSERT_PTR( pDesktopFolder );

	shell::CPidl desktopPidl;
	ASSERT( desktopPidl.CreateFromFolder( pDesktopFolder ) );
	fs::CPath desktopPath = desktopPidl.GetAbsolutePath();

	shell::CPidl poolDirPidl;
	ASSERT( poolDirPidl.IsEmpty() );
	ASSERT( poolDirPidl.CreateAbsolute( poolDirPath.GetPtr() ) );
	ASSERT( !poolDirPidl.IsEmpty() );
	ASSERT_EQUAL( poolDirPath, poolDirPidl.GetAbsolutePath() );

	{
		shell::CPidl poolDirPidl2;
		poolDirPidl2.AssignCopy( poolDirPidl.Get() );
		ASSERT( poolDirPidl == poolDirPidl2 );				// absolute compare
	}

	CComPtr< IShellFolder > pPoolFolder = poolDirPidl.FindFolder();
	ASSERT_PTR( pPoolFolder );

	shell::CPidl pidl;
	{
		ASSERT( pidl.CreateFrom( pDesktopFolder ) );
		ASSERT_EQUAL( desktopPath, pidl.GetAbsolutePath() );

		ASSERT( pidl.CreateFrom( pPoolFolder ) );
		ASSERT_EQUAL( poolDirPath, pidl.GetAbsolutePath() );
	}

	{
		const fs::CPath& filePath = pool.GetFilePaths()[ 0 ];

		shell::CPidl filePidl;
		ASSERT( filePidl.CreateAbsolute( filePath.GetPtr() ) );
		ASSERT( filePidl.GetCount() > 1 );
		ASSERT_EQUAL( filePath, filePidl.GetAbsolutePath() );
		ASSERT_EQUAL( _T("fa.txt"), filePidl.GetName() );		// filePath.GetNameExt()

		{	// last item ID
			shell::CPidl lastPidl;
			lastPidl.AssignCopy( filePidl.GetLastItem() );
			ASSERT_EQUAL( _T("fa.txt"), lastPidl.GetName() );
		}

		{	// child PIDL
			shell::CPidl itemPidl;
			ASSERT( itemPidl.CreateRelative( pPoolFolder, filePath.GetNameExt() ) );
			ASSERT_EQUAL( 1, itemPidl.GetCount() );
			ASSERT_EQUAL( desktopPath / itemPidl.GetName(), itemPidl.GetAbsolutePath() );		// strangely, for child PIDLs the desktop directory is implicitly prepended
			ASSERT_EQUAL( _T("fa.txt"), itemPidl.GetName() );		// filePath.GetNameExt()

			// shell item from child PIDL relative to parent folder
			CComPtr< IShellItem > pChildFileItem = itemPidl.FindItem( pPoolFolder );
			ASSERT_PTR( pChildFileItem );
			ASSERT_EQUAL( filePath, shell::CWinExplorer().GetItemPath( pChildFileItem ) );

			ASSERT( pidl.CreateFrom( pChildFileItem ) );
			ASSERT_EQUAL( filePath, pidl.GetAbsolutePath() );
		}

		{	// shell item
			CComPtr< IShellItem > pFileItem = filePidl.FindItem();
			ASSERT_PTR( pFileItem );
			fs::CPath itemPath = shell::CWinExplorer().GetItemPath( pFileItem );
			ASSERT_EQUAL( filePath, itemPath );

			ASSERT( pidl.CreateFrom( pFileItem ) );
			ASSERT_EQUAL( filePath, pidl.GetAbsolutePath() );
		}

		// copy-move
		shell::CPidl copyPidl = filePidl;
		ASSERT( filePidl.IsNull() );
		ASSERT( !copyPidl.IsEmpty() );
		ASSERT_EQUAL( filePath, copyPidl.GetAbsolutePath() );
	}

	{	// relative PIDL
		static const TCHAR s_relFilePath[] = _T("d1\\fb.txt");		// relative to pool folder

		shell::CPidl relativePidl;
		ASSERT( relativePidl.CreateRelative( pPoolFolder, s_relFilePath ) );
		ASSERT_EQUAL( 2, relativePidl.GetCount() );
		ASSERT_EQUAL( _T("fb.txt"), relativePidl.GetName() );		// don't know how to find the actual relative path
	}
}

void CShellFileSystemTests::TestShellRelativePidl( void )
{
	ut::CTempFilePool pool( _T("a.txt|d1\\b.txt|d2\\sub3\\c.txt") );

	std::vector< PIDLIST_RELATIVE > pidlItemsArray;
	CComPtr< IShellFolder > pCommonFolder = shell::MakeRelativePidlArray( pidlItemsArray, pool.GetFilePaths() );
	ASSERT_PTR( pCommonFolder );

	ASSERT_EQUAL( 3, pidlItemsArray.size() );
	ASSERT_EQUAL( pool.GetFilePaths()[ 0 ].GetNameExt(), shell::pidl::GetName( pidlItemsArray[ 0 ] ) );

	shell::ClearOwningPidls( pidlItemsArray );
}


void CShellFileSystemTests::Run( void )
{
	__super::Run();

	TestShellPidl();
	TestShellRelativePidl();
}


#endif //_DEBUG
