
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/PathTests.h"
#include "test/TempFilePairPool.h"
#include "Path.h"
#include "FlexPath.h"
#include "ContainerOwnership.h"
#include "StringUtilities.h"
#include "StdHashValue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "Resequence.hxx"


CPathTests::CPathTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CPathTests& CPathTests::Instance( void )
{
	static CPathTests s_testCase;
	return s_testCase;
}

void CPathTests::TestPathBasics( void )
{
	{
		ASSERT( !path::HasRoot( _T("") ) );
		ASSERT( !path::HasRoot( _T("X") ) );
		ASSERT( !path::HasRoot( _T("X:") ) );

		ASSERT( path::HasRoot( _T("\\") ) );
		ASSERT( path::HasRoot( _T("X:\\") ) );
		ASSERT( path::HasRoot( _T("X:\\A\\file.txt") ) );
		ASSERT( path::HasRoot( _T("X:\\A\\B\\file.txt") ) );
			ASSERT( path::HasRoot( _T("/") ) );
			ASSERT( path::HasRoot( _T("X:/") ) );
			ASSERT( path::HasRoot( _T("X:/A/file.txt") ) );
			ASSERT( path::HasRoot( _T("X:/A/B\\file.txt") ) );

		ASSERT( !path::HasRoot( _T("..\\Dir\\file.txt") ) );
			ASSERT( !path::HasRoot( _T("../Dir/file.txt") ) );
	}

	{
		ASSERT( !path::HasPrefix( _T("something"), _T("") ) );

		ASSERT( path::HasPrefix( _T("..\\Dir\\file.txt"), _T("..") ) );
		ASSERT( !path::HasPrefix( _T("..\\Dir\\file.txt"), _T(".") ) );

		ASSERT( path::HasPrefix( _T(".\\Dir\\file.txt"), _T(".") ) );
		ASSERT( !path::HasPrefix( _T(".\\Dir\\file.txt"), _T("..") ) );

		ASSERT( !path::HasPrefix( _T("\\\\server"), _T("\\\\") ) );
		ASSERT( path::HasPrefix( _T("\\\\server\\share"), _T("\\\\SERVER") ) );
		ASSERT( path::HasPrefix( _T("\\\\server\\share"), _T("\\\\Server\\sHARE") ) );

		ASSERT( path::HasPrefix( _T("X:\\A\\"), _T("X:\\A") ) );
		ASSERT( path::HasPrefix( _T("X:\\A"), _T("X:\\A\\") ) );

		ASSERT( path::HasPrefix( _T("X:\\A\\file.txt"), _T("X:\\A") ) );
		ASSERT( path::HasPrefix( _T("X:\\A\\file.txt"), _T("X:\\A\\") ) );
	}

	{
		ASSERT( !path::HasDirectory( _T("") ) );
		ASSERT( path::HasDirectory( _T("\\") ) );
		ASSERT( path::HasDirectory( _T("/") ) );
		ASSERT( path::HasDirectory( _T("\\name") ) );
		ASSERT( path::HasDirectory( _T("/name") ) );

		ASSERT( !path::HasDirectory( _T("name") ) );
		ASSERT( !path::HasDirectory( _T("name.ext") ) );
		ASSERT( path::HasDirectory( _T("..\\name.ext") ) );
		ASSERT( path::HasDirectory( _T(".\\name.ext") ) );
			ASSERT( path::HasDirectory( _T("../name.ext") ) );
			ASSERT( path::HasDirectory( _T("./name.ext") ) );
	}

	{
		ASSERT_EQUAL_STR( _T("apple"), path::FindFilename( _T("C:\\Images\\fruit.stg/apple") ) );
		ASSERT_EQUAL_STR( _T("apple"), path::FindFilename( _T("C:\\Images/fruit.stg/apple") ) );
		ASSERT_EQUAL_STR( _T("apple.jpg"), path::FindFilename( _T("C:\\Images\\fruit.stg/apple.jpg") ) );
		ASSERT_EQUAL_STR( _T("apple.jpg"), path::FindFilename( _T("C:\\Images\\fruit.stg/World\\Europe\\apple.jpg") ) );
		ASSERT_EQUAL_STR( _T("apple.jpg"), path::FindFilename( _T("C:\\Images\\fruit.stg/World\\Europe/apple.jpg") ) );

		ASSERT_EQUAL_STR( _T(".jpg"), path::FindExt( _T("C:\\Images\\fruit.stg/apple.jpg") ) );
		ASSERT_EQUAL_STR( _T(".jpg"), path::FindExt( _T("C:\\Images\\fruit.stg/World\\Europe\\apple.fruit.jpg") ) );
		ASSERT_EQUAL_STR( _T(".jpg"), path::FindExt( _T("C:\\Images\\fruit.stg/World\\Europe/apple.fruit.jpg") ) );
	}
}

void CPathTests::TestPathIs( void )
{
	// "\", , "\\server\share", or "\\server\"; paths such as "..\path2"
	ASSERT( path::IsRoot( _T("\\") ) );
	ASSERT( path::IsRoot( _T("X:\\") ) );
	ASSERT( path::IsRoot( _T("\\\\server") ) );
	ASSERT( path::IsRoot( _T("\\\\server\\share") ) );
	ASSERT( !path::IsRoot( _T("") ) );
	ASSERT( !path::IsRoot( _T("X:\\My") ) );
		ASSERT( path::IsRoot( _T("/") ) );
		ASSERT( path::IsRoot( _T("X:/") ) );
		ASSERT( path::IsRoot( _T("//server") ) );
		ASSERT( path::IsRoot( _T("//server/share") ) );
		ASSERT( !path::IsRoot( _T("") ) );
		ASSERT( !path::IsRoot( _T("X:/My") ) );

	ASSERT( path::IsAbsolute( _T("X:\\Dir") ) );
	ASSERT( path::IsAbsolute( _T("X:\\Dir\\file.txt") ) );
	ASSERT( !path::IsAbsolute( _T("..\\Dir\\file.txt") ) );
	ASSERT( path::IsRelative( _T("..\\Dir\\file.txt") ) );
	ASSERT( path::IsRelative( _T("file.txt") ) );
		ASSERT( path::IsAbsolute( _T("X:/Dir") ) );
		ASSERT( path::IsAbsolute( _T("X:/Dir/file.txt") ) );
		ASSERT( !path::IsAbsolute( _T("../Dir/file.txt") ) );
		ASSERT( path::IsRelative( _T("../Dir/file.txt") ) );
		ASSERT( path::IsRelative( _T("file.txt") ) );
}

