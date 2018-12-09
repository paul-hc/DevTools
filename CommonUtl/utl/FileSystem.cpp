
#include "stdafx.h"
#include "FileSystem.h"
#include "EnumTags.h"
#include "FlexPath.h"
#include "RuntimeException.h"
#include <shlwapi.h>				// for PathRelativePathTo
#include <stdexcept>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	bool IsValidFile( const TCHAR* pFilePath )
	{
		DWORD attr = ::GetFileAttributes( pFilePath );
		return attr != INVALID_FILE_ATTRIBUTES && !HasFlag( attr, FILE_ATTRIBUTE_DIRECTORY );
	}

	bool IsValidDirectory( const TCHAR* pDirPath )
	{
		DWORD attr = ::GetFileAttributes( pDirPath );
		return attr != INVALID_FILE_ATTRIBUTES && HasFlag( attr, FILE_ATTRIBUTE_DIRECTORY );
	}

	bool IsReadOnlyFile( const TCHAR* pFilePath )
	{
		DWORD attr = ::GetFileAttributes( pFilePath );
		return attr != INVALID_FILE_ATTRIBUTES && HasFlag( attr, FILE_ATTRIBUTE_READONLY );
	}

	bool IsProtectedFile( const TCHAR* pFilePath )
	{
		DWORD attr = ::GetFileAttributes( pFilePath );
		return attr != INVALID_FILE_ATTRIBUTES && HasFlag( attr, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM );
	}


	fs::CPath MakeAbsoluteToCWD( const TCHAR* pRelativePath )
	{
		std::tstring relativePath = path::MakeNormal( pRelativePath );
		TCHAR absolutePath[ _MAX_PATH ];
		if ( NULL == ::_tfullpath( absolutePath, relativePath.c_str(), COUNT_OF( absolutePath ) ) )
			return std::tstring();			// failed

		return fs::CPath( absolutePath );
	}

	fs::CPath MakeRelativeTo( const TCHAR* pFromPath, const fs::CPath& dirPath )
	{
		std::tstring fromPath = path::MakeNormal( pFromPath );
		DWORD fromAttr = IsValidDirectory( fromPath.c_str() ) ? FILE_ATTRIBUTE_DIRECTORY : 0;
		TCHAR relativePath[ _MAX_PATH ];

		if ( !::PathRelativePathTo( relativePath, fromPath.c_str(), fromAttr, dirPath.GetPtr(), FILE_ATTRIBUTE_DIRECTORY ) )
			return fromPath;		// on error return input path as is (but normalized)

		return fs::CPath( relativePath );
	}

	std::tstring GetTempDirPath( void )
	{
		TCHAR dirPath[ MAX_PATH ];
		if ( 0 == ::GetTempPath( MAX_PATH, dirPath ) )
			return std::tstring();
		return dirPath;
	}

	std::tstring MakeTempDirPath( const TCHAR* pSubPath )
	{
		TCHAR dirPath[ MAX_PATH ];
		if ( 0 == ::GetTempPath( MAX_PATH, dirPath ) )
			return std::tstring();

		if ( !str::IsEmpty( pSubPath ) )
			return path::Combine( dirPath, pSubPath );
		return dirPath;
	}



	fs::AcquireResult MakeFileWritable( const TCHAR* pFilePath )
	{
		ASSERT( !str::IsEmpty( pFilePath ) );

		DWORD fileAttr = ::GetFileAttributes( pFilePath );
		if ( INVALID_FILE_ATTRIBUTES == fileAttr )
			return fs::CreationError;

		if ( !HasFlag( fileAttr, FILE_ATTRIBUTE_READONLY ) )
			return fs::FoundExisting;		// writeable file

		if ( !::SetFileAttributes( pFilePath, fileAttr & ~FILE_ATTRIBUTE_READONLY ) )
			return fs::CreationError;

		return fs::Created;
	}

	bool DeleteFile( const TCHAR* pFilePath )
	{
		return
			MakeFileWritable( pFilePath ) != fs::CreationError &&
			::DeleteFile( pFilePath ) != FALSE;
	}


	bool CreateDir( const TCHAR* pDirPath )
	{
		return
			fs::IsValidDirectory( pDirPath ) ||
			::CreateDirectory( pDirPath, NULL ) != FALSE;
	}

	bool CreateDirPath( const TCHAR* pDirPath )
	{
		std::tstring dirPath = path::MakeNormal( pDirPath );
		path::SetBackslash( dirPath, false );
		if ( !path::IsValid( dirPath ) )
			return false;

		// skip the root path and create successive subdirectories
		for ( size_t pos = path::GetRootPath( dirPath.c_str() ).length(); pos != dirPath.length(); )
		{
			size_t posNext = dirPath.find_first_of( path::DirDelims(), pos );
			std::tstring subDir = dirPath.substr( 0, posNext );
			if ( !fs::CreateDir( subDir.c_str() ) )
				return false;

			if ( std::tstring::npos == posNext )
				break;

			pos = ++posNext;
		}

		return fs::IsValidDirectory( dirPath.c_str() );
	}


	namespace thr
	{
		void MakeFileWritable( const TCHAR* pFilePath )
		{
			ASSERT( !str::IsEmpty( pFilePath ) );

			static const TCHAR s_opTag[] = _T("Make file writable");

			DWORD fileAttr = ::GetFileAttributes( pFilePath );
			if ( INVALID_FILE_ATTRIBUTES == fileAttr )
				ThrowFileOpLastError( ::GetLastError(), s_opTag, pFilePath, NULL );

			if ( HasFlag( fileAttr, FILE_ATTRIBUTE_READONLY ) )
				if ( !::SetFileAttributes( pFilePath, fileAttr & ~FILE_ATTRIBUTE_READONLY ) )
					ThrowFileOpLastError( ::GetLastError(), s_opTag, pFilePath, NULL );
		}

		void CopyFile( const TCHAR* pSrcFilePath, const TCHAR* pDestFilePath, bool failIfExists ) throws_( CRuntimeException )
		{
			ASSERT( !str::IsEmpty( pSrcFilePath ) && !str::IsEmpty( pDestFilePath ) );

			if ( !::CopyFile( pSrcFilePath, pDestFilePath, failIfExists ) )
				ThrowFileOpLastError( ::GetLastError(), _T("Copy file"), pSrcFilePath, pDestFilePath );
		}

		void MoveFile( const TCHAR* pSrcFilePath, const TCHAR* pDestFilePath,
					   DWORD flags /*= MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH*/ ) throws_( CRuntimeException )
		{
			ASSERT( !str::IsEmpty( pSrcFilePath ) && !str::IsEmpty( pDestFilePath ) );

			if ( !::MoveFileEx( pSrcFilePath, pDestFilePath, flags ) )
				ThrowFileOpLastError( ::GetLastError(), _T("Move file"), pSrcFilePath, pDestFilePath );
		}

		void DeleteFile( const TCHAR* pFilePath ) throws_( CRuntimeException )
		{
			MakeFileWritable( pFilePath );

			if ( !::DeleteFile( pFilePath ) )
				ThrowFileOpLastError( ::GetLastError(), _T("Delete file"), pFilePath, NULL );
		}


		void CreateDirPath( const TCHAR* pDirPath ) throws_( CRuntimeException )
		{
			if ( !fs::CreateDirPath( pDirPath ) )
				ThrowFileException( _T("Could not create directory: "), pDirPath );
		}

		void CreateDirPath_Mfc( const TCHAR* pDirPath ) throws_( CFileException )
		{
			if ( !fs::CreateDirPath( pDirPath ) )
				AfxThrowFileException( CFileException::badPath, -1, pDirPath );
		}

		void ThrowFileOpLastError( DWORD lastError, const TCHAR operationTag[], const TCHAR* pSrcFilePath, const TCHAR* pDestFilePath ) throws_( CRuntimeException )
		{
			std::tstring description = operationTag;
			std::tstring whatTag = _T("failed to copy to destination file: ");
			const TCHAR* pFilePath = pDestFilePath != NULL ? pDestFilePath : pSrcFilePath;

			switch ( lastError )
			{
				case ERROR_FILE_NOT_FOUND:
				case ERROR_PATH_NOT_FOUND:
					whatTag = _T("file not found");
					pFilePath = pSrcFilePath != NULL ? pSrcFilePath : pDestFilePath;
					break;
				case ERROR_ACCESS_DENIED:
					whatTag = _T("destination file already exists");
					break;
				case ERROR_ENCRYPTION_FAILED:
					whatTag = _T("failed to encrypt destination file ");
					break;
			}

			stream::Tag( description, whatTag, _T(" - ") );
			description += _T(": ");

			fs::ThrowFileException( description, pFilePath );
		}

		void ThrowFileOpLastError_Mfc( const TCHAR* pFilePath, DWORD lastError /*= ::GetLastError()*/ ) throws_( CFileException )
		{
			CFileException::ThrowOsError( lastError, pFilePath );
		}
	}

	bool DeleteDir( const TCHAR* pDirPath )
	{
		DWORD dirAttr = ::GetFileAttributes( pDirPath );
		if ( INVALID_FILE_ATTRIBUTES == dirAttr )
			return false;

		if ( HasFlag( dirAttr, FILE_ATTRIBUTE_READONLY ) )								// read-only file?
			::SetFileAttributes( pDirPath, dirAttr & ~FILE_ATTRIBUTE_READONLY );		// make it writeable so that it can be deleted

		return
			DeleteAllFiles( pDirPath ) &&
			::RemoveDirectory( pDirPath ) != FALSE;
	}

	bool DeleteAllFiles( const TCHAR* pDirPath )
	{
		if ( !fs::IsValidDirectory( pDirPath ) )
			return false;

		fs::CEnumerator found;
		fs::EnumFiles( &found, pDirPath, _T("*") );

		for ( std::vector< std::tstring >::iterator itSubDirPath = found.m_subDirPaths.begin(); itSubDirPath != found.m_subDirPaths.end(); )
			if ( fs::DeleteDir( itSubDirPath->c_str() ) )
				itSubDirPath = found.m_subDirPaths.erase( itSubDirPath );
			else
				++itSubDirPath;

		for ( std::vector< std::tstring >::iterator itFilePath = found.m_filePaths.begin(); itFilePath != found.m_filePaths.end(); )
			if ( fs::DeleteFile( itFilePath->c_str() ) )
				itFilePath = found.m_filePaths.erase( itFilePath );
			else
				++itFilePath;

		return found.m_subDirPaths.empty() && found.m_filePaths.empty();			// deleted all existing
	}


	UINT64 BufferedCopy( CFile& rDestFile, CFile& srcFile, size_t chunkSize /*= 4 * KiloByte*/ )
	{
		UINT64 fileSize = srcFile.GetLength();
		std::vector< BYTE > buffer;
		buffer.resize( (std::min)( chunkSize, static_cast< size_t >( fileSize ) ) );		// grow the size of the copy buffer as needed

		for ( UINT64 bytesLeft = fileSize; bytesLeft != 0; )
		{
			UINT bytesRead = srcFile.Read( &buffer.front(), static_cast< UINT >( buffer.size() ) );
			rDestFile.Write( &buffer.front(), bytesRead );
			bytesLeft -= bytesRead;

			ENSURE( bytesLeft < fileSize );			// no underflow
		}
		return fileSize;
	}

} //namespace fs


