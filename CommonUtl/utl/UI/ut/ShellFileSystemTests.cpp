
#include "stdafx.h"
#include "ut/ShellFileSystemTests.h"
#include "Recycler.h"
#include "ShellTypes.h"
#include "ShellUtilities.h"
#include "ShellContextMenuHost.h"
#include "WinExplorer.h"
#include "StringUtilities.h"
#include "utl/ContainerUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


namespace ut
{
	size_t ShellDeleteFiles( const ut::CTempFilePool& pool, const TCHAR relFilePaths[] )
	{
		std::vector< fs::CPath > fullPaths;
		pool.SplitQualifyPaths( fullPaths, relFilePaths );

		return shell::DeleteFiles( fullPaths, AfxGetMainWnd(), FOF_ALLOWUNDO | FOF_SILENT | FOF_NOCONFIRMATION );
	}
}


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

void CShellFileSystemTests::TestPathShellApi( void )
{
	ASSERT( PathIsSameRoot( _T("C:\\dev\\Samples\\CodeProject\\RecycleBin_src\\Debug\\RecycleBinApp.exe"), _T("C:\\dev\\Samples\\CodeProject\\RecycleBin_src\\CoolBtn.h") ) );
	ASSERT( PathIsSameRoot( _T("C:\\dev\\Samples\\CodeProject\\"), _T("C:\\dev\\Samples") ) );

	ASSERT( !PathIsSameRoot( _T("C:\\dev\\Samples\\CodeProject\\RecycleBin_src\\Debug\\RecycleBinApp.exe"), _T("E:\\Media Library\\Music Drive Mapping.txt") ) );

	TCHAR commonPath[ MAX_PATH ];
	ASSERT( PathCommonPrefix( _T("C:\\dev\\Samples\\CodeProject\\RecycleBin_src\\Debug\\RecycleBinApp.exe"), _T("C:\\dev\\Samples\\_scratch\\SynchronizedQueue.h"), commonPath ) > 3 );
	ASSERT_EQUAL_STR( _T("C:\\dev\\Samples"), commonPath );

	PathCommonPrefix( _T("C:\\dev\\Samples\\_scratch\\SynchronizedQueue.h"), _T("C:\\dev\\Samples\\_scratch\\"), commonPath );
	ASSERT_EQUAL_STR( _T("C:\\dev\\Samples\\_scratch"), commonPath );
}

void CShellFileSystemTests::TestPathExplorerSort( void )
{
	const TCHAR s_srcFiles[] =
		_T("Ardeal\\1254 Biertan{DUP}.txt|")
		_T("ardeal\\1254 Biertan-DUP.txt|")
		_T("ARDEAL\\1254 Biertan~DUP.txt|")
		_T("ARDeal\\1254 biertan[DUP].txt|")
		_T("ardEAL\\1254 biertan_DUP.txt|")
		_T("Ardeal\\1254 Biertan(DUP).txt|")
		_T("Ardeal\\1254 Biertan+DUP.txt|")
		_T("Ardeal\\1254 Biertan_noDUP.txt|")
		_T("Ardeal\\1254 Biertan.txt");

	std::vector< fs::CPath > filePaths;
	str::Split( filePaths, s_srcFiles, _T("|") );

	std::random_shuffle( filePaths.begin(), filePaths.end() );

	shell::SortPaths( filePaths );

	path::StripCommonParentPath( filePaths );		// get rid of the directory prefix, to focus on filename order

	//ut::CTempFilePairPool pool( s_srcFiles );		// uncomment this line to break in the debugger and read the actual file order in Explorer.exe

	// Explorer.exe sort order (on Windows 7):		note: it changes with version; provided by ::StrCmpLogicalW from <shlwapi.h>
	ASSERT_EQUAL(
		_T("1254 Biertan(DUP).txt|")
		_T("1254 Biertan.txt|")
		_T("1254 biertan[DUP].txt|")
		_T("1254 biertan_DUP.txt|")
		_T("1254 Biertan_noDUP.txt|")
		_T("1254 Biertan{DUP}.txt|")
		_T("1254 Biertan~DUP.txt|")
		_T("1254 Biertan+DUP.txt|")
		_T("1254 Biertan-DUP.txt")
		, str::Join( filePaths, _T("|") ) );
}

