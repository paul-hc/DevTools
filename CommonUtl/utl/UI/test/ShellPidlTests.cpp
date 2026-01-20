
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/ShellPidlTests.h"
#include "ShellPidl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CShellPidlTests::CShellPidlTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CShellPidlTests& CShellPidlTests::Instance( void )
{
	static CShellPidlTests s_testCase;
	return s_testCase;
}

void CShellPidlTests::TestNullAndEmptyPidl( void )
{
	fs::TDirPath tempDirPath = fs::GetTempDirPath();
	CComPtr<IShellFolder> pDesktopFolder = shell::GetDesktopFolder();

	ASSERT_PTR( pDesktopFolder );

	{
		shell::CPidlAbsolute pidl;

		ASSERT( pidl.IsNull() );
		pidl.SetEmpty();
		ASSERT( pidl.IsEmpty() );
		ASSERT( !pidl.HasValue() );
		ASSERT_EQUAL( _T("Desktop"), pidl.ToPath().GetFilename() );

		ASSERT( pidl.CreateFromPath( tempDirPath.GetPtr() ) );
		ASSERT( !pidl.IsEmpty() );
		ASSERT( pidl.HasValue() );
		ASSERT_EQUAL( tempDirPath, pidl.ToPath() );

		shell::CPidlAbsolute copyPidl( pidl );

		ASSERT( pidl.IsNull() );		// ownership transfered
		ASSERT_EQUAL( tempDirPath, copyPidl.ToPath() );
	}

	{
		shell::CPidlAbsolute desktopPidl;

		ASSERT( desktopPidl.CreateFromFolder( pDesktopFolder ) );
		ASSERT( desktopPidl.IsEmpty() );		// Desktop == empty
	}
}