namespace fs
{
	struct FriendlyFileFind : CFileFind { using CFileFind::m_pFoundInfo; };

	const WIN32_FIND_DATA* GetFindData( const CFileFind& foundFile )
	{
		const FriendlyFileFind* pFriendlyFileFinder = reinterpret_cast< const FriendlyFileFind* >( &foundFile );
		ASSERT_PTR( pFriendlyFileFinder->m_pFoundInfo );				// must have already found have a file (current file)
		return reinterpret_cast< const WIN32_FIND_DATA* >( pFriendlyFileFinder->m_pFoundInfo );
	}


	UINT64 GetFileSize( const TCHAR* pFilePath )
	{
		_stat64 fileStatus;
		if ( 0 == _tstat64( pFilePath, &fileStatus ) )
			return static_cast< UINT64 >( fileStatus.st_size );

		return ULLONG_MAX;			// error, could use errno to find out more
	}

	CTime ReadFileTime( const fs::CPath& filePath, TimeField timeField )
	{
		_stat64i32 fileStatus;
		if ( 0 == _tstat( filePath.GetPtr(), &fileStatus ) )
			switch ( timeField )
			{
				case CreatedDate:	return fileStatus.st_ctime;
				case ModifiedDate:	return fileStatus.st_mtime;
				case AccessedDate:	return fileStatus.st_atime;
			}

		return 0;
	}


