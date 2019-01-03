
#include "stdafx.h"
#include "ut/FileSystemTests.h"
#include "Path.h"
#include "FlexPath.h"
#include "FileState.h"
#include "FileContent.h"
#include "TimeUtils.h"
#include "Resequence.hxx"
#include "StringUtilities.h"
#include "StructuredStorage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


namespace ut
{
	static const CTime s_ct( 2017, 7, 1, 14, 10, 0 );
	static const CTime s_mt( 2017, 7, 1, 14, 20, 0 );
	static const CTime s_at( 2017, 7, 1, 14, 30, 0 );
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
	ut::CTempFilePool pool( _T("a|a.txt|a.doc|a.jpg|a.png|D1\\b|D1\\b.doc|D1\\b.txt|D1\\D2\\c|D1/D2/c.doc|D1\\D2\\c.png|D1\\D2\\c.txt") );
	const fs::CPath& poolDirPath = pool.GetPoolDirPath();

	{
		fs::CRelativeEnumerator found( poolDirPath );
		fs::EnumFiles( &found, poolDirPath, _T("*.*"), Shallow );
		ASSERT_EQUAL( _T("a|a.doc|a.jpg|a.png|a.txt"), ut::JoinFiles( found ) );
	}
	{
		fs::CRelativeEnumerator found( poolDirPath );
		fs::EnumFiles( &found, poolDirPath, _T("*.*"), Deep );
		ASSERT_EQUAL( _T("a|a.doc|a.jpg|a.png|a.txt|D1\\b|D1\\b.doc|D1\\b.txt|D1\\D2\\c|D1\\D2\\c.doc|D1\\D2\\c.png|D1\\D2\\c.txt"), ut::JoinFiles( found ) );
	}
	{
		fs::CRelativeEnumerator found( poolDirPath );
		fs::EnumFiles( &found, poolDirPath, _T("*."), Deep );			// filter files with no extension
		ASSERT_EQUAL( _T("a|D1\\b|D1\\D2\\c"), ut::JoinFiles( found ) );
	}
	{
		fs::CRelativeEnumerator found( poolDirPath );
		fs::EnumFiles( &found, poolDirPath, _T("*.doc"), Deep );
		ASSERT_EQUAL( _T("a.doc|D1\\b.doc|D1\\D2\\c.doc"), ut::JoinFiles( found ) );
	}
	{
		fs::CRelativeEnumerator found( poolDirPath );
		fs::EnumFiles( &found, poolDirPath, _T("*.doc;*.txt"), Deep );
		ASSERT_EQUAL( _T("a.doc|a.txt|D1\\b.doc|D1\\b.txt|D1\\D2\\c.doc|D1\\D2\\c.txt"), ut::JoinFiles( found ) );
		ASSERT_EQUAL( _T("D1|D1\\D2"), ut::JoinSubDirs( found ) );
	}
	{
		fs::CRelativeEnumerator found( poolDirPath );
		fs::EnumFiles( &found, poolDirPath, _T("*.?oc;*.t?t"), Deep );
		ASSERT_EQUAL( _T("a.doc|a.txt|D1\\b.doc|D1\\b.txt|D1\\D2\\c.doc|D1\\D2\\c.txt"), ut::JoinFiles( found ) );
	}
	{
		fs::CRelativeEnumerator found( poolDirPath );
		fs::EnumFiles( &found, poolDirPath, _T("*.jpg;*.png"), Deep );
		ASSERT_EQUAL( _T("a.jpg|a.png|D1\\D2\\c.png"), ut::JoinFiles( found ) );
		ASSERT_EQUAL( _T("D1|D1\\D2"), ut::JoinSubDirs( found ) );
	}
	{
		fs::CRelativeEnumerator found( poolDirPath );
		fs::EnumFiles( &found, poolDirPath, _T("*.jpg;*.png"), Shallow );
		ASSERT_EQUAL( _T("a.jpg|a.png"), ut::JoinFiles( found ) );
		ASSERT_EQUAL( _T("D1"), ut::JoinSubDirs( found ) );				// only the shallow subdirs
	}
	{
		fs::CRelativeEnumerator found( poolDirPath );
		fs::EnumFiles( &found, poolDirPath, _T("*.exe;*.bat"), Deep );
		ASSERT_EQUAL( _T(""), ut::JoinFiles( found ) );
	}

