
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/FileSystemTests.h"
#include "test/TempFilePairPool.h"
#include "Path.h"
#include "FileState.h"
#include "FileContent.h"
#include "FileEnumerator.h"
#include "StringUtilities.h"
#include "IoBin.h"
#include "TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "Resequence.hxx"


namespace ut
{
	// Pick test dates outside summer Daylight Savings - there are incompatibility issues starting with Visual C++ 2022,
	// due to CTime( const FILETIME& fileTime, int nDST = -1 ) constructor implementation!
	//
	static const CTime s_creationTime	= time_utl::ParseTimestamp( _T("29-11-2017 14:10:00") );
	static const CTime s_modifTime		= time_utl::ParseTimestamp( _T("29-11-2017 14:20:00") );
	static const CTime s_accessTime		= time_utl::ParseTimestamp( _T("29-11-2017 14:30:00") );

	static const fs::TEnumFlags s_recurse( fs::EF_Recurse );

	void testBackupFileFlat( fs::FileContentMatch matchBy );
	void testBackupFileSubDir( fs::FileContentMatch matchBy );
}


CFileSystemTests::CFileSystemTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CFileSystemTests& CFileSystemTests::Instance( void )
{
	static CFileSystemTests s_testCase;
	return s_testCase;
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
	const fs::TDirPath& poolDirPath = pool.GetPoolDirPath();

	{
		fs::CRelativePathEnumerator found( poolDirPath );
		fs::EnumFiles( &found, poolDirPath, _T("*.*") );
		ASSERT_EQUAL( _T("a|a.doc|a.jpg|a.png|a.txt"), ut::JoinFiles( found ) );
	}
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		fs::EnumFiles( &found, poolDirPath, _T("*.*") );
		ASSERT_EQUAL( _T("a|a.doc|a.jpg|a.png|a.txt|D1\\b|D1\\b.doc|D1\\b.txt|D1\\D2\\c|D1\\D2\\c.doc|D1\\D2\\c.png|D1\\D2\\c.txt"), ut::JoinFiles( found ) );
	}
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		fs::EnumFiles( &found, poolDirPath, _T("*.") );						// filter files with no extension
		ASSERT_EQUAL( _T("a|D1\\b|D1\\D2\\c"), ut::JoinFiles( found ) );
	}
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		fs::EnumFiles( &found, poolDirPath, _T("*.doc") );
		ASSERT_EQUAL( _T("a.doc|D1\\b.doc|D1\\D2\\c.doc"), ut::JoinFiles( found ) );
	}
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		fs::EnumFiles( &found, poolDirPath, _T("*.doc;*.txt") );
		ASSERT_EQUAL( _T("a.doc|a.txt|D1\\b.doc|D1\\b.txt|D1\\D2\\c.doc|D1\\D2\\c.txt"), ut::JoinFiles( found ) );
		ASSERT_EQUAL( _T("D1|D1\\D2"), ut::JoinSubDirs( found ) );
	}
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		fs::EnumFiles( &found, poolDirPath, _T("*.?oc;*.t?t") );
		ASSERT_EQUAL( _T("a.doc|a.txt|D1\\b.doc|D1\\b.txt|D1\\D2\\c.doc|D1\\D2\\c.txt"), ut::JoinFiles( found ) );
	}
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		fs::EnumFiles( &found, poolDirPath, _T("*.jpg;*.png") );
		ASSERT_EQUAL( _T("a.jpg|a.png|D1\\D2\\c.png"), ut::JoinFiles( found ) );
		ASSERT_EQUAL( _T("D1|D1\\D2"), ut::JoinSubDirs( found ) );
	}
	{
		fs::CRelativePathEnumerator found( poolDirPath );
		fs::EnumFiles( &found, poolDirPath, _T("*.jpg;*.png") );
		ASSERT_EQUAL( _T("a.jpg|a.png"), ut::JoinFiles( found ) );
		ASSERT_EQUAL( _T("D1"), ut::JoinSubDirs( found ) );				// only shallow subdirs
	}
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		fs::EnumFiles( &found, poolDirPath, _T("*.exe;*.bat") );
		ASSERT_EQUAL( _T(""), ut::JoinFiles( found ) );
	}


	ASSERT_EQUAL( _T("a"), ut::FindFirstFile( poolDirPath, _T("*"), fs::EF_Recurse ) );
	ASSERT_EQUAL( _T("a"), ut::FindFirstFile( poolDirPath, _T("*.*") ) );
	ASSERT_EQUAL( _T("a"), ut::FindFirstFile( poolDirPath, _T("*.") ) );
	ASSERT_EQUAL( _T("a.png"), ut::FindFirstFile( poolDirPath, _T("*.png") ) );
	ASSERT_EQUAL( _T("a.png"), ut::FindFirstFile( poolDirPath, _T("a*.png") ) );
	ASSERT_EQUAL( _T("a.png"), ut::FindFirstFile( poolDirPath, _T("*a*.png") ) );
	ASSERT_EQUAL( _T(""), ut::FindFirstFile( poolDirPath, _T("c.png") ) );
	ASSERT_EQUAL( _T("D1\\D2\\c.png"), ut::FindFirstFile( poolDirPath, _T("c.png"), fs::EF_Recurse ) );

	ASSERT_EQUAL( _T(""), ut::FindFirstFile( poolDirPath, _T("*.exe"), fs::EF_Recurse ) );
}