	const CEnumTags& GetTags_TimeField( void )
	{
		static const CEnumTags s_tags( _T("Created Date|Modified Date|Accessed Date"), _T("C|M|A") );
		return s_tags;
	}

	const CEnumTags& GetTags_FileExpireStatus( void )
	{
		static const CEnumTags s_tags( _T("|source file modified|source file deleted") );
		return s_tags;
	}

	FileExpireStatus CheckExpireStatus( const fs::CPath& filePath, const CTime& lastModifyTime )
	{
		CTime currModifTime = ReadLastModifyTime( filePath );
		if ( 0 == currModifTime.GetTime() )				// probably image file deleted -> expired
			return ExpiredFileDeleted;

		return currModifTime > lastModifyTime ? ExpiredFileModified : FileNotExpired;
	}


	namespace thr
	{
		void WriteFileTime( const TCHAR* pFilePath, TimeField timeField, const CTime& time ) throws_( CRuntimeException )
		{
			try
			{
				WriteFileTime_Mfc( pFilePath, timeField, time );
			}
			catch ( CException* pExc )
			{
				CRuntimeException::ThrowFromMfc( pExc );
			}
		}

		void WriteFileTime_Mfc( const TCHAR* pFilePath, TimeField timeField, const CTime& time ) throws_( CFileException )
		{
			ASSERT( !str::IsEmpty( pFilePath ) );
			ASSERT( time.GetTime() != 0 );

			FILETIME fileTime;
			std::vector< const FILETIME* > triplet( _TimeFieldCount );		// all reset to NULL

			triplet[ timeField ] = MakeFileTime_Mfc( fileTime, time );
			if ( triplet[ timeField ] != NULL )
			{
				fs::CScopedWriteableFile scopedWriteable( pFilePath );

				fs::CHandle file( ::CreateFile( pFilePath, GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ, NULL, OPEN_EXISTING,
					IsValidDirectory( pFilePath ) ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL,		// IMPORTANT: for access to directory vs file
					NULL ) );

				if ( !file.IsValid() ||
					 !::SetFileTime( file.Get(), triplet[ CreatedDate ], triplet[ AccessedDate ], triplet[ ModifiedDate ] ) ||
					 !file.Close() )
					ThrowFileOpLastError_Mfc( pFilePath );
			}
		}


