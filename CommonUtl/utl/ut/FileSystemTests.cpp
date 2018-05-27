
#include "stdafx.h"
#include "ut/FileSystemTests.h"
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


void CFileSystemTests::TestFileSystem( void )
{
	std::tstring tempDirPath = ut::GetTempUt_DirPath().Get();
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
	const fs::CPath& tempDirPath = pool.GetPoolDirPath();

	{
		fs::CEnumerator found( tempDirPath.Get() );
		fs::EnumFiles( &found, tempDirPath.GetPtr(), _T("*.*"), Shallow );
		ASSERT_EQUAL( _T("a|a.doc|a.txt"), ut::JoinFiles( found ) );
	}
	{
		fs::CEnumerator found( tempDirPath.Get() );
		fs::EnumFiles( &found, tempDirPath.GetPtr(), _T("*.*"), Deep );
		ASSERT_EQUAL( _T("a|a.doc|a.txt|d1\\b|d1\\b.doc|d1\\b.txt|d1\\d2\\c|d1\\d2\\c.doc|d1\\d2\\c.txt"), ut::JoinFiles( found ) );
	}
	{
		fs::CEnumerator found( tempDirPath.Get() );
		fs::EnumFiles( &found, tempDirPath.GetPtr(), _T("*."), Deep );
		ASSERT_EQUAL( _T("a|d1\\b|d1\\d2\\c"), ut::JoinFiles( found ) );
	}
	{
		fs::CEnumerator found( tempDirPath.Get() );
		fs::EnumFiles( &found, tempDirPath.GetPtr(), _T("*.doc"), Deep );
		ASSERT_EQUAL( _T("a.doc|d1\\b.doc|d1\\d2\\c.doc"), ut::JoinFiles( found ) );
	}
	{
		fs::CEnumerator found( tempDirPath.Get() );
		fs::EnumFiles( &found, tempDirPath.GetPtr(), _T("*.doc;*.txt"), Deep );
		ASSERT_EQUAL( _T("a.doc|a.txt|d1\\b.doc|d1\\b.txt|d1\\d2\\c.doc|d1\\d2\\c.txt"), ut::JoinFiles( found ) );
		ASSERT_EQUAL( _T("d1|d1\\d2"), ut::JoinSubDirs( found ) );
	}
	{
		fs::CEnumerator found( tempDirPath.Get() );
		fs::EnumFiles( &found, tempDirPath.GetPtr(), _T("*.?oc;*.t?t"), Deep );
		ASSERT_EQUAL( _T("a.doc|a.txt|d1\\b.doc|d1\\b.txt|d1\\d2\\c.doc|d1\\d2\\c.txt"), ut::JoinFiles( found ) );
	}
	{
		fs::CEnumerator found( tempDirPath.Get() );
		fs::EnumFiles( &found, tempDirPath.GetPtr(), _T("*.exe;*.bat"), Deep );
		ASSERT_EQUAL( _T(""), ut::JoinFiles( found ) );
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
		ASSERT_EQUAL( _T("a.txt|b.txt|c.txt"), pool.JoinDest() );
	}
	{
		ut::CTempFilePairPool pool( _T("a.txt|d1\\b.txt|d1\\d2\\c.txt") );
		pool.CopySrc();
		ASSERT_EQUAL( _T("a.txt|b.txt|c.txt"), pool.JoinDest() );
	}
}

void CFileSystemTests::Run( void )
{
	__super::Run();

	TestFileSystem();
	TestFileEnum();
	TestStgShortFilenames();
	TestTempFilePool();
}


#endif //_DEBUG
