
#include "stdafx.h"
#include "FileContent.h"
#include "FileState.h"
#include "FileEnumerator.h"
#include "FileSystem.h"
#include "Crc32.h"
#include "ContainerUtilities.h"
#include "TimeUtils.h"
#include "StringUtilities.h"
#include "StdHashValue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace stdext
{
	size_t hash_value( const fs::CFileContentKey& key )
	{
		size_t value = stdext::hash_value( key.m_fileSize );
		utl::hash_combine( value, key.m_crc32 );
		return value;
	}
}


namespace fs
{
	TransferMatch EvalTransferMatch( const fs::CPath& srcFilePath, const fs::CPath& destFilePath, bool checkTimestamp /*= true*/, FileContentMatch matchContentBy /*= FileSize*/ )
	{
		if ( !IsValidFile( srcFilePath.GetPtr() ) )
			return NoSrcFile;

		if ( !IsValidFile( destFilePath.GetPtr() ) )
			return NoDestFile;

		if ( checkTimestamp )
		{
			CTime destModifTime = ReadLastModifyTime( destFilePath );
			if ( !time_utl::IsValid( destModifTime ) )
				return NoDestFile;

			switch ( CheckExpireStatus( srcFilePath, destModifTime ) )
			{
				case FileNotExpired:		break;
				case ExpiredFileDeleted:	return NoSrcFile;
				case ExpiredFileModified:	return SrcFileNewer;
			}
		}

		CFileLazyContentKey srcKey( srcFilePath ), destKey( destFilePath );
		if ( !srcKey.IsContentMatch( destKey, matchContentBy ) )
			if ( srcKey.GetFileSize() != destKey.GetFileSize() )
				return SizeMismatch;
			else
				return Crc32Mismatch;

		return SrcUpToDate;		// no need to copy SRC over dest
	}

	bool IsSrcFileMismatch( const fs::CPath& srcFilePath, const fs::CPath& destFilePath, bool checkTimestamp /*= true*/, FileContentMatch matchContentBy /*= FileSize*/ )
	{
		return SrcUpToDate == EvalTransferMatch( srcFilePath, destFilePath, checkTimestamp, matchContentBy );
	}


	// CFileContentKey implementation

	CFileContentKey::CFileContentKey( const fs::CFileState& fileState )
		: m_fileSize( fileState.m_fileSize )
		, m_crc32( fileState.GetCrc32( fs::CFileState::AsIs ) )
	{
	}

	UINT CFileContentKey::StoreCrc32( const fs::CFileState& fileState, bool cacheCompute /*= true*/ )
	{
		return m_crc32 = fileState.GetCrc32( cacheCompute ? fs::CFileState::CacheCompute : fs::CFileState::Compute );
	}

	bool CFileContentKey::ComputeFileSize( const fs::CPath& filePath )
	{
		m_fileSize = fs::GetFileSize( filePath.GetPtr() );
		return m_fileSize != ULLONG_MAX;
	}

	bool CFileContentKey::ComputeCrc32( const fs::CPath& filePath )
	{
		m_crc32 = fs::CCrc32FileCache::Instance().AcquireCrc32( filePath );
		return m_crc32 != 0 || 0 == m_fileSize;		// true for valid checksum or empty file
	}

	std::tstring CFileContentKey::Format( void ) const
	{
		std::tstring text = str::Format( _T("file size=%s"), num::FormatFileSize( m_fileSize, num::Bytes, true ).c_str() );

		if ( m_crc32 != 0 )
			text += str::Format( _T(", CRC32=%X"), m_crc32 );

		return text;
	}

	bool CFileContentKey::operator<( const CFileContentKey& right ) const
	{
		if ( m_fileSize < right.m_fileSize )
			return true;
		else if ( m_fileSize == right.m_fileSize )
			return m_crc32 < right.m_crc32;

		return false;
	}


	// CFileLazyContentKey implementation

	UINT64 CFileLazyContentKey::GetFileSize( void ) const
	{
		if ( 0 == m_key.m_fileSize )
			m_key.ComputeFileSize( m_filePath );

		return m_key.m_fileSize;
	}

	UINT CFileLazyContentKey::GetCrc32( void ) const
	{
		if ( !m_key.HasCrc32() )
			m_key.ComputeCrc32( m_filePath );

		return m_key.m_crc32;
	}