void CFileSystemTests::TestFileEnumFilter( void )
{
	ut::CTempFilePool pool( _T("a|a.txt|a.doc|a.jpg|a.png|D1\\b|D1\\b.doc|D1\\b.txt|D1\\D2\\c|D1/D2/c.doc|D1\\D2\\c.png|D1\\D2\\c.txt") );
	const fs::TDirPath& poolDirPath = pool.GetPoolDirPath();

	// ignore files
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse | fs::EF_IgnoreFiles );
		fs::SearchEnumFiles( &found, poolDirPath );
		ASSERT_EQUAL( _T(""), ut::JoinFiles( found ) );
		ASSERT_EQUAL( _T("D1|D1\\D2"), ut::JoinSubDirs( found ) );
	}

	// recursion depth level
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		found.RefOptions().m_maxDepthLevel = 0;
		fs::SearchEnumFiles( &found, poolDirPath );
		ASSERT_EQUAL( _T("a|a.doc|a.jpg|a.png|a.txt"), ut::JoinFiles( found ) );		// root files
	}
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		found.RefOptions().m_maxDepthLevel = 1;
		fs::SearchEnumFiles( &found, poolDirPath );
		ASSERT_EQUAL( _T("a|a.doc|a.jpg|a.png|a.txt|D1\\b|D1\\b.doc|D1\\b.txt"), ut::JoinFiles( found ) );		// root and D1/ files
	}
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		found.RefOptions().m_maxDepthLevel = 2;
		fs::SearchEnumFiles( &found, poolDirPath );
		ASSERT_EQUAL( _T("a|a.doc|a.jpg|a.png|a.txt|D1\\b|D1\\b.doc|D1\\b.txt|D1\\D2\\c|D1\\D2\\c.doc|D1\\D2\\c.png|D1\\D2\\c.txt"), ut::JoinFiles( found ) );	// all files
	}

	// maximum number of files
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		found.RefOptions().m_maxFiles = 6;
		fs::SearchEnumFiles( &found, poolDirPath );
		ASSERT_EQUAL( _T("a|a.doc|a.jpg|a.png|a.txt|D1\\b"), ut::JoinFiles( found ) );		// root files
	}

	// ignore path matches
	{
		std::vector<fs::CPath> ignorePaths;
		ignorePaths.push_back( poolDirPath / _T("D1\\D2") );
		ignorePaths.push_back( fs::CPath( _T("*.doc;*.jpg;*.png") ) );

		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		found.RefOptions().m_ignorePathMatches.Reset( ignorePaths );
		fs::SearchEnumFiles( &found, poolDirPath );
		ASSERT_EQUAL( _T("a|a.txt|D1\\b|D1\\b.txt"), ut::JoinFiles( found ) );		// root files
	}

	// filter by file size
	{
		{	// max size limit
			fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
			found.RefOptions().m_fileSizeRange.m_end = 5;
			fs::SearchEnumFiles( &found, poolDirPath );
			ASSERT_EQUAL( _T(""), ut::JoinFiles( found ) );

			ut::StoreFileTextSize( poolDirPath / _T("a.doc"), 3 );		// lower the size to pass the filter

			found.Clear();
			fs::SearchEnumFiles( &found, poolDirPath );
			ASSERT_EQUAL( _T("a.doc"), ut::JoinFiles( found ) );
		}
		{	// min size limit
			fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
			found.RefOptions().m_fileSizeRange.m_start = 100;
			fs::SearchEnumFiles( &found, poolDirPath );
			ASSERT_EQUAL( _T(""), ut::JoinFiles( found ) );

			// increase the sizes to pass the filter of min 100 bytes
			ut::StoreFileTextSize( poolDirPath / _T("a.txt"), 110 );
			ut::StoreFileTextSize( poolDirPath / _T("D1\\D2\\c.txt"), 128 );

			found.Clear();
			fs::SearchEnumFiles( &found, poolDirPath );
			ASSERT_EQUAL( _T("a.txt|D1\\D2\\c.txt"), ut::JoinFiles( found ) );
		}
	}
}

