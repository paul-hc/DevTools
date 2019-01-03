
#include "stdafx.h"
#include "FileSystem.h"
#include "EnumTags.h"
#include "FlexPath.h"
#include "RuntimeException.h"
#include "StringUtilities.h"
#include "TimeUtils.h"
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
		fs::EnumFiles( &found, fs::CPath( pDirPath ), _T("*"), Shallow );

		for ( std::vector< fs::CPath >::iterator itSubDirPath = found.m_subDirPaths.begin(); itSubDirPath != found.m_subDirPaths.end(); )
			if ( fs::DeleteDir( itSubDirPath->GetPtr() ) )
				itSubDirPath = found.m_subDirPaths.erase( itSubDirPath );
			else
				++itSubDirPath;

		for ( std::vector< fs::CPath >::iterator itFilePath = found.m_filePaths.begin(); itFilePath != found.m_filePaths.end(); )
			if ( fs::DeleteFile( itFilePath->GetPtr() ) )
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


	namespace thr
	{
		void MakeFileWritable( const TCHAR* pFilePath ) throws_( CRuntimeException )
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


		void CreateDirPath( const TCHAR* pDirPath,
							fs::ExcPolicy policy /*= fs::RuntimeExc*/ ) throws_( CRuntimeException, CFileException* )
		{
			if ( !fs::CreateDirPath( pDirPath ) )
				ThrowFileException( _T("Could not create directory"), pDirPath, policy );
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

			impl::ThrowFileException( description, pFilePath );
		}

	} //namespace thr

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
		if ( !time_utl::IsValid( currModifTime ) )				// probably image file deleted -> expired
			return ExpiredFileDeleted;

		return currModifTime > lastModifyTime ? ExpiredFileModified : FileNotExpired;
	}


	namespace thr
	{
		CTime ReadFileTime( const fs::CPath& filePath, TimeField timeField,
							fs::ExcPolicy policy /*= fs::RuntimeExc*/ ) throws_( CRuntimeException, CFileException* )
		{
			_stat64i32 fileStatus;
			int result = _tstat( filePath.GetPtr(), &fileStatus );
			if ( 0 == result )
				switch ( timeField )
				{
					case CreatedDate:	return fileStatus.st_ctime;
					case ModifiedDate:	return fileStatus.st_mtime;
					case AccessedDate:	return fileStatus.st_atime;
				}

			impl::ThrowMfcErrorAs( policy, impl::NewErrnoException( filePath.GetPtr(), result ) );
		}

		void WriteFileTime( const TCHAR* pFilePath, TimeField timeField, const CTime& time,
							fs::ExcPolicy policy /*= fs::RuntimeExc*/ ) throws_( CRuntimeException, CFileException* )
		{
			ASSERT( !str::IsEmpty( pFilePath ) );
			ASSERT( time_utl::IsValid( time ) );

			FILETIME fileTime;
			std::vector< const FILETIME* > triplet( _TimeFieldCount );		// all reset to NULL

			triplet[ timeField ] = MakeFileTime( fileTime, time, pFilePath, policy );
			if ( triplet[ timeField ] != NULL )
			{
				fs::CScopedWriteableFile scopedWriteable( pFilePath );

				fs::CHandle file( ::CreateFile( pFilePath, GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ, NULL, OPEN_EXISTING,
					IsValidDirectory( pFilePath ) ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL,		// IMPORTANT: for access to directory vs file
					NULL ) );

				if ( file.IsValid() )
					if ( ::SetFileTime( file.Get(), triplet[ CreatedDate ], triplet[ AccessedDate ], triplet[ ModifiedDate ] ) != FALSE )
						if ( file.Close() )
							return;

				impl::ThrowMfcErrorAs( policy, impl::NewLastErrorException( pFilePath ) );
			}
		}

		FILETIME* MakeFileTime( FILETIME& rOutFileTime, const CTime& time, const TCHAR* pFilePath,
								fs::ExcPolicy policy /*= fs::RuntimeExc*/ ) throws_( CRuntimeException, CFileException* )
		{
			if ( !time_utl::IsValid( time ) )
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
				impl::ThrowMfcErrorAs( policy, impl::NewLastErrorException( pFilePath ) );

			// convert local file time to UTC file time
			if ( !::LocalFileTimeToFileTime( &localTime, &rOutFileTime ) )
				impl::ThrowMfcErrorAs( policy, impl::NewLastErrorException( pFilePath ) );

			return &rOutFileTime;
		}

		void TouchFile( const fs::CPath& filePath, const CTime& time /*= CTime::GetCurrentTime()*/, TimeField timeField /*= ModifiedDate*/,
						fs::ExcPolicy policy /*= fs::RuntimeExc*/ ) throws_( CRuntimeException, CFileException* )
		{
			WriteFileTime( filePath.GetPtr(), timeField, time, policy );
		}

		void TouchFileBy( const fs::CPath& filePath, const CTimeSpan& bySpan, TimeField timeField /*= ModifiedDate*/,
						  fs::ExcPolicy policy /*= fs::RuntimeExc*/ ) throws_( CRuntimeException, CFileException* )
		{
			WriteFileTime( filePath.GetPtr(), timeField, ReadFileTime( filePath, timeField, policy ) + bySpan, policy );
		}
	}
}


