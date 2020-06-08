
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "test/StructuredStorageTest.h"
#include "FlexPath.h"
#include "FileState.h"
#include "FileEnumerator.h"
#include "StringUtilities.h"
#include "StructuredStorage.h"
#include "StorageTrack.h"

#define new DEBUG_NEW


namespace ut
{
	void BuildStorage( fs::CStructuredStorage* pDocStg, const fs::CRelativeEnumerator& src )
	{
		ASSERT_PTR( pDocStg );
		ASSERT( pDocStg->IsOpen() );

		for ( std::vector< fs::CPath >::const_iterator itSubDirPath = src.m_subDirPaths.begin(); itSubDirPath != src.m_subDirPaths.end(); ++itSubDirPath )
		{
			fs::CStorageTrack subDirTrack( pDocStg );
			ASSERT( subDirTrack.CreateDeepSubPath( itSubDirPath->GetPtr() ) );
		}

		const fs::CPath& srcDirPath = src.GetRelativeDirPath();

		for ( std::vector< fs::CPath >::const_iterator itFilePath = src.m_filePaths.begin(); itFilePath != src.m_filePaths.end(); ++itFilePath )
		{
			std::auto_ptr< CFile > pSrcFile = fs::OpenFile( srcDirPath / *itFilePath, false );
			ASSERT_PTR( pSrcFile.get() );

			fs::CPath subPath = itFilePath->GetParentPath();

			fs::CStorageTrack subDirTrack( pDocStg );
			ASSERT( subDirTrack.OpenDeepSubPath( subPath.GetPtr(), STGM_READWRITE ) );

			fs::CPath streamName( itFilePath->GetFilename() );

			std::auto_ptr< COleStreamFile > pDestStreamFile( pDocStg->CreateFile( streamName.GetPtr(), subDirTrack.GetCurrent() ) );
			ASSERT_PTR( pDestStreamFile.get() );

			fs::BufferedCopy( *pDestStreamFile, *pSrcFile );
		}
	}
}


CStructuredStorageTest::CStructuredStorageTest( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CStructuredStorageTest& CStructuredStorageTest::Instance( void )
{
	static CStructuredStorageTest s_testCase;
	return s_testCase;
}

void CStructuredStorageTest::TestLongFilenames( void )
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

void CStructuredStorageTest::TestStructuredStorage( void )
{
	const fs::CPath docPath = ut::CTempFilePool::MakePoolDirPath().GetParentPath() / _T("StorageDoc.bin");

	// create storage document
	{
		ut::CTempFilePairPool pool( _T("a1.txt|a2.txt|B1\\b1.txt|B1\\b2.txt|B1\\SD\\ThisIsASuperLongFilenameOfAnUnknownImageFileThatKeepsGoing.txt") );
		const fs::CPath& poolDirPath = pool.GetPoolDirPath();

		fs::CRelativeEnumerator srcEnum( poolDirPath );
		fs::EnumFiles( &srcEnum, poolDirPath, NULL, Deep );
		ASSERT_EQUAL( _T("a1.txt|a2.txt|B1\\b1.txt|B1\\b2.txt|B1\\SD\\ThisIsASuperLongFilenameOfAnUnknownImageFileThatKeepsGoing.txt"), ut::JoinFiles( srcEnum ) );

		fs::CStructuredStorage docStg;
		ASSERT( docStg.CreateDocFile( docPath.GetPtr() ) );

		ut::BuildStorage( &docStg, srcEnum );

		fs::CStorageTrack track( &docStg );
		fs::CEnumerator foundEnum;

		track.EnumElements( &foundEnum );		// Shallow
		ASSERT_EQUAL( _T("a1.txt|a2.txt"), ut::JoinFiles( foundEnum ) );
	}

	// load storage document
	{
		fs::CStructuredStorage docStg;
		ASSERT( docStg.OpenDocFile( docPath.GetPtr() ) );

		_TestEnumerateElements( &docStg );
	}
}

void CStructuredStorageTest::_TestEnumerateElements( fs::CStructuredStorage* pDocStg )
{
	fs::CStorageTrack track( pDocStg );

	// enumerate compound document
	{
		fs::CEnumerator foundEnum;
		track.EnumElements( &foundEnum, Deep );
		ASSERT_EQUAL( _T("a1.txt|a2.txt|B1\\b1.txt|B1\\b2.txt|B1\\SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( foundEnum ) );
	}

	// enumerate a sub-storage
	{
		fs::CEnumerator foundEnum;
		track.Push( pDocStg->OpenDir( _T("B1") ) );

		track.EnumElements( &foundEnum, Deep );
		ASSERT_EQUAL( _T("B1\\b1.txt|B1\\b2.txt|B1\\SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( foundEnum ) );

		{	// relative to "B1"
			fs::CRelativeEnumerator relEnum( track.MakeSubPath() );
			track.EnumElements( &relEnum, Deep );
			ASSERT_EQUAL( _T("b1.txt|b2.txt|SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( relEnum ) );
		}

		track.Push( pDocStg->OpenDir( _T("SD"), track.GetCurrent() ) );		// go deeper
		foundEnum.Clear();
		track.EnumElements( &foundEnum, Deep );
		ASSERT_EQUAL( _T("B1\\SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( foundEnum ) );

		{	// relative to "B1\\SD"
			fs::CRelativeEnumerator relEnum( track.MakeSubPath() );
			track.EnumElements( &relEnum, Deep );
			ASSERT_EQUAL( _T("ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( relEnum ) );
		}
	}

	// enumerate deep a sub-storage
	{
		track.Clear();
		track.OpenDeepSubPath( _T("B1\\SD") );

		fs::CEnumerator foundEnum;
		track.EnumElements( &foundEnum, Deep );
		ASSERT_EQUAL( _T("B1\\SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( foundEnum ) );
	}
}


void CStructuredStorageTest::Run( void )
{
	__super::Run();

	TestLongFilenames();
	TestStructuredStorage();
}


#endif //_DEBUG
