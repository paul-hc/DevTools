
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "test/StructuredStorageTest.h"
#include "FlexPath.h"
#include "FileState.h"
#include "FileEnumerator.h"
#include "StringUtilities.h"
#include "StructuredStorage.h"
#include "StorageTrail.h"

#define new DEBUG_NEW


namespace ut
{
	void BuildStorage( fs::CStructuredStorage* pDocStorage, const fs::CRelativeEnumerator& src )
	{
		ASSERT_PTR( pDocStorage );
		ASSERT( pDocStorage->IsOpen() );

		for ( std::vector< fs::CPath >::const_iterator itSubDirPath = src.m_subDirPaths.begin(); itSubDirPath != src.m_subDirPaths.end(); ++itSubDirPath )
		{
			fs::CStorageTrail subDirTrack( pDocStorage );
			ASSERT( subDirTrack.CreateDeepSubPath( itSubDirPath->GetPtr() ) );
		}

		const fs::CPath& srcDirPath = src.GetRelativeDirPath();

		for ( std::vector< fs::CPath >::const_iterator itFilePath = src.m_filePaths.begin(); itFilePath != src.m_filePaths.end(); ++itFilePath )
		{
			std::auto_ptr< CFile > pSrcFile = fs::OpenFile( srcDirPath / *itFilePath, false );
			ASSERT_PTR( pSrcFile.get() );

			fs::CPath subPath = itFilePath->GetParentPath();

			fs::CStorageTrail subDirTrack( pDocStorage );
			ASSERT( subDirTrack.OpenDeepSubPath( subPath.GetPtr(), STGM_READWRITE ) );

			fs::CPath streamName( itFilePath->GetFilename() );

			std::auto_ptr< COleStreamFile > pDestStreamFile( pDocStorage->CreateStreamFile( streamName.GetPtr(), subDirTrack.GetCurrent() ) );
			ASSERT_PTR( pDestStreamFile.get() );

			fs::BufferedCopy( *pDestStreamFile, *pSrcFile );
		}
	}

	std::tstring ReadTextFromStream( const fs::CStructuredStorage* pDocStorage, IStream* pStream )
	{
		ASSERT_PTR( pDocStorage );
		ASSERT( pDocStorage->IsOpen() );
		ASSERT_PTR( pStream );

		fs::CFileState streamState;
		ASSERT( fs::stg::GetElementFullState( streamState, pStream ) );

		size_t textLength = static_cast<size_t>( streamState.m_fileSize );
		std::vector< char > textBuffer;

		textBuffer.reserve( textLength + 1 );		// including EOS
		textBuffer.resize( textLength );

		ULONG actualRead;
		pDocStorage->Handle( pStream->Read( &textBuffer.front(), static_cast<ULONG>( textLength ), &actualRead ) );
		ASSERT_EQUAL( textLength, actualRead );

		textBuffer.push_back( '\0' );				// add EOS

		return str::FromUtf8( &textBuffer.front() );
	}
}


// CStructuredStorageTest implementation

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
	const fs::CPath docStgPath = ut::CTempFilePool::MakePoolDirPath().GetParentPath() / _T("StorageDoc.bin");

	// create storage document
	{
		ut::CTempFilePairPool pool( _T("a1.txt|a2.txt|B1\\b1.txt|B1\\b2.txt|B1\\SD\\ThisIsASuperLongFilenameOfAnUnknownImageFileThatKeepsGoing.txt") );
		const fs::CPath& poolDirPath = pool.GetPoolDirPath();

		fs::CRelativeEnumerator srcEnum( poolDirPath );
		fs::EnumFiles( &srcEnum, poolDirPath, NULL, Deep );
		ASSERT_EQUAL( _T("a1.txt|a2.txt|B1\\b1.txt|B1\\b2.txt|B1\\SD\\ThisIsASuperLongFilenameOfAnUnknownImageFileThatKeepsGoing.txt"), ut::JoinFiles( srcEnum ) );

		fs::CStructuredStorage docStorage;
		ASSERT( docStorage.CreateDocFile( docStgPath.GetPtr() ) );
		ENSURE( fs::CStructuredStorage::IsValidDocFile( docStgPath.GetPtr() ) );

		ut::BuildStorage( &docStorage, srcEnum );

		fs::CStorageTrail trail( &docStorage );
		fs::CEnumerator foundEnum;

		trail.EnumElements( &foundEnum );		// Shallow
		ASSERT_EQUAL( _T("a1.txt|a2.txt"), ut::JoinFiles( foundEnum ) );

		ENSURE( fs::CStructuredStorage::IsValidDocFile( docStgPath.GetPtr() ) );
	}

	// load storage document
	{
		ENSURE( fs::CStructuredStorage::IsValidDocFile( docStgPath.GetPtr() ) );

		fs::CStructuredStorage docStorage;
		ASSERT( docStorage.OpenDocFile( docStgPath.GetPtr() ) );

		_TestEnumerateElements( &docStorage );

		// test shared stream access
		fs::TEmbeddedPath streamPath( _T("a1.txt") );
		ASSERT( !docStorage.IsStreamOpen( streamPath ) );

		docStorage.SetUseStreamSharing( true );

		_TestOpenSharedStreams( &docStorage, streamPath );
		ASSERT( docStorage.IsStreamOpen( streamPath ) );

		if ( false )		// doesn't work yet without the current trail part of the storage - TODO...
		{
			streamPath.Set( _T("B1\\b1.txt") );
			_TestOpenSharedStreams( &docStorage, streamPath );
			ASSERT( docStorage.IsStreamOpen( streamPath ) );
		}
	}
}