namespace fs
{
	namespace impl
	{
		CFileException* NewLastErrorException( const TCHAR* pFilePath, LONG lastError /*= ::GetLastError()*/ )
		{
			return lastError != 0
				? new CFileException( CFileException::OsErrorToException( lastError ), lastError, pFilePath )
				: NULL;
		}

		CFileException* NewErrnoException( const TCHAR* pFilePath, unsigned long errNo )	// = _doserrno
		{
			return errNo != 0
				? new CFileException( CFileException::ErrnoToException( errNo != -1 ? errNo : (int)_doserrno ), _doserrno, pFilePath )
				: NULL;
		}

		void __declspec( noreturn ) ThrowMfcErrorAs( ExcPolicy policy, CFileException* pExc ) throws_( CRuntimeException, CFileException* )
		{
			if ( pExc != NULL )
				switch ( policy )
				{
					case RuntimeExc:
						throw CRuntimeException( str::Enquote< std::tstring >( std::auto_ptr< CFileException >( pExc ).get()->m_strFileName ) );	// pExc will be deleted
					case MfcExc:
						throw pExc;
					default: ASSERT( false );
				}
		}

		void __declspec( noreturn ) ThrowFileException( const std::tstring& description, const TCHAR* pFilePath, const TCHAR sep[] /*= _T(": ")*/ ) throws_( CRuntimeException )
		{
			std::tstring message = description;
			stream::Tag( message, str::Enquote< std::tstring >( pFilePath ), sep );

			throw CRuntimeException( message );
		}
	}
}


namespace fs
{
	// CScopedWriteableFile implementation

	CScopedWriteableFile::CScopedWriteableFile( const TCHAR* pFilePath )
		: m_pFilePath( pFilePath )
		, m_origAttr( ::GetFileAttributes( m_pFilePath ) )
	{
		if ( m_origAttr != INVALID_FILE_ATTRIBUTES )
			if ( HasFlag( m_origAttr, FILE_ATTRIBUTE_READONLY ) )										// currently readonly?
				if ( !::SetFileAttributes( m_pFilePath, m_origAttr & ~FILE_ATTRIBUTE_READONLY ) )		// temporarily allow changing file times for a read-only file/directory
				{
					m_origAttr = INVALID_FILE_ATTRIBUTES;												// failed removing FILE_ATTRIBUTE_READONLY
					TRACE( _T("* fs::CScopedWriteableFile::CScopedWriteableFile - SetFileAttributes() failed for %s\n"), str::Enquote< std::tstring >( m_pFilePath ).c_str() );
				}
	}

	CScopedWriteableFile::~CScopedWriteableFile()
	{
		if ( m_origAttr != INVALID_FILE_ATTRIBUTES )
			if ( HasFlag( m_origAttr, FILE_ATTRIBUTE_READONLY ) )			// was readonly?
				if ( !::SetFileAttributes( m_pFilePath, m_origAttr ) )		// restore original FILE_ATTRIBUTE_READONLY
					TRACE( _T("* fs::CScopedWriteableFile::~CScopedWriteableFile - SetFileAttributes() failed for %s\n"), str::Enquote< std::tstring >( m_pFilePath ).c_str() );
	}


	// CScopedFileTime implementation

