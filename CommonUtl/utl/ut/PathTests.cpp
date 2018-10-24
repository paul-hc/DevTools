
#include "stdafx.h"
#include "ut/PathTests.h"
#include "Path.h"
#include "FlexPath.h"
#include "Resequence.hxx"
#include "StringUtilities.h"
#include "StructuredStorage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


CPathTests::CPathTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CPathTests& CPathTests::Instance( void )
{
	static CPathTests testCase;
	return testCase;
}

void CPathTests::TestPathUtilities( void )
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
		ASSERT_EQUAL( _T("X:\\Dir\\Sub"), path.GetParentPath() );
		ASSERT_EQUAL( _T("X:\\Dir\\Sub/"), path.GetParentPath( true ) );

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

void CPathTests::TestPathCompareFind( void )
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
		path::GetMatch getMatchFunc;
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
	inline const fs::CPath& PathOf( const std::pair< const ut::CPathItem*, std::string >& mapItem ) { return mapItem.first->m_path; }
}

void CPathTests::TestHasMultipleDirPaths( void )
{
	{
		std::vector< fs::CPath > paths;
		ASSERT( !path::HasMultipleDirPaths( paths ) );

		paths.push_back( fs::CPath( _T("C:\\Images\\Fruit\\apple.jpg") ) );
		ASSERT( !path::HasMultipleDirPaths( paths ) );

		paths.push_back( fs::CPath( _T("C:\\Images/Fruit/orange.jpg") ) );
		ASSERT( !path::HasMultipleDirPaths( paths ) );

		paths.push_back( fs::CPath( _T("C:\\Images\\Drink\\coffee.jpg") ) );
		ASSERT( path::HasMultipleDirPaths( paths ) );
	}

	{
		#pragma warning( disable: 4709 )	// comma operator within array index expression

		std::map< ut::CPathItem*, std::string > pathMap;

		pathMap[ new ut::CPathItem( _T("C:\\Images/Fruit/apple.jpg"), 10 ) ] = "A";
		pathMap[ new ut::CPathItem( _T("C:\\Images\\Fruit\\orange.jpg"), 20 ) ] = "B";
		ASSERT( !path::HasMultipleDirPaths( pathMap ) );

		pathMap[ new ut::CPathItem( _T("C:\\Images\\Drink\\coffee.jpg"), 30 ) ] = "Odd";
		ASSERT( path::HasMultipleDirPaths( pathMap ) );

		utl::ClearOwningAssocContainerKeys( pathMap );
	}
}

void CPathTests::TestCommonSubpath( void )
{
	{
		std::vector< fs::CPath > paths;
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
		std::vector< fs::CPath > paths;
		paths.push_back( fs::CPath( _T("C:\\Images\\Fruit\\apple.jpg") ) );
		paths.push_back( fs::CPath( _T("C:\\Images/Fruit/orange.jpg") ) );
		paths.push_back( fs::CPath( _T("C:\\Images\\Drink\\coffee.jpg") ) );

		fs::CPath commonDirPath = path::ExtractCommonParentPath( paths );

		for ( std::vector< fs::CPath >::iterator itPath = paths.begin(); itPath != paths.end(); ++itPath )
		{
			std::tstring relPath = itPath->Get();
			path::StripPrefix( relPath, commonDirPath.GetPtr() );
			*itPath = relPath;
		}

		ASSERT_EQUAL( _T("Fruit\\apple.jpg"), paths[ 0 ] );
		ASSERT_EQUAL( _T("Fruit\\orange.jpg"), paths[ 1 ] );
		ASSERT_EQUAL( _T("Drink\\coffee.jpg"), paths[ 2 ] );
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

void CPathTests::TestFlexPath( void )
{
	// complex path
	{
		fs::CFlexPath path( _T("C:\\Images\\fruit.stg>apple.jpg") );
		ASSERT( path.IsComplexPath() );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path.GetPhysicalPath() );
		ASSERT_EQUAL_STR( _T("apple.jpg"), path.GetEmbeddedPath() );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path.GetParentFlexPath() );
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
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg>Europe"), path.GetParentFlexPath() );
		ASSERT_EQUAL_STR( _T("apple.jpg"), path.GetNameExt() );

		ASSERT_EQUAL_STR( _T("Europe\\apple.jpg"), path.GetLeafSubPath() );
		ASSERT_EQUAL( _T("C:\\Images\\fruit.stg"), path.GetOriginParentPath() );

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
		ASSERT_EQUAL( _T("C:\\Images"), path.GetParentFlexPath() );
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
	static const TCHAR s_pathChars[] = _T("C:\\Images/fruit.stg>Europe/apple.jpg");
	static const size_t hashChars = stdext::hash_value( s_pathChars );

	std::tstring pathString = s_pathChars;
	ASSERT_EQUAL( hashChars, stdext::hash_value( pathString ) );

	const size_t hashPathChars = path::GetHashValue( s_pathChars );
	ASSERT( hashPathChars != hashChars );
	ASSERT_EQUAL( hashPathChars, stdext::hash_value( _T("c:\\images\\fruit.stg>europe\\apple.jpg") ) );		// lower-case and normalized backslashes

	fs::CPath path( pathString );
	ASSERT_EQUAL( hashPathChars, stdext::hash_value( path ) );

	fs::CFlexPath flexPath( pathString );
	ASSERT_EQUAL( hashPathChars, stdext::hash_value( flexPath ) );
}

#include <shlwapi.h>		// for Path... functions

void CPathTests::TestPathShellApi( void )
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


void CPathTests::Run( void )
{
	__super::Run();

	TestPathUtilities();
	TestPathCompareFind();
	TestPathWildcardMatch();
	TestHasMultipleDirPaths();
	TestCommonSubpath();
	TestComplexPath();
	TestFlexPath();
	TestPathHashValue();
	TestPathShellApi();
}


#endif //_DEBUG