void CPathTests::TestPathConversion( void )
{
	fs::CPath filePath;

	ASSERT_EQUAL( _T(""), filePath = filePath.MakeUnixPath() );
	ASSERT_EQUAL( _T(""), filePath.MakeWindowsPath() );

	filePath.Set( _T("\\") );
	ASSERT_EQUAL( _T("/"), filePath = filePath.MakeUnixPath() );
	ASSERT_EQUAL( _T("\\"), filePath.MakeWindowsPath() );

	filePath.Set( _T("X:\\") );
	ASSERT_EQUAL( _T("X:/"), filePath = filePath.MakeUnixPath() );
	ASSERT_EQUAL( _T("X:\\"), filePath.MakeWindowsPath() );

	filePath.Set( _T("\\\\server") );
	ASSERT_EQUAL( _T("//server"), filePath = filePath.MakeUnixPath() );
	ASSERT_EQUAL( _T("\\\\server"), filePath.MakeWindowsPath() );

	filePath.Set( _T("\\\\server\\share") );
	ASSERT_EQUAL( _T("//server/share"), filePath = filePath.MakeUnixPath() );
	ASSERT_EQUAL( _T("\\\\server\\share"), filePath.MakeWindowsPath() );

	filePath.Set( _T("X:\\Dir\\Sub\\name.ext") );
	ASSERT_EQUAL( _T("X:/Dir/Sub/name.ext"), filePath = filePath.MakeUnixPath() );
	ASSERT_EQUAL( _T("X:\\Dir\\Sub\\name.ext"), filePath.MakeWindowsPath() );
}