	CScopedFileTime::CScopedFileTime( const fs::CPath& filePath, TimeField timeField /*= fs::ModifiedDate*/ )
		: m_filePath( filePath )
		, m_timeField( timeField )
		, m_origFileTime( fs::ReadFileTime( m_filePath, m_timeField ) )
	{
		if ( !time_utl::IsValid( m_origFileTime ) )
			TRACE( _T("* fs::CScopedFileTime::CScopedFileTime - ReadFileTime() failed for %s\n"), str::Enquote< std::tstring >( m_filePath ).c_str() );
	}

	CScopedFileTime::~CScopedFileTime()
	{
		if ( time_utl::IsValid( m_origFileTime ) )
			try
			{
				fs::thr::WriteFileTime( m_filePath.GetPtr(), m_timeField, m_origFileTime );
			}
			catch ( CRuntimeException& exc )
			{
				exc;
				TRACE( _T("* fs::CScopedFileTime::CScopedFileTime - WriteFileTime() failed for %s\n"), exc.GetMessage().c_str() );
			}
	}
}


namespace fs
{
	// CEnumerator implementation

	void CEnumerator::Clear( void )
	{
		m_filePaths.clear();
		m_subDirPaths.clear();
	}

	void CEnumerator::AddFoundFile( const TCHAR* pFilePath )
	{
		fs::CPath filePath( pFilePath );

		if ( !m_relativeDirPath.IsEmpty() )
			filePath = fs::StripDirPrefix( filePath, m_relativeDirPath );

		if ( !filePath.IsEmpty() )
			m_filePaths.push_back( filePath );

		if ( m_pChainEnum != NULL )
			m_pChainEnum->AddFoundFile( pFilePath );
	}

	void CEnumerator::AddFoundSubDir( const TCHAR* pSubDirPath )
	{
		fs::CPath subDirPath( pSubDirPath );

		if ( !m_relativeDirPath.IsEmpty() )
			subDirPath = fs::StripDirPrefix( subDirPath, m_relativeDirPath );

		if ( !subDirPath.IsEmpty() )
			m_subDirPaths.push_back( subDirPath );

		if ( m_pChainEnum != NULL )
			m_pChainEnum->AddFoundSubDir( pSubDirPath );
	}

	bool CEnumerator::MustStop( void ) const
	{
		return m_filePaths.size() >= m_maxFiles;
	}


	void EnumFiles( IEnumerator* pEnumerator, const fs::CPath& dirPath, const TCHAR* pWildSpec /*= _T("*.*")*/, RecursionDepth depth /*= Shallow*/ )
	{
		ASSERT_PTR( pEnumerator );

		if ( str::IsEmpty( pWildSpec ) )
			pWildSpec = _T("*");

		fs::CPath dirPathFilter = dirPath /
			fs::CPath( Deep == depth || path::IsMultipleWildcard( pWildSpec )
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
				if ( path::MatchWildcard( foundPath.c_str(), pWildSpec ) )
					pEnumerator->AddFile( finder );
			}
		}

		if ( Deep == depth && !pEnumerator->MustStop() )
		{
			fs::SortPaths( subDirPaths );		// natural path order

			for ( std::vector< fs::CPath >::const_iterator itSubDirPath = subDirPaths.begin(); itSubDirPath != subDirPaths.end(); ++itSubDirPath )
				EnumFiles( pEnumerator, *itSubDirPath, pWildSpec, depth );
		}
	}

	void EnumFiles( std::vector< fs::CPath >& rFilePaths, const fs::CPath& dirPath, const TCHAR* pWildSpec /*= _T("*")*/, RecursionDepth depth /*= Shallow*/ )
	{
		CEnumerator found;
		EnumFiles( &found, dirPath, pWildSpec, depth );

		rFilePaths.assign( found.m_filePaths.begin(), found.m_filePaths.end() );
	}

	void EnumSubDirs( std::vector< fs::CPath >& rSubDirPaths, const fs::CPath& dirPath, const TCHAR* pWildSpec /*= _T("*.*")*/, RecursionDepth depth /*= Shallow*/ )
	{
		CEnumerator found;
		EnumFiles( &found, dirPath, pWildSpec, depth );

		rSubDirPaths.assign( found.m_subDirPaths.begin(), found.m_subDirPaths.end() );
	}


	fs::CPath FindFirstFile( const fs::CPath& dirPath, const TCHAR* pWildSpec /*= _T("*.*")*/, RecursionDepth depth /*= Shallow*/ )
	{
		CFirstFileEnumerator singleEnumer;
		EnumFiles( &singleEnumer, dirPath, pWildSpec, depth );
		return singleEnumer.GetFoundPath();
	}

} //namespace fs


