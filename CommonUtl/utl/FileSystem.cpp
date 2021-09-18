
#include "stdafx.h"
#include "FileSystem.h"
#include "FileEnumerator.h"
#include "EnumTags.h"
#include "FlexPath.h"
#include "RuntimeException.h"
#include "ContainerUtilities.h"
#include "TimeUtils.h"
#include <stdexcept>
#include <shlwapi.h>				// for PathRelativePathTo

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

	bool IsValidEmptyDirectory( const TCHAR* pDirPath )
	{
		return ::PathIsDirectoryEmpty( pDirPath ) != FALSE;
	}

	bool IsValidShellLink( const TCHAR* pFilePath )
	{
		return IsValidFile( pFilePath ) && path::MatchExt( pFilePath, _T(".lnk") );
	}

	bool IsValidStructuredStorage( const TCHAR* pDocFilePath )
	{
		HRESULT hResult = ::StgIsStorageFile( pDocFilePath );
		return S_OK == hResult;
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


	void QueryFolderPaths( std::vector< fs::CPath >& rFolderPaths, const std::vector< fs::CPath >& filePaths, bool uniqueOnly /*= true*/ )
	{
		for ( std::vector< fs::CPath >::const_iterator itFilePath = filePaths.begin(); itFilePath != filePaths.end(); ++itFilePath )
		{
			fs::CPath folderPath;

			if ( fs::IsValidDirectory( itFilePath->GetPtr() ) )
				folderPath = *itFilePath;
			else if ( fs::IsValidFile( itFilePath->GetPtr() ) )
				folderPath = itFilePath->GetParentPath();
			else
				continue;

			if ( uniqueOnly )
				utl::AddUnique( rFolderPaths, folderPath );
			else
				rFolderPaths.push_back( folderPath );
		}
	}


	fs::CPath GetModuleFilePath( HINSTANCE hInstance )
	{
		TCHAR modulePath[ MAX_PATH ];
		::GetModuleFileName( hInstance, modulePath, COUNT_OF( modulePath ) );				// may return short path, depending on how it was invoked

		TCHAR longModulePath[ MAX_PATH ];
		::GetLongPathName( modulePath, longModulePath, COUNT_OF( longModulePath ) );		// convert to long path

		return fs::CPath( longModulePath );
	}

	fs::CPath GetTempDirPath( void )
	{
		TCHAR dirPath[ MAX_PATH ];
		if ( 0 == ::GetTempPath( MAX_PATH, dirPath ) )
			dirPath[ 0 ] = _T('\0');

		return fs::CPath( dirPath );
	}


	fs::CPath MakeAbsoluteToCWD( const TCHAR* pRelativePath )
	{
		std::tstring relativePath = path::MakeNormal( pRelativePath );
		TCHAR absolutePath[ MAX_PATH ];
		if ( NULL == ::_tfullpath( absolutePath, relativePath.c_str(), COUNT_OF( absolutePath ) ) )
			return std::tstring();			// failed

		return fs::CPath( absolutePath );
	}

	fs::CPath MakeRelativeTo( const TCHAR* pFromPath, const fs::CPath& dirPath )
	{
		std::tstring fromPath = path::MakeNormal( pFromPath );
		DWORD fromAttr = IsValidDirectory( fromPath.c_str() ) ? FILE_ATTRIBUTE_DIRECTORY : 0;
		TCHAR relativePath[ MAX_PATH ];

		if ( !::PathRelativePathTo( relativePath, fromPath.c_str(), fromAttr, dirPath.GetPtr(), FILE_ATTRIBUTE_DIRECTORY ) )
			return fromPath;		// on error return input path as is (but normalized)

		return fs::CPath( relativePath );
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

	bool DeleteDir( const TCHAR* pDirPath, RecursionDepth depth /*= Deep*/ )
	{
		DWORD dirAttr = ::GetFileAttributes( pDirPath );
		if ( INVALID_FILE_ATTRIBUTES == dirAttr )
			return false;

		if ( HasFlag( dirAttr, FILE_ATTRIBUTE_READONLY ) )								// read-only file?
			::SetFileAttributes( pDirPath, dirAttr & ~FILE_ATTRIBUTE_READONLY );		// make it writeable so that it can be deleted

		if ( Deep == depth )
			if ( !DeleteAllFiles( pDirPath ) )
				return false;

		return ::RemoveDirectory( pDirPath ) != FALSE;
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
	std::auto_ptr< CFile > OpenFile( const fs::CPath& filePath, bool throwMode /*= false*/, DWORD mode /*= CFile::modeRead | CFile::typeBinary*/ ) throws_( CFileException* )
	{
		ASSERT( !filePath.IsComplexPath() );

		static mfc::CAutoException< CFileException > s_fileError;
		s_fileError.m_strFileName = filePath.GetPtr();

		std::auto_ptr< CFile > pFile( new CFile );
		if ( !pFile->Open( filePath.GetPtr(), mode | CFile::shareDenyWrite, &s_fileError ) )		// note: CFile::shareExclusive causes sharing violation
			if ( throwMode )
				throw &s_fileError;
			else
				pFile.reset();

		return pFile;
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


	// CTextFileWriter implementation

	CTextFileWriter::CTextFileWriter( CFile* pDestFile, bool isBinaryFile /*= true*/ )
		: m_pDestFile( pDestFile )
		, m_lineEnd( isBinaryFile ? _T("\r\n") : _T("\n") )
	{
		ASSERT_PTR( m_pDestFile );			// the file must be open for writing (or created)
	}

	void CTextFileWriter::WriteText( const std::tstring& text )
	{
		std::string textUtf8 = str::ToUtf8( text.c_str() );
		m_pDestFile->Write( textUtf8.c_str(), static_cast<UINT>( textUtf8.length() ) );
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
		ASSERT( !path::IsComplex( pFilePath ) );

		_stat64 fileStatus;
		if ( 0 == _tstat64( pFilePath, &fileStatus ) )
			return static_cast< UINT64 >( fileStatus.st_size );

		return ULLONG_MAX;			// error, could use errno to find out more
	}

	CTime ReadFileTime( const fs::CPath& filePath, TimeField timeField )
	{
		_stat64i32 fileStatus;
		if ( 0 == _tstat( path::ExtractPhysical( filePath.Get() ).c_str(), &fileStatus ) )			// translate to physical path
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


	FILETIME* MakeFileTime( FILETIME& rOutFileTime, const CTime& time )
	{
		if ( time_utl::IsValid( time ) )
		{
			SYSTEMTIME sysTime;
			sysTime.wYear = (WORD)time.GetYear();
			sysTime.wMonth = (WORD)time.GetMonth();
			sysTime.wDay = (WORD)time.GetDay();
			sysTime.wHour = (WORD)time.GetHour();
			sysTime.wMinute = (WORD)time.GetMinute();
			sysTime.wSecond = (WORD)time.GetSecond();
			sysTime.wMilliseconds = 0;

			FILETIME localTime;

			if ( ::SystemTimeToFileTime( &sysTime, &localTime ) )					// convert system time to local file time
				if ( ::LocalFileTimeToFileTime( &localTime, &rOutFileTime ) )		// convert local file time to UTC file time
					return &rOutFileTime;
		}

		return NULL;

	}


	namespace thr
	{
		CTime ReadFileTime( const fs::CPath& filePath, TimeField timeField,
							fs::ExcPolicy policy /*= fs::RuntimeExc*/ ) throws_( CRuntimeException, CFileException* )
		{
			_stat64i32 fileStatus;
			int result = _tstat( path::ExtractPhysical( filePath.Get() ).c_str(), &fileStatus );			// translate to physical path
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
			ASSERT( !path::IsComplex( pFilePath ) );
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
			FILETIME* pResult = fs::MakeFileTime( rOutFileTime, time );		// call the no-throw function

			if ( NULL == pResult && time_utl::IsValid( time ) )
				impl::ThrowMfcErrorAs( policy, impl::NewLastErrorException( pFilePath ) );

			return pResult;
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
						throw CRuntimeException( str::sq::Enquote( std::auto_ptr< CFileException >( pExc ).get()->m_strFileName.GetString() ) );	// pExc will be deleted
					case MfcExc:
						throw pExc;
					default: ASSERT( false );
				}
		}

		void __declspec( noreturn ) ThrowFileException( const std::tstring& description, const TCHAR* pFilePath, const TCHAR sep[] /*= _T(": ")*/ ) throws_( CRuntimeException )
		{
			std::tstring message = description;
			stream::Tag( message, str::sq::Enquote( pFilePath ), sep );

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
					TRACE( _T("* fs::CScopedWriteableFile::CScopedWriteableFile - SetFileAttributes() failed for %s\n"), str::sq::Enquote( m_pFilePath ).c_str() );
				}
	}

	CScopedWriteableFile::~CScopedWriteableFile()
	{
		if ( m_origAttr != INVALID_FILE_ATTRIBUTES )
			if ( HasFlag( m_origAttr, FILE_ATTRIBUTE_READONLY ) )			// was readonly?
				if ( !::SetFileAttributes( m_pFilePath, m_origAttr ) )		// restore original FILE_ATTRIBUTE_READONLY
					TRACE( _T("* fs::CScopedWriteableFile::~CScopedWriteableFile - SetFileAttributes() failed for %s\n"), str::sq::Enquote( m_pFilePath ).c_str() );
	}


	// CScopedFileTime implementation

	CScopedFileTime::CScopedFileTime( const fs::CPath& filePath, TimeField timeField /*= fs::ModifiedDate*/ )
		: m_filePath( filePath )
		, m_timeField( timeField )
		, m_origFileTime( fs::ReadFileTime( m_filePath, m_timeField ) )
	{
		if ( !time_utl::IsValid( m_origFileTime ) )
			TRACE( _T("* fs::CScopedFileTime::CScopedFileTime - ReadFileTime() failed for %s\n"), str::sq::EnquoteStr( m_filePath ).c_str() );
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