void CPathTests::TestPathUtilities( void )
{
	// rooth path:
	ASSERT_EQUAL( _T("\\"), path::GetRootPath( _T("\\") ) );
	ASSERT_EQUAL( _T("X:\\"), path::GetRootPath( _T("X:\\") ) );
	ASSERT_EQUAL( _T("X:\\"), path::GetRootPath( _T("X:\\My") ) );
	ASSERT_EQUAL( _T("\\\\server"), path::GetRootPath( _T("\\\\server") ) );
	ASSERT_EQUAL( _T("\\\\server\\share"), path::GetRootPath( _T("\\\\server\\share\\") ) );

	ASSERT_EQUAL( _T("X:\\A\\C"), path::MakeCanonical( _T("X:\\A\\.\\B\\..\\C") ) );
	ASSERT_EQUAL( _T("X:\\A\\C"), path::MakeCanonical( _T("X:/A/./B/../C") ) );

	ASSERT_EQUAL( _T("X:\\A\\.\\B\\..\\C"), path::MakeWindows( _T("X:\\A/./B/../C") ) );

	ASSERT_EQUAL( _T("X:\\A\\B\\file.txt"), path::Combine( _T("X:\\A\\B"), _T("file.txt") ) );
	ASSERT_EQUAL( _T("X:\\A\\B\\subdir\\file.txt"), path::Combine( _T("X:\\A\\B"), _T("subdir\\file.txt") ) );

	ASSERT_EQUAL( _T("X:\\A\\B\\subdir\\file.txt"), path::Combine( _T("X:/A/B"), _T("subdir/file.txt") ) );

	{
		ASSERT( !path::IsValid( _T("") ) );
		ASSERT( path::IsValid( _T("a") ) );
		ASSERT( path::IsValid( _T(".txt") ) );
		ASSERT( path::IsValid( _T("X:\\dir/file") ) );
		ASSERT( path::IsValid( _T("X:\\dir/file.ext") ) );
		ASSERT( path::IsValid( _T("X:\\dir/.ext") ) );
		ASSERT( !path::IsValid( _T("X:\\file?.txt") ) );
		ASSERT( !path::IsValid( _T("X:\\file*.txt") ) );
		ASSERT( !path::IsValid( _T("X:\\file|.txt") ) );
	}

	{
		std::tstring path;
		ASSERT_EQUAL( _T(""), path::SetBackslash( path = _T(""), true ) );
		ASSERT_EQUAL( _T(""), path::SetBackslash( path = _T(""), false ) );
		ASSERT_EQUAL( _T("\\"), path::SetBackslash( path = _T("\\"), true ) );
		ASSERT_EQUAL( _T("\\"), path::SetBackslash( path = _T("\\"), false ) );
		ASSERT_EQUAL( _T("X:\\"), path::SetBackslash( path = _T("X:\\"), true ) );
		ASSERT_EQUAL( _T("X:\\"), path::SetBackslash( path = _T("X:\\"), false ) );
		ASSERT_EQUAL( _T("X:\\Dir\\"), path::SetBackslash( path = _T("X:\\Dir"), true ) );
		ASSERT_EQUAL( _T("X:\\Dir\\"), path::SetBackslash( path = _T("X:\\Dir\\"), true ) );
		ASSERT_EQUAL( _T("X:\\Dir"), path::SetBackslash( path = _T("X:\\Dir"), false ) );
		ASSERT_EQUAL( _T("X:\\Dir"), path::SetBackslash( path = _T("X:\\Dir\\"), false ) );

		ASSERT_EQUAL( _T("X:\\Dir\\"), path::SetBackslash( path = _T("X:\\Dir"), path::AddSlash ) );
		ASSERT_EQUAL( _T("X:\\Dir"), path::SetBackslash( path = _T("X:\\Dir\\"), path::RemoveSlash ) );
		ASSERT_EQUAL( _T("X:\\Dir\\"), path::SetBackslash( path = _T("X:\\Dir\\"), path::PreserveSlash ) );
		ASSERT_EQUAL( _T("X:\\Dir"), path::SetBackslash( path = _T("X:\\Dir"), path::PreserveSlash ) );

		ASSERT_EQUAL( _T("\\Dir\\"), path::SetBackslash( path = _T("\\Dir"), true ) );
		ASSERT_EQUAL( _T("\\Dir\\"), path::SetBackslash( path = _T("\\Dir\\"), true ) );
		ASSERT_EQUAL( _T("\\Dir"), path::SetBackslash( path = _T("\\Dir"), false ) );
		ASSERT_EQUAL( _T("\\Dir"), path::SetBackslash( path = _T("\\Dir\\"), false ) );

		ASSERT_EQUAL( _T("\\\\server\\"), path::SetBackslash( path = _T("\\\\server"), true ) );
		ASSERT_EQUAL( _T("\\\\server\\"), path::SetBackslash( path = _T("\\\\server\\"), true ) );
		ASSERT_EQUAL( _T("\\\\server"), path::SetBackslash( path = _T("\\\\server"), false ) );
		ASSERT_EQUAL( _T("\\\\server"), path::SetBackslash( path = _T("\\\\server\\"), false ) );

		ASSERT_EQUAL( _T("\\\\server\\share\\"), path::SetBackslash( path = _T("\\\\server\\share"), true ) );
		ASSERT_EQUAL( _T("\\\\server\\share\\"), path::SetBackslash( path = _T("\\\\server\\share\\"), true ) );
		ASSERT_EQUAL( _T("\\\\server\\share"), path::SetBackslash( path = _T("\\\\server\\share"), false ) );
		ASSERT_EQUAL( _T("\\\\server\\share"), path::SetBackslash( path = _T("\\\\server\\share\\"), false ) );
	}

	{
		ASSERT_EQUAL( _T("X:\\Dir\\Sub\\"), path::GetParentPath( _T("X:\\Dir\\Sub\\name.ext"), path::PreserveSlash ) );
		ASSERT_EQUAL( _T("X:\\Dir\\"), path::GetParentPath( _T("X:\\Dir\\Sub\\"), path::PreserveSlash ) );
		ASSERT_EQUAL( _T("X:\\Dir/"), path::GetParentPath( _T("X:\\Dir/Sub/"), path::PreserveSlash ) );
		ASSERT_EQUAL( _T("X:\\Dir/"), path::GetParentPath( _T("X:\\Dir/Sub\\"), path::PreserveSlash ) );
		ASSERT_EQUAL( _T("X:\\Dir"), path::GetParentPath( _T("X:\\Dir\\Sub\\"), path::RemoveSlash ) );
		ASSERT_EQUAL( _T("X:\\"), path::GetParentPath( _T("X:\\name.ext"), path::PreserveSlash ) );
		ASSERT_EQUAL( _T("/"), path::GetParentPath( _T("/name.ext"), path::PreserveSlash ) );
		ASSERT_EQUAL( _T(""), path::GetParentPath( _T("name.ext"), path::PreserveSlash ) );

		ASSERT_EQUAL( _T("X:\\Dir\\Sub"), path::GetParentPath( _T("X:\\Dir\\Sub\\name.ext"), path::RemoveSlash ) );
		ASSERT_EQUAL( _T("X:\\"), path::GetParentPath( _T("X:\\name.ext"), path::RemoveSlash ) );
		ASSERT_EQUAL( _T("/"), path::GetParentPath( _T("/name.ext"), path::RemoveSlash ) );

		ASSERT_EQUAL( _T(""), path::GetParentPath( _T("name.ext"), path::AddSlash ) );
	}
	{
		const fs::CPathParts parts( _T("X:\\Dir\\Sub/name.ext") );
		ASSERT_EQUAL( _T("X:"), parts.m_drive );
		ASSERT_EQUAL( _T("\\Dir\\Sub/"), parts.m_dir );
		ASSERT_EQUAL( _T("name"), parts.m_fname );
		ASSERT_EQUAL( _T(".ext"), parts.m_ext );

		ASSERT_EQUAL( _T("X:\\Dir\\Sub"), parts.GetDirPath() );
		ASSERT_EQUAL( _T("name.ext"), parts.GetFilename() );
	}
	{
		const fs::CPathParts parts( _T("\\\\server\\share\\Dir\\name.ext") );
		ASSERT_EQUAL( _T(""), parts.m_drive );
		ASSERT_EQUAL( _T("\\\\server\\share\\Dir\\"), parts.m_dir );
		ASSERT_EQUAL( _T("name"), parts.m_fname );
		ASSERT_EQUAL( _T(".ext"), parts.m_ext );

		ASSERT_EQUAL( _T("\\\\server\\share\\Dir"), parts.GetDirPath() );
		ASSERT_EQUAL( _T("name.ext"), parts.GetFilename() );
	}
	{
		const fs::CPath path( _T("X:\\Dir\\Sub/name.ext") );
		ASSERT_EQUAL( _T("X:\\Dir\\Sub"), path.GetParentPath() );
		ASSERT_EQUAL( _T("X:\\Dir\\Sub/"), path.GetParentPath( true ) );

		ASSERT_EQUAL( _T("name.ext"), path.GetFilename() );
	}

	{	// path depth:
		ASSERT_EQUAL( 0, fs::CPath( _T("") ).GetDepth() );

		ASSERT_EQUAL( 1, fs::CPath( _T("\\") ).GetDepth() );
		ASSERT_EQUAL( 2, fs::CPath( _T("\\\\server") ).GetDepth() );
		ASSERT_EQUAL( 3, fs::CPath( _T("\\\\server\\share") ).GetDepth() );

		ASSERT_EQUAL( 1, fs::CPath( _T("X:") ).GetDepth() );
		ASSERT_EQUAL( 1, fs::CPath( _T("X:\\") ).GetDepth() );
		ASSERT_EQUAL( 2, fs::CPath( _T("X:\\name.ext") ).GetDepth() );
		ASSERT_EQUAL( 3, fs::CPath( _T("X:\\Dir/name.ext") ).GetDepth() );
		ASSERT_EQUAL( 4, fs::CPath( _T("X:\\Dir\\Sub/name.ext") ).GetDepth() );

		ASSERT_EQUAL( 4, fs::CPath( _T("X:\\Dir\\Sub/name.ext") ).GetDepth() );
	}

	{
		fs::CPath path( _T("X:\\Dir\\Sub\\name.ext") );
		path.SetDirPath( _T("C:/A/B") );
		ASSERT_EQUAL( _T("C:/A/B\\name.ext"), path.Get() );

		path.SetDirPath( _T("C:/A/B/C/") );
		ASSERT_EQUAL( _T("C:/A/B/C/name.ext"), path.Get() );

		path.SetFilename( _T("list.ini") );
		ASSERT_EQUAL( _T("C:/A/B/C/list.ini"), path.Get() );

		path.Normalize();
		ASSERT_EQUAL( _T("C:\\A\\B\\C\\list.ini"), path.Get() );
	}
	{
		//"desktop\temp.txt" for "C:\win\desktop\temp.txt" and "c:\win"
		ASSERT_EQUAL( _T("C:\\win\\desktop\\temp.txt"), path::StripCommonPrefix( _T("C:\\win\\desktop\\temp.txt"), nullptr ) );
		ASSERT_EQUAL( _T("C:\\win\\desktop\\temp.txt"), path::StripCommonPrefix( _T("C:\\win\\desktop\\temp.txt"), _T("X:\\win") ) );
		ASSERT_EQUAL( _T("desktop\\temp.txt"), path::StripCommonPrefix( _T("C:\\win\\desktop\\temp.txt"), _T("C:\\WIN") ) );
		ASSERT_EQUAL( _T("desktop\\temp.txt"), path::StripCommonPrefix( _T("C:\\win\\desktop\\temp.txt"), _T("C:\\WIN\\") ) );
		ASSERT_EQUAL( _T("desktop\\temp.txt"), path::StripCommonPrefix( _T("C:\\win\\desktop\\temp.txt"), _T("C:/WIN/") ) );
		ASSERT_EQUAL( _T("desktop\\temp.txt"), path::StripCommonPrefix( _T("C:\\win\\desktop\\temp.txt"), _T("C:/WIN/system") ) );
		ASSERT_EQUAL( _T("desktop/temp.txt"), path::StripCommonPrefix( _T("C:/win/desktop/temp.txt"), _T("C:\\WIN/") ) );
	}
	{
		std::tstring filePath;

		filePath = _T("C:/win/desktop/temp.txt");
		ASSERT( path::StripPrefix( filePath, _T("C:\\WIN") ) );
		ASSERT_EQUAL( _T("desktop/temp.txt"), filePath );

		filePath = _T("C:/win/desktop/temp.txt");
		ASSERT( path::StripPrefix( filePath, _T("C:\\WIN/") ) );
		ASSERT_EQUAL( _T("desktop/temp.txt"), filePath );

		filePath = _T("C:/win/desktop/temp.txt");
		ASSERT( !path::StripPrefix( filePath, _T("C:\\WIN\\system") ) );
		ASSERT_EQUAL( _T("C:/win/desktop/temp.txt"), filePath );			// output is kind of undetermined though
	}
}