namespace fs
{
	namespace impl
	{
		UINT QueryExistingSequenceCount( const fs::CPathParts& filePathParts )
		{
			fs::CPath dirPath = filePathParts.GetDirPath();
			std::vector< fs::CPath > existingPaths;
			UINT seqCount = 0;

			if ( fs::IsValidDirectory( dirPath.GetPtr() ) )
			{
				std::tstring wildSpec = filePathParts.m_fname + _T("*") + filePathParts.m_ext;

				if ( fs::IsValidDirectory( filePathParts.MakePath().GetPtr() ) )
					fs::EnumSubDirs( existingPaths, dirPath, wildSpec.c_str(), Shallow );
				else
					fs::EnumFiles( existingPaths, dirPath, wildSpec.c_str(), Shallow );

				seqCount = static_cast< UINT >( existingPaths.size() );
			}

			const size_t prefixLen = filePathParts.m_fname.length();

			for ( std::vector< fs::CPath >::const_iterator itExistingPath = existingPaths.begin(); itExistingPath != existingPaths.end(); ++itExistingPath )
			{
				fs::CPathParts parts( itExistingPath->GetFilename() );
				ASSERT( str::HasPrefixI( parts.m_fname.c_str(), filePathParts.m_fname.c_str() ) );

				const TCHAR* pNumber = parts.m_fname.c_str() + prefixLen;		// skip past original fname to search for digits

				while ( *pNumber != _T('\0') && !str::CharTraits::IsDigit( *pNumber ) )
					++pNumber;

				if ( *pNumber != _T('\0') )
				{
					UINT number = 0;
					if ( num::ParseNumber( number, pNumber ) )
						seqCount = std::max( seqCount, number );
				}
			}

			return seqCount;
		}
	}

	fs::CPath MakeUniqueNumFilename( const fs::CPath& filePath, const TCHAR fmtNumSuffix[] /*= _T("_[%d]")*/ ) throws_( CRuntimeException )
	{
		ASSERT( !filePath.IsEmpty() );
		if ( !filePath.FileExist() )
			return filePath;						// no filename collision

		fs::CPathParts parts( filePath.Get() );
		const std::tstring fnameBase = parts.m_fname;
		std::tstring lastSuffix;
		fs::CPath uniqueFilePath;

		for ( UINT seqCount = std::max( 2u, impl::QueryExistingSequenceCount( parts ) + 1 ); ; ++seqCount )
		{
			std::tstring suffix = str::Format( fmtNumSuffix, seqCount );
			if ( lastSuffix == suffix )
				throw CRuntimeException( str::Format( _T("Fishy numeric suffix format '%s'. It does not produce unique filenames."), fmtNumSuffix ) );
			else
				lastSuffix = suffix;

			parts.m_fname = fnameBase + suffix;		// e.g. "fnameBase_[N]"
			uniqueFilePath = parts.MakePath();

			if ( !uniqueFilePath.FileExist() )		// path is unique, done
				break;
		}
		return uniqueFilePath;
	}

	fs::CPath MakeUniqueHashedFilename( const fs::CPath& filePath, const TCHAR fmtHashSuffix[] /*= _T("_%08X")*/ )
	{
		ASSERT( !filePath.IsEmpty() );
		if ( !filePath.FileExist() )
			return filePath;						// no filename collision

		fs::CPathParts parts( filePath.Get() );

		const UINT hashKey = static_cast< UINT >( path::GetHashValue( filePath.GetPtr() ) );		// hash key is unique for the whole path
		parts.m_fname += str::Format( fmtHashSuffix, hashKey );		// e.g. "fname_hexHashKey"
		fs::CPath uniqueFilePath = parts.MakePath();

		ENSURE( !uniqueFilePath.FileExist() );		// hashed path is supposed to be be unique

		return uniqueFilePath;
	}
}
