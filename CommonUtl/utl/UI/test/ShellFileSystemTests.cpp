
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/ShellFileSystemTests.h"
#include "Recycler.h"
#include "ShellUtilities.h"
#include "ShellPidl.h"
#include "WinExplorer.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	int TrackContextMenu( IContextMenu* pCtxMenu );		// FWD: defined in ShellPidlTests.cpp

	size_t ShellDeleteFiles( const ut::CTempFilePool& pool, const TCHAR relFilePaths[] )
	{
		std::vector<fs::CPath> fullPaths;
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
	static CShellFileSystemTests s_testCase;
	return s_testCase;
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

	std::vector<fs::CPath> filePaths;
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
	const fs::TDirPath& poolDirPath = pool.GetPoolDirPath();

	ASSERT_EQUAL( _T("a.txt|B\\b1.txt|B\\b2.txt|B\\C\\c1.txt|B\\C\\c2.txt|B\\D\\d1.txt"), ut::EnumJoinFiles( poolDirPath ) );

	// delete files
	ASSERT( ut::ShellDeleteFiles( pool, _T("B\\b2.txt") ) );
	ASSERT_EQUAL( _T("a.txt|B\\b1.txt|B\\C\\c1.txt|B\\C\\c2.txt|B\\D\\d1.txt"), ut::EnumJoinFiles( pool.GetPoolDirPath() ) );

	ASSERT( ut::ShellDeleteFiles( pool, _T("a.txt|B\\C") ) );
	ASSERT_EQUAL( _T("B\\b1.txt|B\\D\\d1.txt"), ut::EnumJoinFiles( pool.GetPoolDirPath() ) );

	// undelete files (restore)
	ASSERT( shell::UndeleteFile( poolDirPath / _T("B\\b2.txt") ) );
	ASSERT_EQUAL( _T("B\\b1.txt|B\\b2.txt|B\\D\\d1.txt"), ut::EnumJoinFiles( pool.GetPoolDirPath() ) );

	std::vector<fs::CPath> delFilePaths, errorFilePaths;
	pool.SplitQualifyPaths( delFilePaths, _T("B\\C|a.txt|XXX\\foo.txt") );

	ASSERT_EQUAL( 2, shell::UndeleteFiles( delFilePaths, AfxGetMainWnd(), &errorFilePaths ) );
	ASSERT_EQUAL( _T("a.txt|B\\b1.txt|B\\b2.txt|B\\C\\c1.txt|B\\C\\c2.txt|B\\D\\d1.txt"), ut::EnumJoinFiles( poolDirPath ) );
	ASSERT_EQUAL( 1, errorFilePaths.size() );
	ASSERT_EQUAL( poolDirPath / _T("XXX\\foo.txt"), errorFilePaths.front() );
}

void CShellFileSystemTests::TestMultiFileContextMenu( void )
{
	ut::CTempFilePool pool( _T("file1.txt|file2.txt|DIR\\file3.txt") );
	const std::vector<fs::CPath>& filePaths = pool.GetFilePaths();

	std::vector< CComPtr<IShellItem> > shellItems; shellItems.reserve( filePaths.size() );

	for ( std::vector<fs::CPath>::const_iterator itFilePath = filePaths.begin(); itFilePath != filePaths.end(); ++itFilePath )
	{
		shellItems.push_back( shell::MakeShellItem( itFilePath->GetPtr() ) );
		ASSERT( shellItems.back() != nullptr );
	}

	{
		CComPtr<IContextMenu> pContextMenu;

		pContextMenu = shell::MakeItemsContextMenu( shellItems, nullptr );
		ASSERT_PTR( pContextMenu );

		//ut::TrackContextMenu( pContextMenu );
	}

	if (0)
	{	// for files in same folder: binding fails for "DIR\file3.txt", so it doesn't work for heterogenous parent directory items
		CComPtr<IShellItemArray> pShellItemArray = shell::MakeShellItemArray( shellItems );
		CComPtr<IContextMenu> pContextMenu;

		if ( !HR_OK( pShellItemArray->BindToHandler( nullptr, BHID_SFUIObject, IID_PPV_ARGS( &pContextMenu ) ) ) )
			ASSERT( false );

		CComPtr<IBindCtx> pBindCtx = shell::CreateFileSysBindContext();
		if ( !HR_OK( pShellItemArray->BindToHandler( pBindCtx, BHID_SFUIObject, IID_PPV_ARGS( &pContextMenu ) ) ) )
			ASSERT( false );

		ASSERT_PTR( pContextMenu );

		ut::TrackContextMenu( pContextMenu );
	}
}


void CShellFileSystemTests::Run( void )
{
	RUN_TEST( TestPathShellApi );
	RUN_TEST( TestPathExplorerSort );
	RUN_TEST( TestRecycler );
	RUN_TEST( TestMultiFileContextMenu );
}


#endif //USE_UT