		FILETIME* MakeFileTime( FILETIME& rOutFileTime, const CTime& time ) throws_( CRuntimeException )
		{
			try
			{
				return MakeFileTime_Mfc( rOutFileTime, time );
			}
			catch ( CException* pExc )
			{
				CRuntimeException::ThrowFromMfc( pExc );
				return NULL;
			}
		}

		FILETIME* MakeFileTime_Mfc( FILETIME& rOutFileTime, const CTime& time ) throws_( CFileException )
		{
			if ( 0 == time.GetTime() )
				return NULL;

			SYSTEMTIME sysTime;
			sysTime.wYear = (WORD)time.GetYear();
			sysTime.wMonth = (WORD)time.GetMonth();
			sysTime.wDay = (WORD)time.GetDay();
			sysTime.wHour = (WORD)time.GetHour();
			sysTime.wMinute = (WORD)time.GetMinute();
			sysTime.wSecond = (WORD)time.GetSecond();
			sysTime.wMilliseconds = 0;

			// convert system time to local file time
			FILETIME localTime;
			if ( !::SystemTimeToFileTime( &sysTime, &localTime ) )
				CFileException::ThrowOsError( (LONG)::GetLastError() );

			// convert local file time to UTC file time
			if ( !::LocalFileTimeToFileTime( &localTime, &rOutFileTime ) )
				CFileException::ThrowOsError( (LONG)::GetLastError() );

			return &rOutFileTime;
		}
	}
}


namespace fs
{
	CScopedWriteableFile::CScopedWriteableFile( const TCHAR* pFilePath )
		: m_pFilePath( pFilePath )
		, m_origAttr( ::GetFileAttributes( m_pFilePath ) )
	{
		if ( m_origAttr != INVALID_FILE_ATTRIBUTES )
			if ( HasFlag( m_origAttr, FILE_ATTRIBUTE_READONLY ) )										// currently readonly?
				if ( !::SetFileAttributes( m_pFilePath, m_origAttr & ~FILE_ATTRIBUTE_READONLY ) )		// temporarily allow changing file times for a read-only file/directory
					m_origAttr = INVALID_FILE_ATTRIBUTES;												// failed removing FILE_ATTRIBUTE_READONLY
	}