void CPathTests::TestPathSort( void )
{
	std::vector<std::tstring> srcPaths;
	srcPaths.push_back( _T("C:\\dir/fn.txt") );
	srcPaths.push_back( _T("C:\\dir2/subDir/fn.txt") );
	srcPaths.push_back( _T("X:\\dir/file.ext") );
	srcPaths.push_back( _T("C:\\dir000/fn.txt") );
	srcPaths.push_back( _T("C:\\dir35/fn.txt") );
	srcPaths.push_back( _T("C:\\dir/file.txt") );
	srcPaths.push_back( _T("C:\\dir00001/fn.txt") );

	{
		std::vector<std::tstring> stringPaths = srcPaths;

		fs::SortPaths( stringPaths );
		std::vector<std::tstring>::const_iterator itPath = stringPaths.begin();
		ASSERT_EQUAL( _T("C:\\dir/file.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir000/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir00001/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir2/subDir/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir35/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("X:\\dir/file.ext"), *itPath++ );
		ASSERT( itPath == stringPaths.end() );

		fs::SortPaths( stringPaths, false );
		itPath = stringPaths.begin();
		ASSERT_EQUAL( _T("X:\\dir/file.ext"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir35/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir2/subDir/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir00001/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir000/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir/file.txt"), *itPath++ );
		ASSERT( itPath == stringPaths.end() );
	}

	{
		std::vector<fs::CPath> paths( srcPaths.begin(), srcPaths.end() );

		fs::SortPaths( paths );
		std::vector<fs::CPath>::const_iterator itPath = paths.begin();
		ASSERT_EQUAL( _T("C:\\dir/file.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir000/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir00001/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir2/subDir/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir35/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("X:\\dir/file.ext"), *itPath++ );
		ASSERT( itPath == paths.end() );

		fs::SortPaths( paths, false );
		itPath = paths.begin();
		ASSERT_EQUAL( _T("X:\\dir/file.ext"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir35/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir2/subDir/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir00001/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir000/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir/fn.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir/file.txt"), *itPath++ );
		ASSERT( itPath == paths.end() );
	}

	{
		std::vector<fs::CPath> paths;
		paths.push_back( fs::CPath( _T("C:\\dir\\X\\Y\\Z\\file.txt") ) );
		paths.push_back( fs::CPath( _T("C:\\dir\\X\\Y\\file.txt") ) );
		paths.push_back( fs::CPath( _T("") ) );
		paths.push_back( fs::CPath( _T("C:\\dir\\X\\file.txt") ) );
		paths.push_back( fs::CPath( _T("C:\\dir/file.txt") ) );
		paths.push_back( fs::CPath( _T("\\") ) );
		paths.push_back( fs::CPath( _T("C:\\dir\\Image.jpg") ) );
		paths.push_back( fs::CPath( _T("C:\\dir\\file.txt") ) );
		paths.push_back( fs::CPath( _T("C:") ) );
		paths.push_back( fs::CPath( _T("A:") ) );

		fs::SortByPathDepth( paths );

		std::vector<fs::CPath>::const_iterator itPath = paths.begin();
		ASSERT_EQUAL( _T(""), *itPath++ );
		ASSERT_EQUAL( _T("A:"), *itPath++ );
		ASSERT_EQUAL( _T("C:"), *itPath++ );
		ASSERT_EQUAL( _T("\\"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir/file.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir\\file.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir\\Image.jpg"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir\\X\\file.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir\\X\\Y\\file.txt"), *itPath++ );
		ASSERT_EQUAL( _T("C:\\dir\\X\\Y\\Z\\file.txt"), *itPath++ );
		ASSERT( itPath == paths.end() );
	}
}

void CPathTests::TestPathSortExisting( void )
{
	ut::CTempFilePairPool pool( _T("a|a.doc|a.txt|d3/some|d1\\b|d1\\b3.doc|d1\\b002.doc|d1\\b.txt|d1\\d2\\c|d1/d2/c.doc|d1\\d2\\c.txt") );
	const fs::TDirPath& poolDirPath = pool.GetPoolDirPath();

	std::vector<fs::CPath> mixedPaths;	// files + directories
	path::QueryParentPaths( mixedPaths, pool.GetFilePaths() );
	mixedPaths.insert( mixedPaths.begin(), pool.GetFilePaths().begin(), pool.GetFilePaths().end() );

	std::random_shuffle( mixedPaths.begin(), mixedPaths.end() );

	// fs::CPath sort ascending
	{
		std::vector<fs::CPath> paths = mixedPaths;
		std::vector<fs::CPath>::const_iterator itPath;

		fs::SortPathsDirsFirst( paths );
		path::StripDirPrefixes( paths, poolDirPath.GetPtr() );		// left with just relative paths (post physical dir/file grouping)

		itPath = paths.begin();
		// dirs first
		ASSERT_EQUAL( _T(""), *itPath++ );							// empty relative root dir
		ASSERT_EQUAL( _T("d1"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\d2"), *itPath++ );
		ASSERT_EQUAL( _T("d3"), *itPath++ );
		// files after
		ASSERT_EQUAL( _T("a"), *itPath++ );
		ASSERT_EQUAL( _T("a.doc"), *itPath++ );
		ASSERT_EQUAL( _T("a.txt"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\b"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\b.txt"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\b002.doc"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\b3.doc"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\d2\\c"), *itPath++ );
		ASSERT_EQUAL( _T("d1/d2/c.doc"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\d2\\c.txt"), *itPath++ );
		ASSERT_EQUAL( _T("d3/some"), *itPath++ );
	}

	// fs::CPath sort descending
	{
		std::vector<fs::CPath> paths = mixedPaths;
		std::vector<fs::CPath>::const_iterator itPath;

		fs::SortPathsDirsFirst( paths, false );					// sort descending
		path::StripDirPrefixes( paths, poolDirPath.GetPtr() );		// left with just relative paths (post physical dir/file grouping)

		itPath = paths.begin();
		// dirs first
		ASSERT_EQUAL( _T("d3"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\d2"), *itPath++ );
		ASSERT_EQUAL( _T("d1"), *itPath++ );
		ASSERT_EQUAL( _T(""), *itPath++ );							// empty relative root dir
		// files after
		ASSERT_EQUAL( _T("d3/some"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\d2\\c.txt"), *itPath++ );
		ASSERT_EQUAL( _T("d1/d2/c.doc"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\d2\\c"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\b3.doc"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\b002.doc"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\b.txt"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\b"), *itPath++ );
		ASSERT_EQUAL( _T("a.txt"), *itPath++ );
		ASSERT_EQUAL( _T("a.doc"), *itPath++ );
		ASSERT_EQUAL( _T("a"), *itPath++ );
	}

	// std::tstring sort ascending
	{
		std::vector<std::tstring> stringPaths;
		for ( std::vector<fs::CPath>::const_iterator itMixedPath = mixedPaths.begin(); itMixedPath != mixedPaths.end(); ++itMixedPath )
			stringPaths.push_back( itMixedPath->Get() );

		std::vector<std::tstring>::const_iterator itPath;

		fs::SortPathsDirsFirst( stringPaths );
		path::StripDirPrefixes( stringPaths, poolDirPath.GetPtr() );		// left with just relative paths (post physical dir/file grouping)

		itPath = stringPaths.begin();
		// dirs first
		ASSERT_EQUAL( _T(""), *itPath++ );							// empty relative root dir
		ASSERT_EQUAL( _T("d1"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\d2"), *itPath++ );
		ASSERT_EQUAL( _T("d3"), *itPath++ );
		// files after
		ASSERT_EQUAL( _T("a"), *itPath++ );
		ASSERT_EQUAL( _T("a.doc"), *itPath++ );
		ASSERT_EQUAL( _T("a.txt"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\b"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\b.txt"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\b002.doc"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\b3.doc"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\d2\\c"), *itPath++ );
		ASSERT_EQUAL( _T("d1\\d2\\c.doc"), *itPath++ );				// string version normalizes
		ASSERT_EQUAL( _T("d1\\d2\\c.txt"), *itPath++ );
		ASSERT_EQUAL( _T("d3\\some"), *itPath++ );
	}
}

void CPathTests::TestPathNaturalSort( void )
{
	const TCHAR s_srcFiles[] =
		_T("Ardeal\\1254 Biertan{DUP}.txt|")
		_T("ardeal/1254 Biertan-DUP.txt|")
		_T("ARDEAL\\1254 Biertan~DUP.txt|")
		_T("ARDeal\\1254 biertan[DUP].txt|")
		_T("ardEAL\\1254 biertan_DUP.txt|")
		_T("Ardeal\\1254 Biertan(DUP).txt|")
		_T("Ardeal\\1254 Biertan+DUP.txt|")
		_T("Ardeal\\1254 Biertan_noDUP.txt|")
		_T("Ardeal/1254 Biertan.txt");

	std::vector<fs::CPath> filePaths;
	str::Split( filePaths, s_srcFiles, _T("|") );

	std::random_shuffle( filePaths.begin(), filePaths.end() );

	fs::SortPaths( filePaths );

	{	// check that fs::TPathSet orders in the same order with fs::SortPaths()
		fs::TPathSet setOfFilePaths( filePaths.begin(), filePaths.end() );
		ASSERT_EQUAL( str::Join( filePaths, _T("|") ), str::Join( setOfFilePaths, _T("|") ) );
	}

	path::StripCommonParentPath( filePaths );		// get rid of the directory prefix, to focus on filename order

	ASSERT_EQUAL(
		_T("1254 Biertan.txt|")				// straight extension (shortest filename) goes first
		_T("1254 Biertan-DUP.txt|")
		_T("1254 Biertan+DUP.txt|")
		_T("1254 biertan_DUP.txt|")
		_T("1254 Biertan_noDUP.txt|")
		_T("1254 Biertan(DUP).txt|")
		_T("1254 biertan[DUP].txt|")
		_T("1254 Biertan{DUP}.txt|")
		_T("1254 Biertan~DUP.txt")
		, str::Join( filePaths, _T("|") ) );

	/*
	ut::CTempFilePairPool pool( s_srcFiles );

	Explorer.exe sort order (on Windows 7):		note: it changes with version; provided by ::StrCmpLogicalW from <shlwapi.h>
		1254 Biertan(DUP).txt
		1254 Biertan.txt
		1254 biertan[DUP].txt
		1254 biertan_DUP.txt
		1254 Biertan_noDUP.txt
		1254 Biertan{DUP}.txt
		1254 Biertan~DUP.txt
		1254 Biertan+DUP.txt
		1254 Biertan-DUP.txt
	*/

	/* Old "intuitive" sort order was:
		1254 Biertan-DUP.txt
		1254 Biertan.txt
		1254 Biertan(DUP).txt
		1254 Biertan+DUP.txt
		1254 biertan[DUP].txt
		1254 biertan_DUP.txt
		1254 Biertan_noDUP.txt
		1254 Biertan{DUP}.txt
		1254 Biertan~DUP.txt
	*/
}

void CPathTests::TestPathCompareFind( void )
{
	{
		ASSERT( path::Equivalent( _T("X:\\DIR\\SUB\\NAME.EXT"), _T("x:/dir/sub/name.ext") ) );
		ASSERT( !path::Equals( _T("X:\\DIR\\SUB\\NAME.EXT"), _T("x:/dir/sub/name.ext") ) );

		ASSERT( !path::Equivalent( _T("a:\\list.txt"), _T("x:/fname.ext") ) );
		ASSERT( !path::Equals( _T("a:\\list.txt"), _T("x:/fname.ext") ) );

		ASSERT_EQUAL( pred::Equal, path::CompareEquivalent( _T("X:\\DIR\\SUB\\NAME.EXT"), _T("x:/dir/sub/name.ext") ) );
		ASSERT_EQUAL( pred::Less, path::CompareEquivalent( _T("name 05.ext"), _T("name 10.ext") ) );
	}
	{
		static const fs::CPath windowsPath( _T("X:\\DIR\\SUB\\NAME.EXT") );
		static const fs::CPath unixPath( _T("x:/dir/sub/name.ext") );

		ASSERT_EQUAL( windowsPath, unixPath );
		ASSERT( windowsPath == unixPath );

		// check transitive
		ASSERT( !( windowsPath < unixPath ) );
		ASSERT( !( unixPath < windowsPath ) );

		ASSERT( windowsPath.ExtEquals( _T(".EXT") ) );
		ASSERT( windowsPath.ExtEquals( _T(".ext") ) );
		ASSERT( windowsPath.ExtEquals( _T(".ExT") ) );
		ASSERT( !windowsPath.ExtEquals( _T(".extx") ) );
	}
	{
		path::TGetMatch getMatchFunc;
		ASSERT_EQUAL( str::MatchEqual, getMatchFunc( _T("x:\\dir\\sub\\name.ext"), _T("x:/dir/sub/name.ext") ) );
		ASSERT_EQUAL( str::MatchEqualDiffCase, getMatchFunc( _T("X:\\dir\\sub\\NAME.ext"), _T("x:/dir/sub/name.ext") ) );
		ASSERT_EQUAL( str::MatchNotEqual, getMatchFunc( _T("X:\\dir\\sub\\NAME.txt"), _T("x:/dir/sub/name.jpg") ) );
	}
	{
		ASSERT_EQUAL( _T("\\name.ext"), std::tstring( path::Find( _T("X:\\Dir\\Sub\\name.ext"), _T("/Name.EXT") ) ) );
		ASSERT_EQUAL( _T("ir\\Sub\\name.ext"), std::tstring( path::Find( _T("X:\\Dir\\Sub\\name.ext"), _T("ir/Su") ) ) );
		ASSERT_EQUAL( _T("X:\\Dir\\Sub\\name.ext"), std::tstring( path::Find( _T("X:\\Dir\\Sub\\name.ext"), _T("x:/") ) ) );
		ASSERT_EQUAL( _T(""), std::tstring( path::Find( _T("X:\\Dir\\Sub\\name.ext"), _T("DUMMY") ) ) );
		ASSERT_EQUAL( _T("X:/Dir/Sub\\name.ext"), std::tstring( path::Find( _T("X:/Dir/Sub\\name.ext"), _T("X:\\Dir\\Sub\\name.ext") ) ) );
	}
}

void CPathTests::TestPathWildcardMatch( void )
{
	ASSERT( path::MatchWildcard( _T("a"), _T("*") ) );
	ASSERT( path::MatchWildcard( _T("a"), _T("*.*") ) );
	ASSERT( path::MatchWildcard( _T("a"), _T("*.") ) );
	ASSERT( path::MatchWildcard( _T("a"), _T("?") ) );
	ASSERT( path::MatchWildcard( _T("a"), _T("?.") ) );
	ASSERT( path::MatchWildcard( _T("a"), _T("a") ) );
	ASSERT( path::MatchWildcard( _T("a"), _T("A") ) );
	ASSERT( path::MatchWildcard( _T("a"), _T("a.*") ) );
	ASSERT( path::MatchWildcard( _T("a"), _T("a.") ) );
	ASSERT( !path::MatchWildcard( _T("a"), _T("a.txt") ) );
	ASSERT( !path::MatchWildcard( _T("a"), _T("*.txt") ) );

	ASSERT( path::MatchWildcard( _T("a.txt"), _T("*") ) );
	ASSERT( path::MatchWildcard( _T("a.txt"), _T("*.*") ) );
	ASSERT( !path::MatchWildcard( _T("a.txt"), _T("*.") ) );
	ASSERT( path::MatchWildcard( _T("a.txt"), _T("a.*") ) );
	ASSERT( path::MatchWildcard( _T("a.txt"), _T("*.txt") ) );
	ASSERT( path::MatchWildcard( _T("a.txt"), _T("?.txt") ) );
	ASSERT( path::MatchWildcard( _T("a.txt"), _T("a.txt") ) );
	ASSERT( path::MatchWildcard( _T("a.txt"), _T("A.TXT") ) );
	ASSERT( !path::MatchWildcard( _T("a.txt"), _T("a.") ) );

	ASSERT( path::MatchWildcard( _T(".txt"), _T("*") ) );
	ASSERT( path::MatchWildcard( _T(".txt"), _T(".*") ) );
	ASSERT( path::MatchWildcard( _T(".txt"), _T("*.*") ) );
	ASSERT( !path::MatchWildcard( _T(".txt"), _T("*.") ) );
	ASSERT( path::MatchWildcard( _T(".txt"), _T("*.txt") ) );
	ASSERT( path::MatchWildcard( _T(".txt"), _T(".txt") ) );
	ASSERT( path::MatchWildcard( _T(".txt"), _T(".TXT") ) );
	ASSERT( !path::MatchWildcard( _T(".txt"), _T("a.") ) );

	ASSERT( !path::MatchWildcard( _T("a"), _T("b") ) );
	ASSERT( !path::MatchWildcard( _T("a"), _T("b.") ) );
	ASSERT( !path::MatchWildcard( _T("a"), _T("b.x") ) );
	ASSERT( !path::MatchWildcard( _T("a"), _T("b.*") ) );

	ASSERT( !path::MatchWildcard( _T("a.txt"), _T("b") ) );
	ASSERT( !path::MatchWildcard( _T("a.txt"), _T("b.") ) );
	ASSERT( !path::MatchWildcard( _T("a.txt"), _T("b.x") ) );
	ASSERT( !path::MatchWildcard( _T("a.txt"), _T("b.txt") ) );
	ASSERT( !path::MatchWildcard( _T("a.txt"), _T("b.*") ) );

	// match multiple wildcard specs
	ASSERT( path::MatchWildcard( _T("image.JPEG"), _T("*.doc;*.jpg;*jpeg;*.png") ) );
	ASSERT( path::MatchWildcard( _T("image.JPEG"), _T("*.doc;*.jpg,*jpeg,*.png") ) );
	ASSERT( path::MatchWildcard( _T("image.JPEG"), _T("*.doc;*.jp*") ) );
	ASSERT( path::MatchWildcard( _T("image.JPEG"), _T("*.doc;*.jp*") ) );
}

namespace ut
{
	struct CPathItem
	{
		CPathItem( const TCHAR* pPath, int avgLength = 0 ) : m_path( pPath ), m_avgLength( avgLength ) {}
	public:
		fs::CPath m_path;
		int m_avgLength;
	};
}


namespace func
{
#ifndef IS_CPP_11
	const fs::CPath& PathOf( const std::pair<const ut::CPathItem*, std::string>& mapItem ) { return mapItem.first->m_path; }
#endif
}

void CPathTests::TestHasMultipleDirPaths( void )
{
	{
		std::vector<fs::CPath> paths;
		ASSERT( !path::HasMultipleDirPaths( paths ) );

		paths.push_back( fs::CPath( _T("C:\\Images\\Fruit\\apple.jpg") ) );
		ASSERT( !path::HasMultipleDirPaths( paths ) );

		paths.push_back( fs::CPath( _T("C:\\Images/Fruit/orange.jpg") ) );
		ASSERT( !path::HasMultipleDirPaths( paths ) );

		paths.push_back( fs::CPath( _T("C:\\Images\\Drink\\coffee.jpg") ) );
		ASSERT( path::HasMultipleDirPaths( paths ) );
	}

#ifndef IS_CPP_11
	{
		#pragma warning( disable: 4709 )	// comma operator within array index expression

		std::map<ut::CPathItem*, std::string> pathMap;

		pathMap[ new ut::CPathItem( _T("C:\\Images/Fruit/apple.jpg"), 10 ) ] = "A";
		pathMap[ new ut::CPathItem( _T("C:\\Images\\Fruit\\orange.jpg"), 20 ) ] = "B";
		ASSERT( !path::HasMultipleDirPaths( pathMap ) );

		pathMap[ new ut::CPathItem( _T("C:\\Images\\Drink\\coffee.jpg"), 30 ) ] = "Odd";
		ASSERT( path::HasMultipleDirPaths( pathMap ) );

		utl::ClearOwningMapKeys( pathMap );
	}
#endif
}

void CPathTests::TestCommonSubpath( void )
{
	{
		std::vector<fs::CPath> paths;
		ASSERT_EQUAL( _T(""), path::ExtractCommonParentPath( paths ) );

		paths.push_back( fs::CPath( _T("C:\\Images\\Fruit\\apple.jpg") ) );
		ASSERT_EQUAL( _T("C:\\Images\\Fruit"), path::ExtractCommonParentPath( paths ) );

		paths.push_back( fs::CPath( _T("C:\\Images/Fruit/orange.jpg") ) );
		ASSERT_EQUAL( _T("C:\\Images\\Fruit"), path::ExtractCommonParentPath( paths ) );

		paths.push_back( fs::CPath( _T("C:\\Images\\Drink\\coffee.jpg") ) );
		ASSERT_EQUAL( _T("C:\\Images"), path::ExtractCommonParentPath( paths ) );

		paths.push_back( fs::CPath( _T("C:\\Show\\bill.jpg") ) );
		ASSERT_EQUAL( _T("C:\\"), path::ExtractCommonParentPath( paths ) );

		paths.push_back( fs::CPath( _T("D:\\Eat\\cheese.jpg") ) );
		ASSERT_EQUAL( _T(""), path::ExtractCommonParentPath( paths ) );
	}

	{
		std::vector<fs::CPath> paths;
		paths.push_back( fs::CPath( _T("C:\\Images\\Fruit\\apple.jpg") ) );
		paths.push_back( fs::CPath( _T("C:\\Images/Fruit/orange.jpg") ) );
		paths.push_back( fs::CPath( _T("C:\\Images\\Drink\\coffee.jpg") ) );

		fs::TDirPath commonDirPath = path::ExtractCommonParentPath( paths );

		for ( std::vector<fs::CPath>::iterator itPath = paths.begin(); itPath != paths.end(); ++itPath )
		{
			std::tstring relPath = itPath->Get();
			path::StripPrefix( relPath, commonDirPath.GetPtr() );
			*itPath = relPath;
		}

		ASSERT_EQUAL( _T("Fruit\\apple.jpg"), paths[ 0 ] );
		ASSERT_EQUAL( _T("Fruit\\orange.jpg"), paths[ 1 ] );
		ASSERT_EQUAL( _T("Drink\\coffee.jpg"), paths[ 2 ] );
	}

	{
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg>Europe"), path::FindCommonPrefix( _T("C:\\Images\\fruit.stg>Europe\\apple.jpg"), _T("C:/Images/fruit.stg>Europe/Citrus/orange.jpg") ) );

		std::vector<fs::CPath> paths;
		paths.push_back( fs::CPath( _T("C:\\Images\\fruit.stg>Europe\\apple.jpg") ) );
		paths.push_back( fs::CPath( _T("C:\\Images\\fruit.stg>Europe\\Citrus\\orange.jpg") ) );

		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg>Europe"), path::ExtractCommonParentPath( paths ) );

		paths.push_back( fs::CPath( _T("C:\\Images\\fruit.stg>Africa\\coffee.jpg") ) );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path::ExtractCommonParentPath( paths ) );

		paths.push_back( fs::CPath( _T("C:\\Images\\Asia\\mango.jpg") ) );
		ASSERT_EQUAL( _T("C:\\Images"), path::ExtractCommonParentPath( paths ) );

		paths.push_back( fs::CPath( _T("X:\\Docs\\catalog.txt") ) );
		ASSERT_EQUAL( _T(""), path::ExtractCommonParentPath( paths ) );
	}
}

void CPathTests::TestComplexPath( void )
{
	ASSERT( !path::IsComplex( _T("C:\\Images\\Fruit/apple.jpg") ) );
	ASSERT( path::IsComplex( _T("C:\\Images\\fruit.stg>apple.jpg") ) );
	ASSERT( path::IsComplex( _T("C:\\Images\\fruit.stg>World\\Europe\\apple.jpg") ) );

	ASSERT( path::IsWellFormed( _T("C:\\Images\\Europe/apple.jpg") ) );
	ASSERT( path::IsWellFormed( _T("C:\\Images\\fruit.stg>World/apple.jpg") ) );
	ASSERT( !path::IsWellFormed( _T("C:\\Images\\fruit.stg>World>/apple.jpg") ) );

	ASSERT_EQUAL_STR( _T("apple"), path::FindFilename( _T("C:\\Images\\fruit.stg>apple") ) );
	ASSERT_EQUAL_STR( _T("apple"), path::FindFilename( _T("C:\\Images/fruit.stg>apple") ) );
	ASSERT_EQUAL_STR( _T("apple.jpg"), path::FindFilename( _T("C:\\Images\\fruit.stg>apple.jpg") ) );
	ASSERT_EQUAL_STR( _T("apple.jpg"), path::FindFilename( _T("C:\\Images\\fruit.stg>World\\Europe\\apple.jpg") ) );
	ASSERT_EQUAL_STR( _T("apple.jpg"), path::FindFilename( _T("C:\\Images\\fruit.stg>World\\Europe/apple.jpg") ) );

	ASSERT_EQUAL_STR( _T(".jpg"), path::FindExt( _T("C:\\Images\\fruit.stg>apple.jpg") ) );
	ASSERT_EQUAL_STR( _T(".jpg"), path::FindExt( _T("C:\\Images\\fruit.stg>World\\Europe\\apple.fruit.jpg") ) );

	ASSERT_EQUAL( _T("C:\\Images\\Europe/apple.jpg"), path::ExtractPhysical( _T("C:\\Images\\Europe/apple.jpg") ) );
	ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path::ExtractPhysical( _T("C:\\Images\\fruit.stg>apple.jpg") ) );

	ASSERT_EQUAL_STR( _T(""), path::GetEmbedded( _T("C:\\Images\\Europe/apple.jpg") ) );
	ASSERT_EQUAL_STR( _T("apple.jpg"), path::GetEmbedded( _T("C:\\Images\\fruit.stg>apple.jpg") ) );
	ASSERT_EQUAL_STR( _T("World\\Europe/apple.jpg"), path::GetEmbedded( _T("C:\\Images\\fruit.stg>World\\Europe/apple.jpg") ) );

	ASSERT_EQUAL_STR( _T("C:\\Images\\Europe/apple.jpg"), path::GetSubPath( _T("C:\\Images\\Europe/apple.jpg") ) );
	ASSERT_EQUAL_STR( _T("World\\Europe/apple.jpg"), path::GetSubPath( _T("C:\\Images\\fruit.stg>World\\Europe/apple.jpg") ) );

	ASSERT_EQUAL( _T("C:\\Images\\fruit.stg>World\\Europe/apple.jpg"), path::MakeComplex( _T("C:\\Images\\fruit.stg"), _T("World\\Europe/apple.jpg") ) );

	std::tstring physicalPath, embeddedPath;
	ASSERT( path::SplitComplex( physicalPath, embeddedPath, _T("C:\\Images\\fruit.stg>World\\Europe/apple.jpg") ) );
	ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), physicalPath );
	ASSERT_EQUAL( _T("World\\Europe/apple.jpg"), embeddedPath );

	ASSERT( !path::SplitComplex( physicalPath, embeddedPath, _T("C:\\Images\\World\\Europe/apple.jpg") ) );
	ASSERT_EQUAL( _T("C:\\Images\\World\\Europe/apple.jpg"), physicalPath );
	ASSERT_EQUAL( _T(""), embeddedPath );
}

void CPathTests::TestFlexPath( void )
{
	ASSERT( path::IsEmbedded( _T("file.txt") ) );
	ASSERT( path::IsEmbedded( _T("Dir\\file.txt") ) );
	ASSERT( path::IsEmbedded( _T("Dir/file.txt") ) );
	ASSERT( !path::IsEmbedded( _T(".\\Dir\\file.txt") ) );
	ASSERT( !path::IsEmbedded( _T("..\\Dir\\file.txt") ) );
	ASSERT( !path::IsEmbedded( _T("X:\\Dir\\file.txt") ) );

	// complex path
	{
		fs::CFlexPath path( _T("C:\\Images\\fruit.stg>apple.jpg") );
		ASSERT( path.IsComplexPath() );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path.GetPhysicalPath() );
		ASSERT_EQUAL_STR( _T("apple.jpg"), path.GetEmbeddedPathPtr() );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path.GetParentPath() );
		ASSERT_EQUAL( _T("apple.jpg"), path.GetFilename() );

		fs::CPath physicalPath;
		fs::TEmbeddedPath embeddedPath;
		ASSERT( path.SplitComplexPath( physicalPath, embeddedPath ) );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), physicalPath );
		ASSERT_EQUAL( _T("apple.jpg"), embeddedPath );
	}

	// complex path with storage\\leaf sub-path
	{
		fs::CFlexPath path( _T("C:\\Images\\fruit.stg>Europe\\apple.jpg") );
		ASSERT( path.IsComplexPath() );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path.GetPhysicalPath() );
		ASSERT_EQUAL_STR( _T("Europe\\apple.jpg"), path.GetEmbeddedPathPtr() );
		ASSERT_EQUAL( _T("apple.jpg"), path.GetFilename() );

		ASSERT( path.GetParentPath() == path.GetParentFlexPath() );

		fs::TDirPath parentPath = path;
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg>Europe"), parentPath = parentPath.GetParentPath() );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), parentPath = parentPath.GetParentPath() );
		ASSERT_EQUAL( _T("C:\\Images"), parentPath = parentPath.GetParentPath() );
		ASSERT_EQUAL( _T("C:\\"), parentPath = parentPath.GetParentPath() );

		ASSERT_EQUAL_STR( _T("Europe\\apple.jpg"), path.GetLeafSubPath() );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path.GetOriginParentPath() );

		fs::CPath physicalPath;
		fs::TEmbeddedPath embeddedPath;
		ASSERT( path.SplitComplexPath( physicalPath, embeddedPath ) );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), physicalPath );
		ASSERT_EQUAL( _T("Europe\\apple.jpg"), embeddedPath );
	}

	// normal file path
	{
		fs::CFlexPath path( _T("C:\\Images\\orange.png") );
		ASSERT( path.IsPhysicalPath() );
		ASSERT( !path.IsComplexPath() );
		ASSERT_EQUAL( _T("C:\\Images\\orange.png"), path.GetPhysicalPath() );
		ASSERT_EQUAL( _T(""), path.GetEmbeddedPath() );
		ASSERT_EQUAL( _T("C:\\Images"), path.GetParentPath() );
		ASSERT_EQUAL( _T("orange.png"), path.GetFilename() );

		fs::CPath physicalPath;
		fs::TEmbeddedPath embeddedPath;
		ASSERT( !path.SplitComplexPath( physicalPath, embeddedPath ) );
		ASSERT_EQUAL( _T("C:\\Images\\orange.png"), physicalPath );
		ASSERT_EQUAL( _T(""), embeddedPath );
	}

	// ordered path set
	static const TCHAR comma[] = _T(",");
	{
		std::set<fs::CFlexPath> pathSet;
		pathSet.insert( fs::CFlexPath( _T("File 10") ) );
		pathSet.insert( fs::CFlexPath( _T("file 1") ) );
		pathSet.insert( fs::CFlexPath( _T("File 7") ) );
		pathSet.insert( fs::CFlexPath( _T("file 3") ) );

		// should be ignored as duplicates
		pathSet.insert( fs::CFlexPath( _T("fILE 7") ) );
		pathSet.insert( fs::CFlexPath( _T("FILE 1") ) );
		pathSet.insert( fs::CFlexPath( _T("FILE 3") ) );

		ASSERT_EQUAL( _T("file 1,file 3,File 7,File 10"), str::Join( pathSet.begin(), pathSet.end(), comma ) );
	}

	// split/make with complex path
	{
		// test bug: was splitting "C:\Images\fruit.stg>apple.jpg" to d="C:" dir="\\Images\\" fname="fruit.stg>apple.jpg"
		fs::CPathParts stgParts( _T("C:\\Images\\fruit.stg>apple.jpg") );
		ASSERT_EQUAL( _T("C:"), stgParts.m_drive );
		ASSERT_EQUAL( _T("\\Images\\fruit.stg>"), stgParts.m_dir );
		ASSERT_EQUAL( _T("apple"), stgParts.m_fname );
		ASSERT_EQUAL( _T(".jpg"), stgParts.m_ext );

		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg>apple.jpg"), stgParts.MakePath() );
	}
	{
		fs::CPathParts stgParts( _T("C:\\Images\\fruit.stg>Europe\\apple.jpg") );
		ASSERT_EQUAL( _T("C:"), stgParts.m_drive );
		ASSERT_EQUAL( _T("\\Images\\fruit.stg>Europe\\"), stgParts.m_dir );
		ASSERT_EQUAL( _T("apple"), stgParts.m_fname );
		ASSERT_EQUAL( _T(".jpg"), stgParts.m_ext );

		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg>Europe\\apple.jpg"), stgParts.MakePath() );
	}
}

void CPathTests::TestPathHashValue( void )
{
	static const fs::CPath s_path( _T("C:\\Images/fruit.stg>Europe/apple.jpg") );
	static const size_t strHashValue = std::hash<std::tstring>()( s_path.Get() );

	const size_t pathHashValue = path::GetHashValue( s_path.Get() );
	ASSERT( pathHashValue != strHashValue );		// strHashValue is sensitive to capitalization

	ASSERT_EQUAL( pathHashValue, s_path.GetHashValue() );

	ASSERT_EQUAL( pathHashValue, std::hash<fs::CPath>()( s_path ) );

	fs::CFlexPath flexPath( s_path.Get() );
	ASSERT_EQUAL( pathHashValue, utl::GetHashValue( flexPath ) );		// aka std::hash<fs::CPath>()( flexPath )

	ASSERT_EQUAL( std::hash<fs::CPath>()( fs::CPath( _T("C:\\Images/fruit.stg\\Europe/apple.jpg") ) ), std::hash<fs::CPath>()( fs::CPath( _T("C:/IMAGES/FRUIT.STG/EUROPE/APPLE.JPG") ) ) );
}


void CPathTests::Run( void )
{
	RUN_TEST( TestPathBasics );
	RUN_TEST( TestPathIs );
	RUN_TEST( TestPathConversion );
	RUN_TEST( TestPathUtilities );
	RUN_TEST( TestPathSort );
	RUN_TEST( TestPathSortExisting );
	RUN_TEST( TestPathNaturalSort );
	RUN_TEST( TestPathCompareFind );
	RUN_TEST( TestPathWildcardMatch );
	RUN_TEST( TestHasMultipleDirPaths );
	RUN_TEST( TestCommonSubpath );
	RUN_TEST( TestComplexPath );
	RUN_TEST( TestFlexPath );
	RUN_TEST( TestPathHashValue );
}


#endif //USE_UT