void CFileSystemTests::TestFileEnumHidden( void )
{
	ut::CTempFilePool pool( _T("a.doc|a.txt|D1\\b.txt|D1\\D2\\c.txt") );
	const fs::TDirPath& poolDirPath = pool.GetPoolDirPath();

	fs::AddFileAttributes( poolDirPath / _T("a.txt"), FILE_ATTRIBUTE_HIDDEN );		// hide file
	fs::AddFileAttributes( poolDirPath / _T("D1\\D2"), FILE_ATTRIBUTE_HIDDEN );		// hide subdirectory

	// include hidden nodes
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse );
		fs::SearchEnumFiles( &found, poolDirPath );
		ASSERT_EQUAL( _T("a.doc|a.txt|D1\\b.txt|D1\\D2\\c.txt"), ut::JoinFiles( found ) );
		ASSERT_EQUAL( _T("D1|D1\\D2"), ut::JoinSubDirs( found ) );
	}

	// ignore hidden nodes
	{
		fs::CRelativePathEnumerator found( poolDirPath, ut::s_recurse | fs::EF_IgnoreHiddenNodes );
		fs::SearchEnumFiles( &found, poolDirPath );
		ASSERT_EQUAL( _T("a.doc|D1\\b.txt"), ut::JoinFiles( found ) );
		ASSERT_EQUAL( _T("D1"), ut::JoinSubDirs( found ) );
	}
}