	CScopedWriteableFile::~CScopedWriteableFile()
	{
		if ( m_origAttr != INVALID_FILE_ATTRIBUTES )
			if ( HasFlag( m_origAttr, FILE_ATTRIBUTE_READONLY ) )			// was readonly?
				::SetFileAttributes( m_pFilePath, m_origAttr );				// restore original FILE_ATTRIBUTE_READONLY
	}
}


namespace fs
{
	class CFileRefEnumerator : public IEnumerator
	{
	public:
		CFileRefEnumerator( std::vector< std::tstring >* pFilePaths ) : m_pFilePaths( pFilePaths ) { ASSERT_PTR( pFilePaths ); }

		// IEnumerator interface
		virtual void AddFoundFile( const TCHAR* pFilePath ) { m_pFilePaths->push_back( pFilePath ); }
	private:
		std::vector< std::tstring >* m_pFilePaths;
	};


	class CSubDirRefEnumerator : public IEnumerator
	{
	public:
		CSubDirRefEnumerator( std::vector< std::tstring >* pSubDirPaths ) : m_pSubDirPaths( pSubDirPaths ) { ASSERT_PTR( pSubDirPaths ); }

		// IEnumerator interface
		virtual void AddFoundFile( const TCHAR* pFilePath ) { pFilePath; }
		virtual void AddFoundSubDir( const TCHAR* pSubDirPath ) { m_pSubDirPaths->push_back( pSubDirPath ); }
	private:
		std::vector< std::tstring >* m_pSubDirPaths;
	};


	struct CSingleFileEnumerator : public CPathEnumerator
	{
		CSingleFileEnumerator( void ) {}

		// base overrides
		virtual bool MustStop( void ) { return !m_filePaths.empty(); }

		fs::CPath GetFoundPath( void ) const { return !m_filePaths.empty() ? *m_filePaths.begin() : fs::CPath(); }
	};


	// CEnumerator implementation

	void CEnumerator::AddFoundFile( const TCHAR* pFilePath )
	{
		std::tstring filePath = path::StripCommonPrefix( pFilePath, m_refDirPath.c_str() );
		if ( !filePath.empty() )
			m_filePaths.push_back( filePath );

		if ( m_pChainEnum != NULL )
			m_pChainEnum->AddFoundFile( pFilePath );
	}

	void CEnumerator::AddFoundSubDir( const TCHAR* pSubDirPath )
	{
		std::tstring subDirPath = path::StripCommonPrefix( pSubDirPath, m_refDirPath.c_str() );
		if ( !subDirPath.empty() )
			m_subDirPaths.push_back( subDirPath );

		if ( m_pChainEnum != NULL )
			m_pChainEnum->AddFoundSubDir( pSubDirPath );
	}


	// CPathEnumerator implementation

	void CPathEnumerator::AddFoundFile( const TCHAR* pFilePath )
	{
		std::tstring filePath = path::StripCommonPrefix( pFilePath, m_refDirPath.c_str() );
		if ( !filePath.empty() )
			m_filePaths.insert( filePath );

		if ( m_pChainEnum != NULL )
			m_pChainEnum->AddFoundFile( pFilePath );
	}

	void CPathEnumerator::AddFoundSubDir( const TCHAR* pSubDirPath )
	{
		std::tstring subDirPath = path::StripCommonPrefix( pSubDirPath, m_refDirPath.c_str() );
		if ( !subDirPath.empty() )
			m_subDirPaths.insert( subDirPath );

		if ( m_pChainEnum != NULL )
			m_pChainEnum->AddFoundSubDir( pSubDirPath );
	}


