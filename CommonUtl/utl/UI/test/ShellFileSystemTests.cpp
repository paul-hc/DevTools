
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/ShellFileSystemTests.h"
#include "test/UnitTestUI.h"
#include "Recycler.h"
#include "ShellEnumerator.h"
#include "ShellUtilities.h"
#include "ShellPidl.h"
#include "WinExplorer.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	size_t ShellDeleteFiles( const ut::CTempFilePool& pool, const TCHAR relFilePaths[] )
	{
		std::vector<fs::CPath> fullPaths;
		pool.SplitQualifyPaths( fullPaths, relFilePaths );

		return shell::DeleteFiles( fullPaths, AfxGetMainWnd(), FOF_ALLOWUNDO | FOF_SILENT | FOF_NOCONFIRMATION );
	}

	static const fs::TEnumFlags s_recurse( fs::EF_Recurse );
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

void CShellFileSystemTests::TestEnumShellItems( void )
{
	shell::CPidlAbsolute cpAllItemsFolderPidl( FOLDERID_ControlPanelFolder );	// initial set to "::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0"
	ASSERT_EQUAL( _T("Control Panel\\All Control Panel Items"), cpAllItemsFolderPidl.GetEditingName() );

	shell::CPidlAbsolute cpRootFolderPidl;
	ASSERT( cpAllItemsFolderPidl.GetParentPidl( cpRootFolderPidl ) );			// go to parent "::{26EE0668-A00A-44D7-9371-BEB064C98683}"
	ASSERT_EQUAL( _T("Control Panel"), cpRootFolderPidl.GetEditingName() );

	const shell::TFolderPath cpRootPath = cpRootFolderPidl.ToShellPath();

	// "Control Panel" root, no recursion:
	{
		fs::TEnumFlags noFlags;
		fs::CPathEnumerator found( noFlags );
		shell::CEnumContext enumCtx;

		enumCtx.SearchEnumItems( &found, cpRootPath );
		ASSERT_EQUAL( _T(""), ut::JoinFiles( found ) );		// no leaf items
		ASSERT_EQUAL( _T("\
Control Panel\\All Control Panel Items\n\
Control Panel\\Appearance and Personalization\n\
Control Panel\\Clock and Region\n\
Control Panel\\Ease of Access\n\
Control Panel\\Hardware and Sound\n\
Control Panel\\Network and Internet\n\
Control Panel\\Programs\n\
Control Panel\\System and Security\n\
Control Panel\\User Accounts"),
					  ut::JoinEditingNames( found.m_subDirPaths ) );
	}

	// "Control Panel\All Control Panel Items" folder, no recursion:
	if ( !shell::IsRunningUnderWow64() )	// works only in 64-bit, and yields empty results on 32-bit under WOW64 (different structure on 32-bit)
	{
		fs::TEnumFlags recurse( fs::EF_Recurse );
		fs::CPathEnumerator found( recurse );
		shell::CEnumContext enumCtx;

		found.RefOptions().m_maxDepthLevel = 0;				// restrict recursion as deeper search yields way too many results

		enumCtx.EnumFolderItems( &found, cpAllItemsFolderPidl );	// folder PIDL
		ASSERT_EQUAL( _T(""), ut::JoinFiles( found ) );		// no leaf items

		//TRACE( _T( "\n%s\n" ), ut::JoinEditingNames( found.m_subDirPaths, cpAllItemsFolderPidl.GetEditingName() ).c_str() );
		ASSERT_EQUAL( _T("\
AutoPlay\n\
Backup and Restore (Windows 7)\n\
BitLocker Drive Encryption\n\
Credential Manager\n\
Default Programs\n\
Devices and Printers\n\
Ease of Access Center\n\
File History\n\
::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\Fonts\n\
Network and Sharing Center\n\
Power Options\n\
Programs and Features\n\
Recovery\n\
RemoteApp and Desktop Connections\n\
Security and Maintenance\n\
Speech Recognition\n\
Storage Spaces\n\
Sync Center\n\
System\n\
Troubleshooting\n\
User Accounts\n\
Windows Defender Firewall\n\
Windows Tools"),
					  ut::JoinEditingNames( found.m_subDirPaths, cpAllItemsFolderPidl.GetEditingName() ) );
	}
}


void CShellFileSystemTests::Run( void )
{
	RUN_TEST( TestPathShellApi );
	RUN_TEST( TestPathExplorerSort );
	RUN_TEST( TestRecycler );
	RUN_TEST( TestMultiFileContextMenu );
	RUN_TEST( TestEnumShellItems );
}


#endif //USE_UT