	bool CFileLazyContentKey::IsContentMatch( const CFileLazyContentKey& right, FileContentMatch matchBy ) const
	{
		if ( GetFilePath() == right.GetFilePath() )
		{
			ASSERT( false );		// doesn't make much sense to inquire for the same file
			return true;
		}

		if ( GetFileSize() != right.GetFileSize() )		// quick size mismatch?
			return false;

		if ( FileSizeAndCrc32 == matchBy )
			if ( GetCrc32() != right.GetCrc32() )		// slower but accurate - CRC32 evaluation goes through the entire file contents
				return false;

		return true;
	}


	// CDupFileEnumerator class

	class CDupFileEnumerator : public fs::IEnumeratorImpl
	{
	public:
		CDupFileEnumerator( const CFileBackup& backup );

		// base overrides
		virtual void AddFoundFile( const TCHAR* pFilePath );
		virtual bool AddFoundSubDir( const TCHAR* pSubDirPath ) { pSubDirPath; return true; }
		virtual bool MustStop( void ) const { return !m_dupFilePaths.empty(); }
	private:
		const CFileBackup& m_backup;
		CFileCompareContent m_srcContent;
	public:
		std::vector< fs::CPath > m_dupFilePaths;
	};


	// CFileBackup class

	CFileBackup::CFileBackup( const fs::CPath& srcFilePath, fs::TDirPath backupDirPath /*= fs::CPath()*/, FileContentMatch matchBy /*= FileSize*/, const TCHAR fmtNumSuffix[] /*= _T("-[%d]")*/ )
		: m_srcFilePath( srcFilePath )
		, m_backupDirPath( backupDirPath )
		, m_matchBy( matchBy )
		, m_pFmtNumSuffix( fmtNumSuffix )
	{
		ASSERT( fs::IsValidFile( m_srcFilePath.GetPtr() ) );
		CvtAbsoluteToCWD( m_srcFilePath );

		if ( path::IsRelative( m_backupDirPath.GetPtr() ) )
		{
			m_backupDirPath = m_srcFilePath.GetParentPath() / m_backupDirPath;
			CvtAbsoluteToCWD( m_backupDirPath );
		}
	}

	bool CFileBackup::FindFirstDuplicateFile( fs::CPath& rDupFilePath ) const
	{
		if ( !fs::IsValidDirectory( m_backupDirPath.GetPtr() ) )
			return false;		// not yet created

		const fs::CPathParts srcParts( m_srcFilePath.Get() );
		std::tstring wildSpec = srcParts.m_fname + _T("*") + srcParts.m_ext;	// "name*.ext"

		CDupFileEnumerator enumerator( *this );
		EnumFiles( &enumerator, m_backupDirPath, wildSpec.c_str() );

		if ( enumerator.m_dupFilePaths.empty() )
			return false;

		rDupFilePath = enumerator.m_dupFilePaths.front();			// first hit
		return true;
	}

	fs::AcquireResult CFileBackup::CreateBackupFile( fs::CPath& rBackupFilePath ) throws_( CRuntimeException )
	{
		if ( FindFirstDuplicateFile( rBackupFilePath ) )
			return fs::FoundExisting;			// found existing backed-up file matching "name*.ext" with matching content

		fs::thr::CreateDirPath( m_backupDirPath.GetPtr() );

		fs::CPath backupFilePath = m_backupDirPath / m_srcFilePath.GetFilename();

		backupFilePath = fs::MakeUniqueNumFilename( backupFilePath, m_pFmtNumSuffix );		// make unique backup file name by avoiding collisions with existing file backups
		fs::thr::CopyFile( m_srcFilePath.GetPtr(), backupFilePath.GetPtr(), FALSE );

		rBackupFilePath = backupFilePath;
		return fs::Created;
	}


	// CDupFileEnumerator implementation

	CDupFileEnumerator::CDupFileEnumerator( const CFileBackup& backup )
		: m_backup( backup )
		, m_srcContent( m_backup.m_srcFilePath, FileSize )
	{
	}

	void CDupFileEnumerator::AddFoundFile( const TCHAR* pFilePath )
	{
		fs::CPath filePath( pFilePath );

		if ( filePath != m_backup.m_srcFilePath )		// different than source file?
			if ( m_srcContent.IsContentMatch( filePath ) )
				m_dupFilePaths.push_back( filePath );
	}
}