void CShellPidlTests::TestShellPidl( void )
{
	ut::CTempFilePool pool( _T("fa.txt|d1\\fb.txt") );
	const fs::TDirPath& poolDirPath = pool.GetPoolDirPath();

	CComPtr<IShellFolder> pDesktopFolder = shell::GetDesktopFolder();
	ASSERT_PTR( pDesktopFolder );

	shell::CPidlAbsolute desktopPidl;
	ASSERT( desktopPidl.CreateFromFolder( pDesktopFolder ) );
	fs::TDirPath desktopPath = desktopPidl.ToPath();

	shell::CPidlAbsolute poolDirPidl;
	{
		ASSERT( poolDirPidl.IsNull() );
		poolDirPidl.SetEmpty();
		ASSERT( poolDirPidl.IsEmpty() );
		ASSERT( !poolDirPidl.HasValue() );

		ASSERT( poolDirPidl.CreateFromPath( poolDirPath.GetPtr() ) );
		ASSERT( !poolDirPidl.IsEmpty() );
		ASSERT( poolDirPidl.HasValue() );
		ASSERT_EQUAL( poolDirPath, poolDirPidl.ToPath() );
	}

	{
		shell::CPidlAbsolute poolDirPidl2;
		poolDirPidl2.AssignCopy( poolDirPidl.Get() );
		ASSERT( poolDirPidl == poolDirPidl2 );				// absolute compare
	}

	CComPtr<IShellFolder> pPoolFolder = poolDirPidl.OpenFolder();
	ASSERT_PTR( pPoolFolder );

	shell::CPidlAbsolute pidl;
	{
		ASSERT( pidl.CreateFrom( pDesktopFolder ) );
		ASSERT_EQUAL( desktopPath, pidl.ToPath() );

		ASSERT( pidl.CreateFrom( pPoolFolder ) );
		ASSERT_EQUAL( poolDirPath, pidl.ToPath() );
	}

	// FILE[0]
	{
		const fs::CPath& filePath = pool.GetFilePaths()[0];

		shell::CPidlAbsolute filePidl;
		ASSERT( filePidl.CreateFromPath( filePath.GetPtr() ) );
		ASSERT( filePidl.GetItemCount() > 1 );
		ASSERT_EQUAL( filePath, filePidl.ToPath() );
		ASSERT_EQUAL( _T("fa.txt"), filePidl.GetFnameExt() );		// filePath.GetFilename()
		ASSERT_EQUAL( _T("temp_ut\\_OUT\\fa.txt"), filePidl.ToPath().FindUpwardsRelativePath( 3 ) );

		{	// last item ID
			shell::CPidlChild lastPidl;
			lastPidl.AssignCopy( filePidl.GetLastItem() );
			ASSERT_EQUAL( _T("fa.txt"), lastPidl.GetFnameExt() );
		}

		{	// child PIDL
			shell::CPidlRelative itemPidl;
			ASSERT( itemPidl.CreateRelative( pPoolFolder, filePath.GetFilenamePtr() ) );
			ASSERT_EQUAL( 1, itemPidl.GetItemCount() );
			ASSERT_EQUAL( desktopPath / itemPidl.GetFnameExt(), itemPidl.ToPath() );		// strangely, for child PIDLs the desktop directory is implicitly prepended
			ASSERT_EQUAL( _T("fa.txt"), itemPidl.GetFnameExt() );		// filePath.GetFilename()

			// shell item from child PIDL relative to parent folder
			CComPtr<IShellItem> pChildFileItem = itemPidl.OpenShellItem( pPoolFolder );
			ASSERT_PTR( pChildFileItem );
			ASSERT_EQUAL( filePath, shell::GetFilePath( pChildFileItem ) );

			ASSERT( pidl.CreateFrom( pChildFileItem ) );
			ASSERT_EQUAL( filePath, pidl.ToPath() );
		}

		{	// shell item
			CComPtr<IShellItem> pFileItem = filePidl.OpenShellItem();
			ASSERT_PTR( pFileItem );
			fs::CPath itemPath = shell::GetFilePath( pFileItem );
			ASSERT_EQUAL( filePath, itemPath );

			ASSERT( pidl.CreateFrom( pFileItem ) );
			ASSERT_EQUAL( filePath, pidl.ToPath() );
		}

		// copy-move
		shell::CPidlAbsolute copyPidl = filePidl;
		ASSERT( filePidl.IsNull() );		// detached
		ASSERT( !copyPidl.IsEmpty() );
		ASSERT_EQUAL( filePath, copyPidl.ToPath() );
	}

	// FILE[1]
	{
		const fs::CPath& filePath = pool.GetFilePaths()[1];

		shell::CPidlAbsolute filePidl;
		ASSERT( filePidl.CreateFromPath( filePath.GetPtr() ) );
		ASSERT_EQUAL( filePath, filePidl.ToPath() );
		ASSERT_EQUAL( _T("fb.txt"), filePidl.GetFnameExt() );		// filePath.GetFilename()
		ASSERT_EQUAL( _T("temp_ut\\_OUT\\d1\\fb.txt"), filePidl.ToPath().FindUpwardsRelativePath( 4 ) );

		shell::CPidlRelative fileRelPidl;
		fileRelPidl.AssignCopy( ::ILFindChild( poolDirPidl.Get(), filePidl.Get() ) );

		ASSERT_EQUAL( _T("fb.txt"), fileRelPidl.ToRelativePath( pPoolFolder ) );
	}

	{	// relative PIDL
		static const TCHAR s_relFilePath[] = _T("d1\\fb.txt");		// relative to pool folder

		shell::CPidlRelative relativePidl;
		ASSERT( relativePidl.CreateRelative( pPoolFolder, s_relFilePath ) );
		ASSERT_EQUAL( 2, relativePidl.GetItemCount() );
		ASSERT_EQUAL( _T("fb.txt"), relativePidl.GetFnameExt() );		// don't know how to find the actual relative path

		//ASSERT_EQUAL( _T("C:\\Users\\Paul\\Desktop\\d1\\fb.txt"), relativePidl.ToPath() );		// depends on user name on the computer
	}
}

void CShellPidlTests::TestShellRelativePidl( void )
{
	ut::CTempFilePool pool( _T("a.txt|d1\\b.txt|d2\\sub3\\c.txt") );

	std::vector<PIDLIST_RELATIVE> pidlItemsArray;
	CComPtr<IShellFolder> pCommonFolder = shell::MakeRelativePidlArray( pidlItemsArray, pool.GetFilePaths() );
	ASSERT_PTR( pCommonFolder );

	ASSERT_EQUAL( pool.GetFilePaths().size(), pidlItemsArray.size() );

	for ( size_t i = 0; i != pidlItemsArray.size(); ++i )
		ASSERT_EQUAL( pool.GetFilePaths()[i].GetFilename(), shell::pidl::GetName( reinterpret_cast<PIDLIST_ABSOLUTE>( pidlItemsArray[i] ) ) );

	shell::ClearOwningPidls( pidlItemsArray );
}


void CShellPidlTests::Run( void )
{
	RUN_TEST( TestNullAndEmptyPidl );
	RUN_TEST( TestShellPidl );
	RUN_TEST( TestShellRelativePidl );
}


#endif //USE_UT