void CFileSystemTests::TestNumericFilename( void )
{
	const fs::TDirPath& poolDirPath = ut::CTempFilePool::MakePoolDirPath();

	const fs::CPath seedFilePath = ut::CTempFilePool::MakePoolDirPath() / _T("SeedFile.txt");
	if ( seedFilePath.FileExist() )
		fs::DeleteFile( seedFilePath.GetPtr() );

	ASSERT_EQUAL( _T("SeedFile.txt"), path::StripDirPrefix( fs::MakeUniqueNumFilename( seedFilePath ), poolDirPath ) );

	ut::CTempFilePool pool( _T("SeedFile.txt") );
	ASSERT_EQUAL( _T("SeedFile_[2].txt"), path::StripDirPrefix( fs::MakeUniqueNumFilename( seedFilePath ), poolDirPath ) );

	pool.CreateFiles( _T("SeedFile_ABC.txt") );
	ASSERT_EQUAL( _T("SeedFile_[3].txt"), path::StripDirPrefix( fs::MakeUniqueNumFilename( seedFilePath ), poolDirPath ) );

	pool.CreateFiles( _T("SeedFile_5.txt|SeedFile7.txt") );
	ASSERT_EQUAL( _T("SeedFile_[8].txt"), path::StripDirPrefix( fs::MakeUniqueNumFilename( seedFilePath ), poolDirPath ) );

	pool.CreateFiles( _T("SeedFile_abc30xyz.txt") );
	ASSERT_EQUAL( _T("SeedFile_[31].txt"), path::StripDirPrefix( fs::MakeUniqueNumFilename( seedFilePath ), poolDirPath ) );

	pool.CreateFiles( _T("SeedFile5870.txt|SeedFile133.txt") );
	ASSERT_EQUAL( _T("SeedFile_[5871].txt"), path::StripDirPrefix( fs::MakeUniqueNumFilename( seedFilePath ), poolDirPath ) );
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


void AFX_CDECL AfxTimeToFileTime( const CTime& time, LPFILETIME pFileTime );		// defined in <afximpl.h>

template<>
inline bool ut::Equals<FILETIME>( const FILETIME& x, const FILETIME& y )			// inject explicit specialization in ut namespace
{
	return x.dwLowDateTime == y.dwLowDateTime && x.dwHighDateTime == y.dwHighDateTime;
}

void CFileSystemTests::TestFileTimeConversions( void )
{	// ensure fs::MakeFileTime() is equivalent with AfxTimeToFileTime()
	FILETIME utlFileTime, mfcFileTime;

	// invalid (null) time conversion:
	{
		CTime badTime( -1 );

		ASSERT( nullptr == fs::MakeFileTime( utlFileTime, badTime ) );

		ASSERT_THROWS( CRuntimeException, fs::thr::MakeFileTime( utlFileTime, badTime, _T("SomeFile.ext"), fs::RuntimeExc ) );
		ASSERT_THROWS_MFC( CFileException, fs::thr::MakeFileTime( utlFileTime, badTime, _T("SomeFile.ext"), fs::MfcExc ) );

		ASSERT_THROWS_MFC( CFileException, AfxTimeToFileTime( badTime, &mfcFileTime ) );
	}

	// valid time conversion:
	{
		ASSERT_PTR( fs::MakeFileTime( utlFileTime, ut::s_creationTime ) );
		AfxTimeToFileTime( ut::s_creationTime, &mfcFileTime );

		ASSERT_EQUAL( utlFileTime, mfcFileTime );
	}
}

bool CFileSystemTests::CheckConsistentFileTime( void )
{	// check if the file system file-times WRITE is consistent with READ
	ut::CTempFilePool pool( _T("file.ext") );
	const fs::CPath& filePath = pool.GetFilePaths()[ 0 ];

	CFileStatus mfcStatus;
	if ( CFile::GetStatus( filePath.GetPtr(), mfcStatus ) )			// read
	{
		mfcStatus.m_ctime = ut::s_creationTime;
		CFile::SetStatus( filePath.GetPtr(), mfcStatus );			// write

		if ( CFile::GetStatus( filePath.GetPtr(), mfcStatus ) )		// read again
			if ( mfcStatus.m_ctime == ut::s_creationTime )
				return true;	// creation file time was recorded right away without errors
	}

	return false;				// not consistent; with recent MFC versions, it may vary due to machine-specifics (time-zone related policies, etc)
}

bool CFileSystemTests::TestFileAndDirectoryState( void )
{
	if ( !CheckConsistentFileTime() )
		return false;

	ut::CTempFilePool pool( _T("fa.txt|D1\\fb.txt") );

	{
		const fs::CPath& filePath = pool.GetFilePaths()[ 0 ];
		fs::CFileState fileState = fs::CFileState::ReadFromFile( filePath );
		ASSERT_EQUAL( CFile::archive, fileState.m_attributes );

		{	// check the WIN32_FILE_ATTRIBUTE_DATA construction via ::GetFileAttributesEx()
			fs::CFileState fileState2( filePath );
			ASSERT( fileState2.IsValid() );
			ASSERT_EQUAL( fileState, fileState2 );

			fs::CFileState fileState3;
			ASSERT( fileState3.Retrieve( filePath ) );
			ASSERT_EQUAL( fileState, fileState3 );

			fs::CPath naFilePath = filePath.Get() + _T("_NA");		// non-existing file
			ASSERT( !fileState2.Retrieve( naFilePath ) );
			ASSERT( !fileState2.IsValid() );
			ASSERT( fileState2 != fileState );
		}

		fs::CFileState newFileState;

		SetFlag( fileState.m_attributes, CFile::readOnly );
		fileState.WriteToFile();
		newFileState = fs::CFileState::ReadFromFile( filePath );

		ASSERT_EQUAL( CFile::readOnly | CFile::archive, newFileState.m_attributes );
		ASSERT_EQUAL( fileState, newFileState );

		fileState.m_creationTime = ut::s_creationTime;
		fileState.WriteToFile();
		newFileState = fs::CFileState::ReadFromFile( filePath );

		ASSERT_EQUAL( ut::s_creationTime, newFileState.m_creationTime );
		ASSERT_EQUAL( fileState, newFileState );

		fileState.m_modifTime = ut::s_modifTime;
		fileState.WriteToFile();
		newFileState = fs::CFileState::ReadFromFile( filePath );

		ASSERT_EQUAL( ut::s_modifTime, newFileState.m_modifTime );
		ASSERT_EQUAL( fileState, newFileState );

		fileState.m_accessTime = ut::s_accessTime;
		fileState.WriteToFile();
		newFileState = fs::CFileState::ReadFromFile( filePath );

		ASSERT_EQUAL( ut::s_accessTime, newFileState.m_accessTime );
		ASSERT_EQUAL( fileState, newFileState );
	}

	{
		fs::TDirPath dirPath = pool.GetFilePaths()[ 1 ].GetParentPath();
		fs::CFileState dirState = fs::CFileState::ReadFromFile( dirPath );
		ASSERT_EQUAL( CFile::directory, dirState.m_attributes );

		fs::CFileState newDirState;

		SetFlag( dirState.m_attributes, CFile::readOnly );
		dirState.WriteToFile();
		newDirState = fs::CFileState::ReadFromFile( dirPath );

		ASSERT_EQUAL( CFile::readOnly | CFile::directory, newDirState.m_attributes );
		ASSERT_EQUAL( dirState, newDirState );

		dirState.m_creationTime = ut::s_creationTime;
		dirState.WriteToFile();
		newDirState = fs::CFileState::ReadFromFile( dirPath );

		ASSERT_EQUAL( ut::s_creationTime, newDirState.m_creationTime );
		ASSERT_EQUAL( dirState, newDirState );

		dirState.m_modifTime = ut::s_modifTime;
		dirState.WriteToFile();
		newDirState = fs::CFileState::ReadFromFile( dirPath );

		ASSERT_EQUAL( ut::s_modifTime, newDirState.m_modifTime );
		ASSERT_EQUAL( dirState, newDirState );

		dirState.m_accessTime = ut::s_accessTime;
		dirState.WriteToFile();
		newDirState = fs::CFileState::ReadFromFile( dirPath );

		ASSERT_EQUAL( ut::s_accessTime, newDirState.m_accessTime );
		ASSERT_EQUAL( dirState, newDirState );
	}
	return true;
}

void CFileSystemTests::TestTouchFile( void )
{
	ut::CTempFilePool pool( _T("name.txt") );
	const fs::TDirPath filePath = pool.GetPoolDirPath() / _T("name.txt");

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
	const fs::CPath destPath = pool.GetPoolDirPath() / _T("fileX.txt");		// non-existent file

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

void CFileSystemTests::TestBackupFileFlat( void )
{
	ut::testBackupFileFlat( fs::FileSize );
	ut::testBackupFileFlat( fs::FileSizeAndCrc32 );
}

namespace ut
{
	void testBackupFileFlat( fs::FileContentMatch matchBy )
	{
		ut::CTempFilePool pool( _T("src.txt") );
		const fs::TDirPath& srcPath = pool.GetFilePaths()[ 0 ];
		fs::CPath bkFilePath;

		// 1) backup to the same directory
		{
			fs::CFileBackup backup( srcPath, fs::TDirPath(), matchBy );
			ASSERT( !backup.FindFirstDuplicateFile( &bkFilePath ) );

			{	// test would-be backup file path - to the same directory
				fs::AcquireResult result = fs::CreationError;

				ASSERT_EQUAL( _T("src-[2].txt"), backup.MakeBackupFilePath( &result ).GetFilename() );
				ASSERT( fs::Created == result );		// though no collision, we need a new copy since it's a backup operation (source file is about to be modified)
			}

			ASSERT_EQUAL( fs::Created, backup.CreateBackupFile( &bkFilePath ) );
			ASSERT_EQUAL( _T("src-[2].txt"), bkFilePath.GetFilename() );

			bkFilePath.Clear();
			ASSERT_EQUAL( fs::FoundExisting, backup.CreateBackupFile( &bkFilePath ) );
			ASSERT_EQUAL( _T("src-[2].txt"), bkFilePath.GetFilename() );

			ut::ModifyFileText( srcPath );			// change SRC content by appending filename
			fs::thr::TouchFile( srcPath );

			ASSERT_EQUAL( fs::Created, backup.CreateBackupFile( &bkFilePath ) );
			ASSERT_EQUAL( _T("src-[3].txt"), bkFilePath.GetFilename() );

			bkFilePath.Clear();
			ASSERT_EQUAL( fs::FoundExisting, backup.CreateBackupFile( &bkFilePath ) );
			ASSERT_EQUAL( _T("src-[3].txt"), bkFilePath.GetFilename() );
		}
	}
}

void CFileSystemTests::TestBackupFileSubDir( void )
{
	ut::testBackupFileSubDir( fs::FileSize );
	ut::testBackupFileSubDir( fs::FileSizeAndCrc32 );
}

namespace ut
{
	void testBackupFileSubDir( fs::FileContentMatch matchBy )
	{
		ut::CTempFilePool pool( _T("src.txt") );
		const fs::TDirPath& srcPath = pool.GetFilePaths()[ 0 ];
		fs::CPath bkFilePath;

		// 2) backup to a sub-directory
		{
			fs::TDirPath subDirPath( _T("SubDirA\\SubDirB") );
			fs::CFileBackup backupSubDir( srcPath, subDirPath, matchBy );
			ASSERT( !backupSubDir.FindFirstDuplicateFile( &bkFilePath ) );

			// test would-be backup file path
			fs::AcquireResult result = fs::CreationError;

			bkFilePath = backupSubDir.MakeBackupFilePath( &result );
			ASSERT_EQUAL( _T("SubDirA\\SubDirB\\src.txt"), path::StripDirPrefix( bkFilePath, pool.GetPoolDirPath() ) );
			ASSERT_EQUAL( fs::Created, result );		// new file should be cloned to the sub-dir

			// create backup #1
			ASSERT_EQUAL( fs::Created, backupSubDir.CreateBackupFile( &bkFilePath ) );
			ASSERT_EQUAL( _T("SubDirA\\SubDirB\\src.txt"), path::StripDirPrefix( bkFilePath, pool.GetPoolDirPath() ) );

			// test would-be backup file-path #1
			bkFilePath = backupSubDir.MakeBackupFilePath( &result );
			ASSERT_EQUAL( _T("SubDirA\\SubDirB\\src.txt"), path::StripDirPrefix( bkFilePath, pool.GetPoolDirPath() ) );
			ASSERT_EQUAL( fs::FoundExisting, result );	// backup is a distinct file with same content

			// test backup file matching with fs::FoundExisting #1
			ASSERT_EQUAL( fs::FoundExisting, backupSubDir.CreateBackupFile( &bkFilePath ) );
			ASSERT_EQUAL( _T("SubDirA\\SubDirB\\src.txt"), path::StripDirPrefix( bkFilePath, pool.GetPoolDirPath() ) );

			ut::ModifyFileText( srcPath );			// change SRC content by appending filename

			// create backup #2
			ASSERT_EQUAL( fs::Created, backupSubDir.CreateBackupFile( &bkFilePath ) );
			ASSERT_EQUAL( _T("SubDirA\\SubDirB\\src-[2].txt"), path::StripDirPrefix( bkFilePath, pool.GetPoolDirPath() ) );

			// test would-be backup file-path #2
			bkFilePath = backupSubDir.MakeBackupFilePath( &result );
			ASSERT_EQUAL( _T("SubDirA\\SubDirB\\src-[2].txt"), path::StripDirPrefix( bkFilePath, pool.GetPoolDirPath() ) );
			ASSERT_EQUAL( fs::FoundExisting, result );
		}
	}
}


void CFileSystemTests::Run( void )
{
	RUN_TEST( TestFileSystem );
	RUN_TEST( TestFileEnum );
	RUN_TEST( TestFileEnumFilter );
	RUN_TEST( TestFileEnumHidden );
	RUN_TEST( TestNumericFilename );
	RUN_TEST( TestTempFilePool );
	RUN_TEST( TestFileTimeConversions );
	RUN_CONDITIONAL_TEST( TestFileAndDirectoryState );
	RUN_TEST( TestTouchFile );
	RUN_TEST( TestFileTransferMatch );
	RUN_TEST( TestBackupFileFlat );
	RUN_TEST( TestBackupFileSubDir );
}


#endif //USE_UT