	ASSERT_EQUAL( _T("a"), ut::FindFirstFile( poolDirPath, _T("*"), Deep ) );
	ASSERT_EQUAL( _T("a"), ut::FindFirstFile( poolDirPath, _T("*.*"), Shallow ) );
	ASSERT_EQUAL( _T("a"), ut::FindFirstFile( poolDirPath, _T("*."), Shallow ) );
	ASSERT_EQUAL( _T("a.png"), ut::FindFirstFile( poolDirPath, _T("*.png"), Shallow ) );
	ASSERT_EQUAL( _T("a.png"), ut::FindFirstFile( poolDirPath, _T("a*.png"), Shallow ) );
	ASSERT_EQUAL( _T("a.png"), ut::FindFirstFile( poolDirPath, _T("*a*.png"), Shallow ) );
	ASSERT_EQUAL( _T(""), ut::FindFirstFile( poolDirPath, _T("c.png"), Shallow ) );
	ASSERT_EQUAL( _T("D1\\D2\\c.png"), ut::FindFirstFile( poolDirPath, _T("c.png"), Deep ) );

	ASSERT_EQUAL( _T(""), ut::FindFirstFile( poolDirPath, _T("*.exe"), Deep ) );
}

void CFileSystemTests::TestNumericFilename( void )
{
	const fs::CPath& poolDirPath = ut::CTempFilePool::MakePoolDirPath();

	const fs::CPath seedFilePath = ut::CTempFilePool::MakePoolDirPath() / fs::CPath( _T("SeedFile.txt") );
	if ( seedFilePath.FileExist() )
		fs::DeleteFile( seedFilePath.GetPtr() );

	ASSERT_EQUAL( _T("SeedFile.txt"), fs::StripDirPrefix( fs::MakeUniqueNumFilename( seedFilePath ), poolDirPath ) );

	ut::CTempFilePool pool( _T("SeedFile.txt") );
	ASSERT_EQUAL( _T("SeedFile_[2].txt"), fs::StripDirPrefix( fs::MakeUniqueNumFilename( seedFilePath ), poolDirPath ) );

	pool.CreateFiles( _T("SeedFile_ABC.txt") );
	ASSERT_EQUAL( _T("SeedFile_[3].txt"), fs::StripDirPrefix( fs::MakeUniqueNumFilename( seedFilePath ), poolDirPath ) );

	pool.CreateFiles( _T("SeedFile_5.txt|SeedFile7.txt") );
	ASSERT_EQUAL( _T("SeedFile_[8].txt"), fs::StripDirPrefix( fs::MakeUniqueNumFilename( seedFilePath ), poolDirPath ) );

	pool.CreateFiles( _T("SeedFile_abc30xyz.txt") );
	ASSERT_EQUAL( _T("SeedFile_[31].txt"), fs::StripDirPrefix( fs::MakeUniqueNumFilename( seedFilePath ), poolDirPath ) );

	pool.CreateFiles( _T("SeedFile5870.txt|SeedFile133.txt") );
	ASSERT_EQUAL( _T("SeedFile_[5871].txt"), fs::StripDirPrefix( fs::MakeUniqueNumFilename( seedFilePath ), poolDirPath ) );
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
		ut::CTempFilePairPool pool( _T("a.txt|D1\\b.txt|D1\\D2\\c.txt") );
		pool.CopySrc();
		ASSERT_EQUAL( _T("a.txt|b.txt|c.txt"), pool.JoinDest() );
	}
}