void CShellFileSystemTests::TestRecycler( void )
{
	ut::CTempFilePool pool( _T("a.txt|B\\b1.txt|B\\b2.txt|B\\C\\c1.txt|B\\C\\c2.txt|B\\D\\d1.txt") );
	const fs::CPath& poolDirPath = pool.GetPoolDirPath();

	ASSERT_EQUAL( _T("a.txt|B\\b1.txt|B\\b2.txt|B\\C\\c1.txt|B\\C\\c2.txt|B\\D\\d1.txt"), ut::EnumJoinFiles( poolDirPath ) );

	// delete files
	ASSERT( ut::ShellDeleteFiles( pool, _T("B\\b2.txt") ) );
	ASSERT_EQUAL( _T("a.txt|B\\b1.txt|B\\C\\c1.txt|B\\C\\c2.txt|B\\D\\d1.txt"), ut::EnumJoinFiles( pool.GetPoolDirPath() ) );

	ASSERT( ut::ShellDeleteFiles( pool, _T("a.txt|B\\C") ) );
	ASSERT_EQUAL( _T("B\\b1.txt|B\\D\\d1.txt"), ut::EnumJoinFiles( pool.GetPoolDirPath() ) );

	// undelete files (restore)
	ASSERT( shell::UndeleteFile( poolDirPath / _T("B\\b2.txt") ) );
	ASSERT_EQUAL( _T("B\\b1.txt|B\\b2.txt|B\\D\\d1.txt"), ut::EnumJoinFiles( pool.GetPoolDirPath() ) );

	std::vector< fs::CPath > delFilePaths, errorFilePaths;
	pool.SplitQualifyPaths( delFilePaths, _T("B\\C|a.txt|XXX\\foo.txt") );

	ASSERT_EQUAL( 2, shell::UndeleteFiles( delFilePaths, AfxGetMainWnd(), &errorFilePaths ) );
	ASSERT_EQUAL( _T("a.txt|B\\b1.txt|B\\b2.txt|B\\C\\c1.txt|B\\C\\c2.txt|B\\D\\d1.txt"), ut::EnumJoinFiles( poolDirPath ) );
	ASSERT_EQUAL( 1, errorFilePaths.size() );
	ASSERT_EQUAL( poolDirPath / _T("XXX\\foo.txt"), errorFilePaths.front() );
}

void CShellFileSystemTests::TestMultiFileContextMenu( void )
{
	ut::CTempFilePool pool( _T("file1.txt|file2.txt|DIR\\file3.txt") );
	const std::vector< fs::CPath >& filePaths = pool.GetFilePaths();

	std::vector< CComPtr< IShellItem > > shellItems; shellItems.reserve( filePaths.size() );

	for ( std::vector< fs::CPath >::const_iterator itFilePath = filePaths.begin(); itFilePath != filePaths.end(); ++itFilePath )
	{
		shellItems.push_back( shell::FindShellItem( *itFilePath ) );
		ASSERT( shellItems.back() != NULL );
	}

	CComPtr< IContextMenu > pContextMenu;
	if ( true )
		pContextMenu = shell::MakeItemsContextMenu( shellItems, NULL );
	else	//	for files in same folder - fails for DIR\\file3.txt
	{
		CComPtr< IShellItemArray > pShellItemArray = shell::MakeShellItemArray( shellItems );
		ASSERT( HR_OK( pShellItemArray->BindToHandler( NULL, BHID_SFUIObject, IID_PPV_ARGS( &pContextMenu ) ) ) );
	}
	ASSERT_PTR( pContextMenu );
}


void CShellFileSystemTests::Run( void )
{
	__super::Run();

	TestShellPidl();
	TestShellRelativePidl();
	TestPathShellApi();
	TestPathExplorerSort();
	TestRecycler();
	TestMultiFileContextMenu();
}


#endif //_DEBUG
