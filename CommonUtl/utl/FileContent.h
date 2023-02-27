#ifndef FileContent_h
#define FileContent_h
#pragma once

#include "FileSystem_fwd.h"


namespace fs
{
	// for file transfer (copy/move)
	enum TransferMatch { SrcUpToDate, NoSrcFile, NoDestFile, SrcFileNewer, SizeMismatch, Crc32Mismatch };

	TransferMatch EvalTransferMatch( const fs::CPath& srcFilePath, const fs::CPath& destFilePath, bool checkTimestamp = true, FileContentMatch matchContentBy = FileSize );
	bool IsSrcFileMismatch( const fs::CPath& srcFilePath, const fs::CPath& destFilePath, bool checkTimestamp = true, FileContentMatch matchContentBy = FileSize );


	struct CFileState;


	struct CFileContentKey
	{
		CFileContentKey( void ) : m_fileSize( 0ull ), m_crc32( 0u ) {}
		CFileContentKey( UINT64 fileSize, UINT crc32 ) : m_fileSize( fileSize ), m_crc32( crc32 ) { ASSERT( m_fileSize != 0ull && m_crc32 != 0u ); }
		CFileContentKey( const fs::CFileState& fileState );		// lazy CRC32 evaluation

		bool IsEmpty( void ) const { return 0 == m_fileSize && 0 == m_crc32; }
		bool HasCrc32( void ) const { return m_crc32 != 0 || 0 == m_fileSize; }
		UINT StoreCrc32( const fs::CFileState& fileState, bool cacheCompute = true );

		bool ComputeFileSize( const fs::CPath& filePath );
		bool ComputeCrc32( const fs::CPath& filePath );

		std::tstring Format( void ) const;

		bool operator==( const CFileContentKey& right ) const { return m_fileSize == right.m_fileSize && m_crc32 == right.m_crc32; }
		bool operator!=( const CFileContentKey& right ) const { return !operator==( right ); }
		bool operator<( const CFileContentKey& right ) const;
	public:
		UINT64 m_fileSize;		// in bytes
		UINT m_crc32;			// CRC32 checksum
	};


	class CFileLazyContentKey
	{
	public:
		CFileLazyContentKey( const fs::CPath& filePath ) : m_filePath( filePath ) { ASSERT( fs::IsValidFile( m_filePath.GetPtr() ) ); }

		// lazy evaluation
		UINT64 GetFileSize( void ) const;
		UINT GetCrc32( void ) const;
		bool IsContentMatch( const CFileLazyContentKey& right, FileContentMatch matchBy ) const;

		// as is (no lazy evaluation)
		const fs::CPath& GetFilePath( void ) const { return m_filePath; }
		const fs::CFileContentKey& GetKey( void ) const { return m_key; }
	private:
		const fs::CPath m_filePath;
		mutable fs::CFileContentKey m_key;
	};


	class CFileCompareContent
	{
	public:
		CFileCompareContent( const fs::CPath& srcFilePath, FileContentMatch matchBy = FileSize )
			: m_srcFileKey( srcFilePath ), m_matchBy( matchBy ) {}

		bool IsContentMatch( const fs::CPath& destFilePath ) const { return m_srcFileKey.IsContentMatch( destFilePath, m_matchBy ); }
	private:
		CFileLazyContentKey m_srcFileKey;
		FileContentMatch m_matchBy;
	};


	class CFileBackup
	{
	public:
		CFileBackup( const fs::CPath& srcFilePath, fs::TDirPath backupDirPath = fs::TDirPath(), FileContentMatch matchBy = FileSize, const TCHAR fmtNumSuffix[] = _T("-[%d]") );

		bool FindFirstDuplicateFile( fs::CPath* pOutDupFilePath ) const;
		fs::CPath MakeBackupFilePath( fs::AcquireResult* pOutResult = nullptr ) const;							// would-be backup file path (no copying involved)

		fs::AcquireResult CreateBackupFile( fs::CPath* pOutBackupFilePath ) throws_( CRuntimeException );	// create a backup copy of the original if none existing files matching "name*.ext" have identical content

		const fs::CPath& GetSrcFilePath( void ) const { return m_srcFilePath; }
		FileContentMatch GetMatchBy( void ) const { return m_matchBy; }
	private:
		fs::CPath m_srcFilePath;			// converted to absolute
		fs::TDirPath m_backupDirPath;
		FileContentMatch m_matchBy;
		const TCHAR* m_pFmtNumSuffix;
	};
}


#include "StdHashValue.h"


template<>
struct std::hash<fs::CFileContentKey>
{
	inline std::size_t operator()( const fs::CFileContentKey& key ) const /*noexcept*/
    {
        return utl::GetHashCombine( key.m_fileSize, key.m_crc32 );
    }
};


#endif // FileContent_h