void CFileSystemTests::TestFileAndDirectoryState( void )
{
	ut::CTempFilePool pool( _T("fa.txt|D1\\fb.txt") );

	{
		const fs::CPath& filePath = pool.GetFilePaths()[ 0 ];
		fs::CFileState fileState = fs::CFileState::ReadFromFile( filePath );
		ASSERT_EQUAL( CFile::archive, fileState.m_attributes );

		fs::CFileState newFileState;

		SetFlag( fileState.m_attributes, CFile::readOnly );
		fileState.WriteToFile();
		newFileState = fs::CFileState::ReadFromFile( filePath );

		ASSERT_EQUAL( CFile::readOnly | CFile::archive, newFileState.m_attributes );
		ASSERT_EQUAL( fileState, newFileState );

		fileState.m_creationTime = ut::s_ct;
		fileState.WriteToFile();
		newFileState = fs::CFileState::ReadFromFile( filePath );

		ASSERT_EQUAL( ut::s_ct, newFileState.m_creationTime );
		ASSERT_EQUAL( fileState, newFileState );

		fileState.m_modifTime = ut::s_mt;
		fileState.WriteToFile();
		newFileState = fs::CFileState::ReadFromFile( filePath );

		ASSERT_EQUAL( ut::s_mt, newFileState.m_modifTime );
		ASSERT_EQUAL( fileState, newFileState );

		fileState.m_accessTime = ut::s_at;
		fileState.WriteToFile();
		newFileState = fs::CFileState::ReadFromFile( filePath );

		ASSERT_EQUAL( ut::s_at, newFileState.m_accessTime );
		ASSERT_EQUAL( fileState, newFileState );
	}

	{
		fs::CPath dirPath = pool.GetFilePaths()[ 1 ].GetParentPath();
		fs::CFileState dirState = fs::CFileState::ReadFromFile( dirPath );
		ASSERT_EQUAL( CFile::directory, dirState.m_attributes );

		fs::CFileState newDirState;

		SetFlag( dirState.m_attributes, CFile::readOnly );
		dirState.WriteToFile();
		newDirState = fs::CFileState::ReadFromFile( dirPath );

		ASSERT_EQUAL( CFile::readOnly | CFile::directory, newDirState.m_attributes );
		ASSERT_EQUAL( dirState, newDirState );

		dirState.m_creationTime = ut::s_ct;
		dirState.WriteToFile();
		newDirState = fs::CFileState::ReadFromFile( dirPath );

		ASSERT_EQUAL( ut::s_ct, newDirState.m_creationTime );
		ASSERT_EQUAL( dirState, newDirState );

		dirState.m_modifTime = ut::s_mt;
		dirState.WriteToFile();
		newDirState = fs::CFileState::ReadFromFile( dirPath );

		ASSERT_EQUAL( ut::s_mt, newDirState.m_modifTime );
		ASSERT_EQUAL( dirState, newDirState );

		dirState.m_accessTime = ut::s_at;
		dirState.WriteToFile();
		newDirState = fs::CFileState::ReadFromFile( dirPath );

		ASSERT_EQUAL( ut::s_at, newDirState.m_accessTime );
		ASSERT_EQUAL( dirState, newDirState );
	}
}

void CFileSystemTests::TestTouchFile( void )
{
	ut::CTempFilePool pool( _T("name.txt") );
	const fs::CPath filePath = pool.GetPoolDirPath() / fs::CPath( _T("name.txt") );

	CTime lastModifiedTime = fs::ReadLastModifyTime( filePath );
	ASSERT_EQUAL( CTime::GetCurrentTime(), lastModifiedTime );

	lastModifiedTime += CTimeSpan( 0, 0, 0, 30 );		// add 30 seconds
	fs::thr::TouchFile( filePath, lastModifiedTime );
	ASSERT_EQUAL( lastModifiedTime, fs::ReadLastModifyTime( filePath ) );
}