	void EnumFiles( IEnumerator* pEnumerator, const TCHAR* pDirPath, const TCHAR* pWildSpec /*= _T("*.*")*/, RecursionDepth depth /*= Shallow*/ )
	{
		ASSERT_PTR( pEnumerator );
		ASSERT_PTR( pDirPath );

		fs::CPath dirPathFilter = fs::CPath( pDirPath ) /
			fs::CPath( Deep == depth || str::IsEmpty( pWildSpec )
				? _T("*")			// need to relax filter to all so that it covers sub-directories
				: pWildSpec );

		std::vector< fs::CPath > subDirPaths;

		CFileFind finder;
		for ( BOOL found = finder.FindFile( dirPathFilter.GetPtr() ); found && !pEnumerator->MustStop(); )
		{
			found = finder.FindNextFile();
			std::tstring foundPath = finder.GetFilePath().GetString();

			if ( finder.IsDirectory() )
			{
				if ( !finder.IsDots() )						// skip "." and ".." dir entries
				{
					pEnumerator->AddFoundSubDir( foundPath.c_str() );
					if ( Deep == depth )
						subDirPaths.push_back( fs::CPath( foundPath ) );
				}
			}
			else
			{
				if ( Shallow == depth || path::MatchWildcard( foundPath.c_str(), pWildSpec ) )
					pEnumerator->AddFile( finder );
			}
		}

		if ( Deep == depth && !pEnumerator->MustStop() )
		{
			fs::SortPaths( subDirPaths );		// sort intuitively

			for ( std::vector< fs::CPath >::const_iterator itSubDirPath = subDirPaths.begin(); itSubDirPath != subDirPaths.end(); ++itSubDirPath )
				EnumFiles( pEnumerator, itSubDirPath->GetPtr(), pWildSpec, depth );
		}
	}

	void EnumFiles( std::vector< std::tstring >& rFilePaths, const TCHAR* pDirPath, const TCHAR* pWildSpec /*= _T("*")*/, RecursionDepth depth /*= Shallow*/ )
	{
		CFileRefEnumerator found( &rFilePaths );
		EnumFiles( &found, pDirPath, pWildSpec, depth );
	}

	void EnumSubDirs( std::vector< std::tstring >& rSubDirPaths, const TCHAR* pDirPath, const TCHAR* pWildSpec /*= _T("*.*")*/, RecursionDepth depth /*= Shallow*/ )
	{
		CSubDirRefEnumerator found( &rSubDirPaths );
		EnumFiles( &found, pDirPath, pWildSpec, depth );
	}


	fs::CPath FindFirstFile( const TCHAR* pDirPath, const TCHAR* pWildSpec /*= _T("*.*")*/, RecursionDepth depth /*= Shallow*/ )
	{
		CSingleFileEnumerator singleEnumer;
		EnumFiles( &singleEnumer, pDirPath, pWildSpec, depth );
		return singleEnumer.GetFoundPath();
	}

} //namespace fs


namespace fs
{
	std::tstring MakeUniqueNumFilename( const TCHAR* pFilePath, const TCHAR fmtNumSuffix[] /*= _T("_[%d]")*/ ) throws_( CRuntimeException )
	{
		ASSERT( !str::IsEmpty( pFilePath ) );
		if ( !FileExist( pFilePath ) )
			return pFilePath;					// no filename collision

		fs::CPathParts parts( pFilePath );
		const std::tstring fnameBase = parts.m_fname;
		std::tstring lastSuffix;
		std::tstring uniqueFilePath;

		for ( UINT seqCount = 2; ; ++seqCount )
		{
			std::tstring suffix = str::Format( fmtNumSuffix, seqCount );
			if ( lastSuffix == suffix )
				throw CRuntimeException( str::Format( _T("Fishy numeric suffix format '%s'. It does not produce unique filenames."), fmtNumSuffix ) );
			else
				lastSuffix = suffix;

			parts.m_fname = fnameBase + suffix;		// e.g. "fnameBase_[N]"
			uniqueFilePath = parts.MakePath();

			if ( !FileExist( uniqueFilePath.c_str() ) )					// unique, done
				break;
		}
		return uniqueFilePath;
	}

	std::tstring MakeUniqueHashedFilename( const TCHAR* pFilePath, const TCHAR fmtHashSuffix[] /*= _T("_%08X")*/ )
	{
		std::tstring uniqueFilePath = pFilePath;
		if ( FileExist( pFilePath ) )
		{
			fs::CPathParts parts( uniqueFilePath );

			const UINT hashKey = static_cast< UINT >( path::GetHashValue( pFilePath ) );		// hash key is unique for the whole path
			parts.m_fname += str::Format( fmtHashSuffix, hashKey );		// e.g. "fname_hexHashKey"
			uniqueFilePath = parts.MakePath();

			ENSURE( !FileExist( uniqueFilePath.c_str() ) );				// unique
		}
		return uniqueFilePath;
	}
}
