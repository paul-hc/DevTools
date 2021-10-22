
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "test/StructuredStorageTest.h"
#include "FlexPath.h"
#include "FileState.h"
#include "FileEnumerator.h"
#include "StringUtilities.h"
#include "StructuredStorage.h"
#include <hash_set>

#define new DEBUG_NEW


namespace ut
{
	void BuildStorage( fs::CStructuredStorage* pDocStorage, const fs::CRelativeEnumerator& src )
	{
		ASSERT_PTR( pDocStorage );
		ASSERT( pDocStorage->IsOpen() );

		for ( std::vector< fs::CPath >::const_iterator itSubDirPath = src.m_subDirPaths.begin(); itSubDirPath != src.m_subDirPaths.end(); ++itSubDirPath )
			ASSERT( pDocStorage->MakeDirPath( itSubDirPath->GetPtr(), false ) );

		const fs::CPath& srcDirPath = src.GetRelativeDirPath();

		fs::CStructuredStorage::CScopedCurrentDir scopedCurrDir( pDocStorage, fs::CStructuredStorage::s_rootFolderName, STGM_READWRITE );
		stdext::hash_set< fs::TEmbeddedPath > createdFolderPaths;

		for ( std::vector< fs::CPath >::const_iterator itFilePath = src.m_filePaths.begin(); itFilePath != src.m_filePaths.end(); ++itFilePath )
		{
			std::auto_ptr< CFile > pSrcFile = fs::OpenFile( srcDirPath / *itFilePath, false );
			ASSERT_PTR( pSrcFile.get() );

			fs::TEmbeddedPath parentFolderPath = itFilePath->GetParentPath();
			if ( createdFolderPaths.insert( parentFolderPath ).second )									// first time insertion
				pDocStorage->ResetToRootCurrentDir().MakeDirPath( parentFolderPath.GetPtr(), true );	// make the folder storage

			const TCHAR* pStreamName = itFilePath->GetFilenamePtr();

			std::auto_ptr< COleStreamFile > pDestStreamFile( pDocStorage->CreateStreamFile( pStreamName ) );
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


	namespace pred
	{
		typedef std::pair< STGTY, fs::CPath > TTypeFilenamePair;

		static TTypeFilenamePair s_match( STGTY_STREAM, fs::CPath() );

		static bool IsElementMatch( const fs::TEmbeddedPath& elementPath, const STATSTG& stgStat )
		{
			return
				s_match.first == (STGTY)stgStat.type &&
				s_match.second == elementPath.GetFilename();
		}
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
	ASSERT_EQUAL( _T("Not a long filename.txt"), fs::CStructuredStorage::MakeShortFilename( _T("Not a long filename.txt") ) );		// 23 chars
	ASSERT_EQUAL( _T("Not a long filename ABCDEFG.txt"), fs::CStructuredStorage::MakeShortFilename( _T("Not a long filename ABCDEFG.txt") ) );		// 31 chars

	// fs::CStructuredStorage::MaxFilenameLen overflow
	ASSERT_EQUAL( _T("This is a long fil_A7A42533.txt"), fs::CStructuredStorage::MakeShortFilename( _T("This is a long filename ABCD.txt") ) );		// 32 chars
	ASSERT_EQUAL( _T("ThisIsASuperLongFi_3B36D197.jpg"), fs::CStructuredStorage::MakeShortFilename( _T("ThisIsASuperLongFilenameOfAnUnknownImageFileThatKeepsGoing.jpg") ) );	// 62 chars
	ASSERT_EQUAL( _T("thisisasuperlongfi_3B36D197.JPG"), fs::CStructuredStorage::MakeShortFilename( _T("thisisasuperlongfilenameofanunknownimagefilethatkeepsgoing.JPG") ) );	// 62 chars

	// sub-paths
	ASSERT( fs::CStructuredStorage::MakeShortFilename( _T("my|docs|This is a long filename ABCDxy.txt") ) != fs::CStructuredStorage::MakeShortFilename( _T("This is a long filename ABCDxy.txt") ) );	// 34 chars
}

void CStructuredStorageTest::TestStructuredStorage( void )
{
	const fs::CPath docStgPath = ut::CTempFilePool::MakePoolDirPath().GetParentPath() / _T("StorageDoc.bin");

	// create storage document
	{
		ut::CTempFilePairPool pool( _T("a1.txt|a2.txt|B1\\b1.txt|B1\\b2.txt|B1\\SD\\ThisIsASuperLongFilenameOfAnUnknownImageFileThatKeepsGoing.txt") );
		const fs::CPath& poolDirPath = pool.GetPoolDirPath();

		fs::CRelativeEnumerator srcEnum( poolDirPath );
		fs::EnumFiles( &srcEnum, poolDirPath, NULL, fs::EF_Recurse );
		ASSERT_EQUAL( _T("a1.txt|a2.txt|B1\\b1.txt|B1\\b2.txt|B1\\SD\\ThisIsASuperLongFilenameOfAnUnknownImageFileThatKeepsGoing.txt"), ut::JoinFiles( srcEnum ) );

		fs::CStructuredStorage docStorage;
		ASSERT( docStorage.CreateDocFile( docStgPath ) );
		ENSURE( fs::IsValidStructuredStorage( docStgPath.GetPtr() ) );

		ut::BuildStorage( &docStorage, srcEnum );

		fs::CEnumerator foundEnum;

		docStorage.EnumElements( &foundEnum );		// Shallow
		ASSERT_EQUAL( _T("a1.txt|a2.txt"), ut::JoinFiles( foundEnum ) );

		ENSURE( fs::IsValidStructuredStorage( docStgPath.GetPtr() ) );
	}

	// load storage document
	{
		ENSURE( fs::IsValidStructuredStorage( docStgPath.GetPtr() ) );

		fs::CStructuredStorage docStorage;
		ASSERT( docStorage.OpenDocFile( docStgPath ) );

		_TestEnumerateElements( &docStorage );
		_TestFindElements( &docStorage );

		// test shared stream access
		{
			// stream sharing OFF
			fs::TEmbeddedPath streamPath( _T("a1.txt") );

			ASSERT( !docStorage.UseStreamSharing() );
			ASSERT( !docStorage.IsStreamOpen( streamPath ) );		// was not cached
			{
				CComPtr< IStream > pStream_a1 = docStorage.OpenStream( streamPath.GetFilenamePtr() );
				ASSERT_PTR( pStream_a1 );
			}
			ASSERT( docStorage.IsStreamOpen( streamPath ) );						// was cached
			ASSERT( !docStorage.FindOpenedStream( streamPath )->HasStream() );		// no shared stream

			// stream sharing ON
			docStorage.SetUseStreamSharing( true );

			_TestOpenSharedStreams( &docStorage, streamPath );
			ASSERT( docStorage.IsStreamOpen( streamPath ) );

			streamPath.Set( _T("B1\\b1.txt") );
			_TestOpenSharedStreams( &docStorage, streamPath );
			ASSERT( docStorage.IsStreamOpen( streamPath ) );

			// stream sharing OFF
			docStorage.SetUseStreamSharing( false );
		}

		// locate deep streams
		ASSERT_PTR( docStorage.LocateReadStream( fs::TEmbeddedPath( _T("a1.txt") ) ).get() );
		ASSERT_PTR( docStorage.LocateReadStream( fs::TEmbeddedPath( _T("B1\\b2.txt") ) ).get() );
		ASSERT_PTR( docStorage.LocateReadStream( fs::TEmbeddedPath( _T("B1\\SD\\ThisIsASuperLongFilenameOfAnUnknownImageFileThatKeepsGoing.txt") ) ).get() );
	}
}

void CStructuredStorageTest::_TestEnumerateElements( fs::CStructuredStorage* pDocStorage )
{
	fs::CStructuredStorage::CScopedCurrentDir scopedRootDir( pDocStorage );

	// enumerate compound document
	{
		fs::CEnumerator foundEnum;
		pDocStorage->EnumElements( &foundEnum, Deep );
		ASSERT_EQUAL( _T("a1.txt|a2.txt|B1\\b1.txt|B1\\b2.txt|B1\\SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( foundEnum ) );
	}

	// enumerate a sub-storage
	{
		fs::CEnumerator foundEnum;
		pDocStorage->ChangeCurrentDir( _T("B1") );

		pDocStorage->EnumElements( &foundEnum, Deep );
		ASSERT_EQUAL( _T("B1\\b1.txt|B1\\b2.txt|B1\\SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( foundEnum ) );

		{
			fs::CRelativeEnumerator relEnum( pDocStorage->GetCurrentDirPath() );		// relative to "B1"
			pDocStorage->EnumElements( &relEnum, Deep );
			ASSERT_EQUAL( _T("b1.txt|b2.txt|SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( relEnum ) );
		}

		pDocStorage->ChangeCurrentDir( _T("SD") );		// go deeper to "SD"
		foundEnum.Clear();
		pDocStorage->EnumElements( &foundEnum, Deep );
		ASSERT_EQUAL( _T("B1\\SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( foundEnum ) );

		{
			fs::CRelativeEnumerator relEnum( pDocStorage->GetCurrentDirPath() );		// relative to "B1\\SD"
			pDocStorage->EnumElements( &relEnum, Deep );
			ASSERT_EQUAL( _T("ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( relEnum ) );
		}
	}

	// enumerate deep a sub-storage
	{
		pDocStorage->ResetToRootCurrentDir();
		pDocStorage->ChangeCurrentDir( _T("B1\\SD") );

		fs::CEnumerator foundEnum;
		pDocStorage->EnumElements( &foundEnum, Deep );
		ASSERT_EQUAL( _T("B1\\SD\\ThisIsASuperLongFi_478086F2.txt"), ut::JoinFiles( foundEnum ) );
	}
}

void CStructuredStorageTest::_TestFindElements( fs::CStructuredStorage* pDocStorage )
{
	fs::CStructuredStorage::CScopedCurrentDir scopedRootDir( pDocStorage );
	fs::TEmbeddedPath foundPath;

	ut::pred::TTypeFilenamePair& rMatch = ut::pred::s_match;

	// find root stream
	rMatch.first = STGTY_STREAM;
	rMatch.second.Set( _T("a2.txt") );
	ASSERT( pDocStorage->FindFirstElementThat( foundPath, &ut::pred::IsElementMatch, Shallow ) );
	ASSERT_EQUAL( _T("a2.txt"), foundPath );
	foundPath.Clear();

	// find deep stream
	rMatch.second.Set( _T("b2.txt") );
	ASSERT( !pDocStorage->FindFirstElementThat( foundPath, &ut::pred::IsElementMatch, Shallow ) );

	ASSERT( pDocStorage->FindFirstElementThat( foundPath, &ut::pred::IsElementMatch, Deep ) );
	ASSERT_EQUAL( _T("B1\\b2.txt"), foundPath );
	foundPath.Clear();

	// find root STORAGE
	rMatch.first = STGTY_STORAGE;
	rMatch.second.Set( _T("GIGI") );			// inexistent
	ASSERT( !pDocStorage->FindFirstElementThat( foundPath, &ut::pred::IsElementMatch, Shallow ) );

	rMatch.second.Set( _T("B1") );
	ASSERT( pDocStorage->FindFirstElementThat( foundPath, &ut::pred::IsElementMatch, Shallow ) );
	ASSERT_EQUAL( _T("B1"), foundPath );
	foundPath.Clear();

	rMatch.second.Set( _T("SD") );
	ASSERT( !pDocStorage->FindFirstElementThat( foundPath, &ut::pred::IsElementMatch, Shallow ) );

	ASSERT( pDocStorage->FindFirstElementThat( foundPath, &ut::pred::IsElementMatch, Deep ) );
	ASSERT_EQUAL( _T("B1\\SD"), foundPath );
}

void CStructuredStorageTest::_TestOpenSharedStreams( fs::CStructuredStorage* pDocStorage, const fs::TEmbeddedPath& streamPath )
{
	fs::TEmbeddedPath streamFolderPath = streamPath.GetParentPath();
	const TCHAR* pStreamName = streamPath.GetFilenamePtr();

	fs::CStructuredStorage::CScopedCurrentDir scopedDir( pDocStorage, streamFolderPath.GetPtr() );

	// stream origin
	CComPtr< IStream > pStream_1 = pDocStorage->OpenStream( pStreamName );
	ASSERT_EQUAL( 2, dbg::GetRefCount( pStream_1 ) );		// 1 in CStreamState::m_pStreamOrigin + 1 local
	ASSERT( pDocStorage->IsStreamOpen( streamPath ) );

	std::tstring content = ut::ReadTextFromStream( pDocStorage, pStream_1 );
	ASSERT( !content.empty() );

	{	// cached entry
		const fs::CStreamState* pStreamState = pDocStorage->FindOpenedStream( streamPath );
		ASSERT_PTR( pStreamState );
		ASSERT_EQUAL( streamPath.GetFilename(), pStreamState->m_fullPath );
		ASSERT_EQUAL( content.length(), pStreamState->m_fileSize );
	}

	// #2 shared duplicate stream
	CComPtr< IStream > pStream_2 = pDocStorage->OpenStream( pStreamName );
	ASSERT_EQUAL( 1, dbg::GetRefCount( pStream_2 ) );
	ASSERT_EQUAL( content, ut::ReadTextFromStream( pDocStorage, pStream_2 ) );

	ASSERT_EQUAL( 2, dbg::GetRefCount( pStream_1 ) );			// origin refCount invariant to cloning

	// #3 shared duplicate stream
	CComPtr< IStream > pStream_3 = pDocStorage->OpenStream( pStreamName );
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
