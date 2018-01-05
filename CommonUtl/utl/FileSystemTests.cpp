
#include "stdafx.h"
#include "FileSystemTests.h"
#include "Path.h"
#include "FlexPath.h"
#include "Resequence.hxx"
#include "StringUtilities.h"
#include "StructuredStorage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


namespace str
{
	inline std::tostream& operator<<( std::tostream& oss, const fs::CPath& path )
	{
		return oss << path.Get();
	}
}


CFileSystemTests::CFileSystemTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CFileSystemTests& CFileSystemTests::Instance( void )
{
	static CFileSystemTests testCase;
	return testCase;
}

void CFileSystemTests::TestPathUtilities( void )
{
	// "\", , "\\server\share", or "\\server\"; paths such as "..\path2"
	ASSERT( path::IsRoot( _T("\\") ) );
	ASSERT( path::IsRoot( _T("X:\\") ) );
	ASSERT( path::IsRoot( _T("\\\\server") ) );
	ASSERT( path::IsRoot( _T("\\\\server\\share") ) );
	ASSERT( !path::IsRoot( _T("") ) );
	ASSERT( !path::IsRoot( _T("X:\\My") ) );

	ASSERT( path::IsAbsolute( _T("X:\\My") ) );
	ASSERT( path::IsAbsolute( _T("X:\\My\\file.txt") ) );
	ASSERT( !path::IsAbsolute( _T("..\\My\\file.txt") ) );
	ASSERT( path::IsRelative( _T("..\\My\\file.txt") ) );
	ASSERT( path::IsRelative( _T("file.txt") ) );

	// rooth path:
	ASSERT_EQUAL( _T("\\"), path::GetRootPath( _T("\\") ) );
	ASSERT_EQUAL( _T("X:\\"), path::GetRootPath( _T("X:\\") ) );
	ASSERT_EQUAL( _T("X:\\"), path::GetRootPath( _T("X:\\My") ) );
	ASSERT_EQUAL( _T("\\\\server"), path::GetRootPath( _T("\\\\server") ) );
	ASSERT_EQUAL( _T("\\\\server\\share"), path::GetRootPath( _T("\\\\server\\share\\") ) );

	ASSERT_EQUAL( _T("X:\\A\\C"), path::MakeCanonical( _T("X:\\A\\.\\B\\..\\C") ) );
	ASSERT_EQUAL( _T("X:\\A\\C"), path::MakeCanonical( _T("X:/A/./B/../C") ) );

	ASSERT_EQUAL( _T("X:\\A\\.\\B\\..\\C"), path::MakeNormal( _T("X:\\A/./B/../C") ) );

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
		ASSERT_EQUAL( _T("X:\\Dir\\Sub\\"), path::GetDirPath( _T("X:\\Dir\\Sub\\name.ext"), path::PreserveSlash ) );
		ASSERT_EQUAL( _T("X:\\"), path::GetDirPath( _T("X:\\name.ext"), path::PreserveSlash ) );
		ASSERT_EQUAL( _T("/"), path::GetDirPath( _T("/name.ext"), path::PreserveSlash ) );

		ASSERT_EQUAL( _T("X:\\Dir\\Sub"), path::GetDirPath( _T("X:\\Dir\\Sub\\name.ext"), path::RemoveSlash ) );
		ASSERT_EQUAL( _T("X:\\"), path::GetDirPath( _T("X:\\name.ext"), path::RemoveSlash ) );
		ASSERT_EQUAL( _T("/"), path::GetDirPath( _T("/name.ext"), path::RemoveSlash ) );

		ASSERT_EQUAL( _T(""), path::GetDirPath( _T("name.ext"), path::AddSlash ) );

		ASSERT_EQUAL( _T("X:\\Dir"), path::GetParentDirPath( _T("X:\\Dir\\Sub\\name.ext"), path::RemoveSlash ) );
		ASSERT_EQUAL( _T("X:\\"), path::GetParentDirPath( _T("X:\\Dir\\name.ext"), path::RemoveSlash ) );
		ASSERT_EQUAL( _T("\\"), path::GetParentDirPath( _T("\\Dir\\name.ext"), path::RemoveSlash ) );
		ASSERT_EQUAL( _T(""), path::GetParentDirPath( _T("name.ext"), path::RemoveSlash ) );
	}
	{
		const fs::CPathParts parts( _T("X:\\Dir\\Sub/name.ext") );
		ASSERT_EQUAL( _T("X:"), parts.m_drive );
		ASSERT_EQUAL( _T("\\Dir\\Sub/"), parts.m_dir );
		ASSERT_EQUAL( _T("name"), parts.m_fname );
		ASSERT_EQUAL( _T(".ext"), parts.m_ext );

		ASSERT_EQUAL( _T("X:\\Dir\\Sub"), parts.GetDirPath() );
		ASSERT_EQUAL( _T("X:\\Dir\\Sub/"), parts.GetDirPath( true ) );
		ASSERT_EQUAL_STR( _T("name.ext"), parts.GetNameExt() );
	}
	{
		const fs::CPathParts parts( _T("\\\\server\\share\\Dir\\name.ext") );
		ASSERT_EQUAL( _T(""), parts.m_drive );
		ASSERT_EQUAL( _T("\\\\server\\share\\Dir\\"), parts.m_dir );
		ASSERT_EQUAL( _T("name"), parts.m_fname );
		ASSERT_EQUAL( _T(".ext"), parts.m_ext );

		ASSERT_EQUAL( _T("\\\\server\\share\\Dir"), parts.GetDirPath() );
		ASSERT_EQUAL( _T("\\\\server\\share\\Dir\\"), parts.GetDirPath( true ) );
		ASSERT_EQUAL_STR( _T("name.ext"), parts.GetNameExt() );
	}
	{
		const fs::CPath path( _T("X:\\Dir\\Sub/name.ext") );
		ASSERT_EQUAL( _T("X:\\Dir\\Sub"), path.GetDirPath() );
		ASSERT_EQUAL( _T("X:\\Dir\\Sub/"), path.GetDirPath( true ) );

		ASSERT_EQUAL_STR( _T("name.ext"), path.GetNameExt() );
	}
	{
		fs::CPath path( _T("X:\\Dir\\Sub\\name.ext") );
		path.SetDirPath( _T("C:/A/B") );
		ASSERT_EQUAL( _T("C:/A/B\\name.ext"), path.Get() );

		path.SetDirPath( _T("C:/A/B/C/") );
		ASSERT_EQUAL( _T("C:/A/B/C/name.ext"), path.Get() );

		path.SetNameExt( _T("list.ini") );
		ASSERT_EQUAL( _T("C:/A/B/C/list.ini"), path.Get() );

		path.Normalize();
		ASSERT_EQUAL( _T("C:\\A\\B\\C\\list.ini"), path.Get() );
	}
	{
		//"desktop\temp.txt" for "C:\win\desktop\temp.txt" and "c:\win"
		ASSERT_EQUAL( _T("C:\\win\\desktop\\temp.txt"), path::StripCommonPrefix( _T("C:\\win\\desktop\\temp.txt"), NULL ) );
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

void CFileSystemTests::TestPathCompareFind( void )
{
	{
		ASSERT( path::EquivalentPtr( _T("X:\\DIR\\SUB\\NAME.EXT"), _T("x:/dir/sub/name.ext") ) );
		ASSERT( !path::Equal( _T("X:\\DIR\\SUB\\NAME.EXT"), _T("x:/dir/sub/name.ext") ) );

		ASSERT( !path::EquivalentPtr( _T("a:\\list.txt"), _T("x:/fname.ext") ) );
		ASSERT( !path::Equal( _T("a:\\list.txt"), _T("x:/fname.ext") ) );

		ASSERT_EQUAL( pred::Equal, path::CompareNPtr( _T("X:\\DIR\\SUB\\NAME.EXT"), _T("x:/dir/sub/name.ext") ) );
		ASSERT_EQUAL( pred::Less, path::CompareNPtr( _T("name 05.ext"), _T("name 10.ext") ) );
	}
	{
		ASSERT_EQUAL( path::MatchEqual, path::Match( _T("x:\\dir\\sub\\name.ext"), _T("x:/dir/sub/name.ext") ) );
		ASSERT_EQUAL( path::MatchEqualDiffCase, path::Match( _T("X:\\dir\\sub\\NAME.ext"), _T("x:/dir/sub/name.ext") ) );
		ASSERT_EQUAL( path::MatchNotEqual, path::Match( _T("X:\\dir\\sub\\NAME.txt"), _T("x:/dir/sub/name.jpg") ) );
	}
	{
		ASSERT_EQUAL( _T("\\name.ext"), std::tstring( path::Find( _T("X:\\Dir\\Sub\\name.ext"), _T("/Name.EXT") ) ) );
		ASSERT_EQUAL( _T("ir\\Sub\\name.ext"), std::tstring( path::Find( _T("X:\\Dir\\Sub\\name.ext"), _T("ir/Su") ) ) );
		ASSERT_EQUAL( _T("X:\\Dir\\Sub\\name.ext"), std::tstring( path::Find( _T("X:\\Dir\\Sub\\name.ext"), _T("x:/") ) ) );
		ASSERT_EQUAL( _T(""), std::tstring( path::Find( _T("X:\\Dir\\Sub\\name.ext"), _T("DUMMY") ) ) );
		ASSERT_EQUAL( _T("X:/Dir/Sub\\name.ext"), std::tstring( path::Find( _T("X:/Dir/Sub\\name.ext"), _T("X:\\Dir\\Sub\\name.ext") ) ) );
	}
}

void CFileSystemTests::TestPathWildcardMatch( void )
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

void CFileSystemTests::TestComplexPath( void )
{
	ASSERT( !path::IsComplex( _T("C:\\Images\\Fruit/apple.jpg") ) );
	ASSERT( path::IsComplex( _T("C:\\Images\\fruit.stg>apple.jpg") ) );
	ASSERT( path::IsComplex( _T("C:\\Images\\fruit.stg>World\\Europe\\apple.jpg") ) );

	ASSERT( path::IsWellFormed( _T("C:\\Images\\Europe/apple.jpg") ) );
	ASSERT( path::IsWellFormed( _T("C:\\Images\\fruit.stg>World/apple.jpg") ) );
	ASSERT( !path::IsWellFormed( _T("C:\\Images\\fruit.stg>World>/apple.jpg") ) );

	ASSERT_EQUAL_STR( _T("apple.jpg"), path::FindFilename( _T("C:\\Images\\fruit.stg>apple.jpg") ) );
	ASSERT_EQUAL_STR( _T("apple.jpg"), path::FindFilename( _T("C:\\Images\\fruit.stg>World\\Europe\\apple.jpg") ) );

	ASSERT_EQUAL_STR( _T(".jpg"), path::FindExt( _T("C:\\Images\\fruit.stg>apple.jpg") ) );
	ASSERT_EQUAL_STR( _T(".jpg"), path::FindExt( _T("C:\\Images\\fruit.stg>World\\Europe\\apple.fruit.jpg") ) );

	ASSERT_EQUAL( _T("C:\\Images\\Europe/apple.jpg"), path::GetPhysical( _T("C:\\Images\\Europe/apple.jpg") ) );
	ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path::GetPhysical( _T("C:\\Images\\fruit.stg>apple.jpg") ) );

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

void CFileSystemTests::TestFlexPath( void )
{
	// complex path
	{
		fs::CFlexPath path( _T("C:\\Images\\fruit.stg>apple.jpg") );
		ASSERT( path.IsComplexPath() );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path.GetPhysicalPath() );
		ASSERT_EQUAL_STR( _T("apple.jpg"), path.GetEmbeddedPath() );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path.GetParentPath() );
		ASSERT_EQUAL_STR( _T("apple.jpg"), path.GetNameExt() );

		std::tstring physicalPath, embeddedPath;
		ASSERT( path.SplitComplexPath( physicalPath, embeddedPath ) );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), physicalPath );
		ASSERT_EQUAL( _T("apple.jpg"), embeddedPath );
	}

	// complex path with storage\\leaf sub-path
	{
		fs::CFlexPath path( _T("C:\\Images\\fruit.stg>Europe\\apple.jpg") );
		ASSERT( path.IsComplexPath() );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path.GetPhysicalPath() );
		ASSERT_EQUAL_STR( _T("Europe\\apple.jpg"), path.GetEmbeddedPath() );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path.GetParentPath() );
		ASSERT_EQUAL_STR( _T("apple.jpg"), path.GetNameExt() );

		std::tstring physicalPath, embeddedPath;
		ASSERT( path.SplitComplexPath( physicalPath, embeddedPath ) );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), physicalPath );
		ASSERT_EQUAL( _T("Europe\\apple.jpg"), embeddedPath );
	}

	// normal file path
	{
		fs::CFlexPath path( _T("C:\\Images\\orange.png") );
		ASSERT( !path.IsComplexPath() );
		ASSERT_EQUAL( _T("C:\\Images\\orange.png"), path.GetPhysicalPath() );
		ASSERT_EQUAL_STR( _T(""), path.GetEmbeddedPath() );
		ASSERT_EQUAL( _T("C:\\Images"), path.GetParentPath() );
		ASSERT_EQUAL_STR( _T("orange.png"), path.GetNameExt() );

		std::tstring physicalPath, embeddedPath;
		ASSERT( !path.SplitComplexPath( physicalPath, embeddedPath ) );
		ASSERT_EQUAL( _T("C:\\Images\\orange.png"), physicalPath );
		ASSERT_EQUAL( _T(""), embeddedPath );
	}

	// ordered path set
	static const TCHAR comma[] = _T(",");
	{
		std::set< fs::CFlexPath > pathSet;
		pathSet.insert( fs::CFlexPath( _T("File 10") ) );
		pathSet.insert( fs::CFlexPath( _T("file 1") ) );
		pathSet.insert( fs::CFlexPath( _T("File 7") ) );
		pathSet.insert( fs::CFlexPath( _T("file 3") ) );

		// should be ignored as duplicates
		pathSet.insert( fs::CFlexPath( _T("fILE 7") ) );
		pathSet.insert( fs::CFlexPath( _T("FILE 1") ) );
		pathSet.insert( fs::CFlexPath( _T("FILE 3") ) );
		
		ASSERT_EQUAL( _T("file 1,file 3,File 7,File 10"), str::Unsplit( pathSet.begin(), pathSet.end(), comma ) );
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

void CFileSystemTests::TestFileSystem( void )
{
	std::tstring tempDirPath = fs::GetTempDirPath();
	ASSERT( fs::IsValidDirectory( tempDirPath.c_str() ) );
	ASSERT( !fs::IsValidFile( tempDirPath.c_str() ) );

	path::SetBackslash( tempDirPath, path::RemoveSlash );
	ASSERT( fs::IsValidDirectory( tempDirPath.c_str() ) );

	path::SetBackslash( tempDirPath, path::AddSlash );
	ASSERT( fs::IsValidDirectory( tempDirPath.c_str() ) );
}

void CFileSystemTests::TestFileEnum( void )
{
	ut::CTempFilePairPool pool( _T("a|a.doc|a.txt|d1\\b|d1\\b.doc|d1\\b.txt|d1\\d2\\c|d1/d2/c.doc|d1\\d2\\c.txt") );
	const TCHAR* pTempDirPath = pool.GetTempDirPath().c_str();

	{
		fs::CEnumerator found( pTempDirPath );
		fs::EnumFiles( &found, pTempDirPath, _T("*.*"), Shallow );
		ASSERT_EQUAL( _T("a|a.doc|a.txt"), ut::UnsplitFiles( found ) );
	}
	{
		fs::CEnumerator found( pTempDirPath );
		fs::EnumFiles( &found, pTempDirPath, _T("*.*"), Deep );
		ASSERT_EQUAL( _T("a|a.doc|a.txt|d1\\b|d1\\b.doc|d1\\b.txt|d1\\d2\\c|d1\\d2\\c.doc|d1\\d2\\c.txt"), ut::UnsplitFiles( found ) );
	}
	{
		fs::CEnumerator found( pTempDirPath );
		fs::EnumFiles( &found, pTempDirPath, _T("*."), Deep );
		ASSERT_EQUAL( _T("a|d1\\b|d1\\d2\\c"), ut::UnsplitFiles( found ) );
	}
	{
		fs::CEnumerator found( pTempDirPath );
		fs::EnumFiles( &found, pTempDirPath, _T("*.doc"), Deep );
		ASSERT_EQUAL( _T("a.doc|d1\\b.doc|d1\\d2\\c.doc"), ut::UnsplitFiles( found ) );
	}
	{
		fs::CEnumerator found( pTempDirPath );
		fs::EnumFiles( &found, pTempDirPath, _T("*.doc;*.txt"), Deep );
		ASSERT_EQUAL( _T("a.doc|a.txt|d1\\b.doc|d1\\b.txt|d1\\d2\\c.doc|d1\\d2\\c.txt"), ut::UnsplitFiles( found ) );
		ASSERT_EQUAL( _T("d1|d1\\d2"), ut::UnsplitSubDirs( found ) );
	}
	{
		fs::CEnumerator found( pTempDirPath );
		fs::EnumFiles( &found, pTempDirPath, _T("*.?oc;*.t?t"), Deep );
		ASSERT_EQUAL( _T("a.doc|a.txt|d1\\b.doc|d1\\b.txt|d1\\d2\\c.doc|d1\\d2\\c.txt"), ut::UnsplitFiles( found ) );
	}
	{
		fs::CEnumerator found( pTempDirPath );
		fs::EnumFiles( &found, pTempDirPath, _T("*.exe;*.bat"), Deep );
		ASSERT_EQUAL( _T(""), ut::UnsplitFiles( found ) );
	}
}

void CFileSystemTests::TestStgShortFilenames( void )
{
	ASSERT_EQUAL( _T(""), fs::CStructuredStorage::MakeShortFilename( _T("") ) );
	ASSERT_EQUAL( _T("Not a long filename"), fs::CStructuredStorage::MakeShortFilename( _T("Not a long filename") ) );				// 19 chars
	ASSERT_EQUAL( _T("Not a long filename ABCDEFG.txt"), fs::CStructuredStorage::MakeShortFilename( _T("Not a long filename ABCDEFG.txt") ) );		// 31 chars
	ASSERT_EQUAL( _T("Documents\\Business\\Payroll\\Not a long filename.txt"), fs::CStructuredStorage::MakeShortFilename( _T("Documents\\Business\\Payroll\\Not a long filename.txt") ) );		// 23 chars

	// fs::CStructuredStorage::MaxFilenameLen overflow
	ASSERT_EQUAL( _T("This is a long fil_A7A42533.txt"), fs::CStructuredStorage::MakeShortFilename( _T("This is a long filename ABCD.txt") ) );		// 32 chars
	ASSERT_EQUAL( _T("ThisIsASuperLongFi_3B36D197.jpg"), fs::CStructuredStorage::MakeShortFilename( _T("ThisIsASuperLongFilenameOfAnUnknownImageFileThatKeepsGoing.jpg") ) );	// 62 chars
	ASSERT_EQUAL( _T("thisisasuperlongfi_3B36D197.JPG"), fs::CStructuredStorage::MakeShortFilename( _T("thisisasuperlongfilenameofanunknownimagefilethatkeepsgoing.JPG") ) );	// 62 chars

	// sub-paths
	ASSERT( fs::CStructuredStorage::MakeShortFilename( _T("my\\docs\\This is a long filename ABCDxy.txt") ) != fs::CStructuredStorage::MakeShortFilename( _T("This is a long filename ABCDxy.txt") ) );	// 34 chars
}

void CFileSystemTests::TestTempFilePool( void )
{
	{
		ut::CTempFilePairPool pool( _T("a.txt|b.txt|c.txt") );
		pool.CopySrc();
		ASSERT_EQUAL( _T("a.txt|b.txt|c.txt"), pool.UnsplitDest() );
	}
	{
		ut::CTempFilePairPool pool( _T("a.txt|d1\\b.txt|d1\\d2\\c.txt") );
		pool.CopySrc();
		ASSERT_EQUAL( _T("a.txt|b.txt|c.txt"), pool.UnsplitDest() );
	}
}

void CFileSystemTests::Run( void )
{
	TRACE( _T("-- UTL File System tests --\n") );

	TestPathUtilities();
	TestPathCompareFind();
	TestPathWildcardMatch();
	TestComplexPath();
	TestFlexPath();
	TestFileSystem();
	TestFileEnum();
	TestStgShortFilenames();
	TestTempFilePool();
}


#endif //_DEBUG