void CFileSystemTests::TestFileTransferMatch( void )
{
	ut::CTempFilePool pool( _T("file.txt|item.txt") );
	const fs::CPath& filePath = pool.GetFilePaths()[ 0 ];
	const fs::CPath destPath = pool.GetPoolDirPath() / fs::CPath( _T("fileX.txt") );		// non-existent file

	ASSERT_EQUAL( fs::NoDestFile, fs::EvalTransferMatch( filePath, destPath ) );
	ASSERT_EQUAL( fs::NoSrcFile, fs::EvalTransferMatch( destPath, filePath ) );

	{
		const fs::CPath& itemPath = pool.GetFilePaths()[ 1 ];		// same timestamp, same size, different content (CRC32)
		ASSERT_EQUAL( fs::SrcUpToDate, fs::EvalTransferMatch( filePath, itemPath ) );
		ASSERT_EQUAL( fs::Crc32Mismatch, fs::EvalTransferMatch( filePath, itemPath, true, fs::FileSizeAndCrc32 ) );
	}

	fs::thr::CopyFile( filePath.GetPtr(), destPath.GetPtr(), true );

	ASSERT_EQUAL( fs::SrcUpToDate, fs::EvalTransferMatch( filePath, destPath ) );
	ASSERT_EQUAL( fs::SrcUpToDate, fs::EvalTransferMatch( filePath, destPath, false ) );
	ASSERT_EQUAL( fs::SrcUpToDate, fs::EvalTransferMatch( filePath, destPath, true, fs::FileSize ) );
	ASSERT_EQUAL( fs::SrcUpToDate, fs::EvalTransferMatch( filePath, destPath, true, fs::FileSizeAndCrc32 ) );

	fs::thr::TouchFileBy( filePath, 30 );						// change SRC timestamp by adding 30 seconds
	ASSERT_EQUAL( fs::SrcFileNewer, fs::EvalTransferMatch( filePath, destPath, true ) );
	ASSERT_EQUAL( fs::SrcUpToDate, fs::EvalTransferMatch( filePath, destPath, false, fs::FileSize ) );

	ut::ModifyFileText( destPath );			// change DEST content
	ASSERT_EQUAL( fs::SizeMismatch, fs::EvalTransferMatch( filePath, destPath, false ) );
}

void CFileSystemTests::TestBackupFile( void )
{
	ut::CTempFilePool pool( _T("src.txt") );
	const fs::CPath& srcPath = pool.GetFilePaths()[ 0 ];

	fs::CPath bkFilePath;
	{
		fs::CFileBackup backup( srcPath );
		ASSERT( !backup.FindFirstDuplicateFile( bkFilePath ) );

		ASSERT_EQUAL( fs::Created, backup.CreateBackupFile( bkFilePath ) );
		ASSERT_EQUAL( _T("src-[2].txt"), bkFilePath.GetFilename() );

		bkFilePath.Clear();
		ASSERT_EQUAL( fs::FoundExisting, backup.CreateBackupFile( bkFilePath ) );
		ASSERT_EQUAL( _T("src-[2].txt"), bkFilePath.GetFilename() );

		ut::ModifyFileText( srcPath );			// change SRC content

		ASSERT_EQUAL( fs::Created, backup.CreateBackupFile( bkFilePath ) );
		ASSERT_EQUAL( _T("src-[3].txt"), bkFilePath.GetFilename() );

		bkFilePath.Clear();
		ASSERT_EQUAL( fs::FoundExisting, backup.CreateBackupFile( bkFilePath ) );
		ASSERT_EQUAL( _T("src-[3].txt"), bkFilePath.GetFilename() );
	}

	{
		fs::CFileBackup backup( srcPath, fs::CPath( _T("SubDirA\\SubDirB") ) );
		ASSERT( !backup.FindFirstDuplicateFile( bkFilePath ) );

		ASSERT_EQUAL( fs::Created, backup.CreateBackupFile( bkFilePath ) );
		ASSERT_EQUAL( _T("SubDirA\\SubDirB\\src.txt"), fs::StripDirPrefix( bkFilePath, pool.GetPoolDirPath() ) );

		ut::ModifyFileText( srcPath );			// change SRC content
		ASSERT_EQUAL( fs::Created, backup.CreateBackupFile( bkFilePath ) );
		ASSERT_EQUAL( _T("SubDirA\\SubDirB\\src-[2].txt"), fs::StripDirPrefix( bkFilePath, pool.GetPoolDirPath() ) );
	}
}


void CFileSystemTests::Run( void )
{
	__super::Run();

	TestFileSystem();
	TestFileEnum();
	TestNumericFilename();
	TestStgShortFilenames();
	TestTempFilePool();
	TestFileAndDirectoryState();
	TestTouchFile();
	TestFileTransferMatch();
	TestBackupFile();
}


#endif //_DEBUG