void CStructuredStorageTest::_TestEnumerateElements( fs::CStructuredStorage* pDocStorage )
{
	fs::CStorageTrail trail( pDocStorage );

	// enumerate compound document
	{
		fs::CEnumerator foundEnum;
		trail.EnumElements( &foundEnum, Deep );
		ASSERT_EQUAL( _T("a1.txt|a2.txt|B1\\b1.txt|B1\\b2.txt|B1\\SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( foundEnum ) );
	}

	// enumerate a sub-storage
	{
		fs::CEnumerator foundEnum;
		trail.Push( pDocStorage->OpenDir( _T("B1") ) );

		trail.EnumElements( &foundEnum, Deep );
		ASSERT_EQUAL( _T("B1\\b1.txt|B1\\b2.txt|B1\\SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( foundEnum ) );

		{	// relative to "B1"
			fs::CRelativeEnumerator relEnum( trail.MakePath() );
			trail.EnumElements( &relEnum, Deep );
			ASSERT_EQUAL( _T("b1.txt|b2.txt|SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( relEnum ) );
		}

		trail.Push( pDocStorage->OpenDir( _T("SD"), trail.GetCurrent() ) );		// go deeper
		foundEnum.Clear();
		trail.EnumElements( &foundEnum, Deep );
		ASSERT_EQUAL( _T("B1\\SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( foundEnum ) );

		{	// relative to "B1\\SD"
			fs::CRelativeEnumerator relEnum( trail.MakePath() );
			trail.EnumElements( &relEnum, Deep );
			ASSERT_EQUAL( _T("ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( relEnum ) );
		}
	}

	// enumerate deep a sub-storage
	{
		trail.Clear();
		trail.OpenDeepSubPath( _T("B1\\SD") );

		fs::CEnumerator foundEnum;
		trail.EnumElements( &foundEnum, Deep );
		ASSERT_EQUAL( _T("B1\\SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( foundEnum ) );
	}
}

void CStructuredStorageTest::_TestOpenSharedStreams( fs::CStructuredStorage* pDocStorage, const fs::TEmbeddedPath& streamPath )
{
	// stream origin
	CComPtr< IStream > pStream_1 = pDocStorage->OpenStream( streamPath.GetPtr() );
	ASSERT_EQUAL( 2, dbg::GetRefCount( pStream_1 ) );		// 1 in CStreamState::m_pStreamOrigin + 1 local
	ASSERT( pDocStorage->IsStreamOpen( streamPath ) );

	std::tstring content = ut::ReadTextFromStream( pDocStorage, pStream_1 );
	ASSERT( !content.empty() );

	{	// cached entry
		const fs::CStreamState* pStreamState = pDocStorage->FindOpenedStream( streamPath );
		ASSERT_PTR( pStreamState );
		ASSERT_EQUAL( streamPath, pStreamState->m_fullPath );
		ASSERT_EQUAL( content.length(), pStreamState->m_fileSize );
	}

	// #2 shared duplicate stream
	CComPtr< IStream > pStream_2 = pDocStorage->OpenStream( _T("a1.txt") );
	ASSERT_EQUAL( 1, dbg::GetRefCount( pStream_2 ) );
	ASSERT_EQUAL( content, ut::ReadTextFromStream( pDocStorage, pStream_2 ) );
	ASSERT_EQUAL( 2, dbg::GetRefCount( pStream_1 ) );			// origin refCount invariant to cloning

	// #3 shared duplicate stream
	CComPtr< IStream > pStream_3 = pDocStorage->OpenStream( _T("a1.txt") );
	ASSERT_EQUAL( 1, dbg::GetRefCount( pStream_3 ) );
	ASSERT_EQUAL( content, ut::ReadTextFromStream( pDocStorage, pStream_3 ) );
	ASSERT_EQUAL( 2, dbg::GetRefCount( pStream_1 ) );			// origin refCount invariant to cloning
}


void CStructuredStorageTest::Run( void )
{
	__super::Run();

	TestLongFilenames();
	TestStructuredStorage();
}


#endif //_DEBUG
