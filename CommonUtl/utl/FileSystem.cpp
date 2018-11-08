
#include "stdafx.h"
#include "FileSystem.h"
#include "StringUtilities.h"
#include "EnumTags.h"
#include "FlexPath.h"
#include "RuntimeException.h"
#include <shlwapi.h>
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

	std::tstring MakeAbsoluteToCWD( const TCHAR* pRelativePath )
	{
		std::tstring relativePath = path::MakeNormal( pRelativePath );
		TCHAR absolutePath[ _MAX_PATH ];
		if ( NULL == _tfullpath( absolutePath, relativePath.c_str(), COUNT_OF( absolutePath ) ) )
			return std::tstring();			// failed

		return absolutePath;
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

	std::tstring MakeUniqueNumFilename( const TCHAR* pFullPath )
	{
		ASSERT( !str::IsEmpty( pFullPath ) );
		if ( !FileExist( pFullPath ) )
			return pFullPath;					// no filename collision

		fs::CPathParts parts( pFullPath );
		const std::tstring fnameBase = parts.m_fname;
		std::tstring uniqueFullPath;

		for ( UINT seqCount = 2; ; ++seqCount )
		{
			parts.m_fname = str::Format( _T("%s_[%d]"), fnameBase.c_str(), seqCount );		// "fnameBase_[N]"
			uniqueFullPath = parts.MakePath();

			if ( !FileExist( uniqueFullPath.c_str() ) )					// unique, done
				break;
		}
		return uniqueFullPath;
	}

	std::tstring MakeUniqueHashedFilename( const TCHAR* pFullPath )
	{
		std::tstring uniqueFullPath = pFullPath;
		if ( FileExist( pFullPath ) )
		{
			fs::CPathParts parts( uniqueFullPath );

			const UINT hashKey = static_cast< UINT >( path::GetHashValue( pFullPath ) );		// hash key is unique for the whole path
			parts.m_fname += str::Format( _T("_%08X"), hashKey );		// "fname_hexHashKey"
			uniqueFullPath = parts.MakePath();

			ENSURE( !FileExist( uniqueFullPath.c_str() ) );				// unique
		}
		return uniqueFullPath;
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

	void EnsureDirPath( const TCHAR* pDirPath ) throws_( CRuntimeException )
	{
		if ( !CreateDirPath( pDirPath ) )
			throw CRuntimeException( str::Format( _T("Cannot create directory path: %s"), pDirPath ) );
	}

	void EnsureDirPath_Mfc( const TCHAR* pDirPath ) throws_( CFileException )
	{
		if ( !CreateDirPath( pDirPath ) )
			AfxThrowFileException( CFileException::badPath, -1, pDirPath );
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

	bool DeleteFile( const TCHAR* pFilePath )
	{
		DWORD fileAttr = ::GetFileAttributes( pFilePath );
		if ( INVALID_FILE_ATTRIBUTES == fileAttr )
			return false;

		if ( HasFlag( fileAttr, FILE_ATTRIBUTE_READONLY ) )								// read-only file?
			::SetFileAttributes( pFilePath, fileAttr & ~FILE_ATTRIBUTE_READONLY );		// make it writeable so that it can be deleted

		return ::DeleteFile( pFilePath ) != FALSE;
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

	ULONGLONG BufferedCopy( CFile& rDestFile, CFile& srcFile, size_t chunkSize /*= 4 * KiloByte*/ )
	{
		ULONGLONG fileSize = srcFile.GetLength();
		std::vector< BYTE > buffer;
		buffer.resize( (std::min)( chunkSize, static_cast< size_t >( fileSize ) ) );		// grow the size of the copy buffer as needed

		for ( ULONGLONG bytesLeft = fileSize; bytesLeft != 0; )
		{
			UINT bytesRead = srcFile.Read( &buffer.front(), static_cast< UINT >( buffer.size() ) );
			rDestFile.Write( &buffer.front(), bytesRead );
			bytesLeft -= bytesRead;

			ENSURE( bytesLeft < fileSize );			// no underflow
		}
		return fileSize;
	}

	CComPtr< IStream > DuplicateToMemoryStream( IStream* pSrcStream, bool autoDelete /*= true*/ )
	{
		CComPtr< IStream > pDestStream;
		ULARGE_INTEGER size;
		if ( HR_OK( ::IStream_Size( pSrcStream, &size ) ) )
		{
			ASSERT( 0 == size.u.HighPart );				// shouldn't be huge

			if ( HR_OK( ::CreateStreamOnHGlobal( NULL, autoDelete, &pDestStream ) ) )
				if ( HR_OK( ::IStream_Copy( pSrcStream, pDestStream, size.u.LowPart ) ) )
					return pDestStream;
		}

		return NULL;
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


	ULONGLONG GetFileSize( const TCHAR* pFilePath )
	{
		_stat64 fileStatus;
		if ( 0 == _tstat64( pFilePath, &fileStatus ) )
			return static_cast< ULONGLONG >( fileStatus.st_size );

		return ULLONG_MAX;			// error, could use errno to find out more
	}

	CTime ReadLastModifyTime( const fs::CPath& filePath )
	{
		_stat64i32 fileStatus;
		if ( 0 == _tstat( filePath.GetPtr(), &fileStatus ) )
			return fileStatus.st_mtime;
		return 0;
	}

	const CEnumTags& GetTags_FileExpireStatus( void )
	{
		static const CEnumTags tags( _T("|source file modified|source file deleted") );
		return tags;
	}

	FileExpireStatus CheckExpireStatus( const fs::CPath& filePath, const CTime& lastModifyTime )
	{
		CTime currModifTime = ReadLastModifyTime( filePath );
		if ( 0 == currModifTime.GetTime() )				// probably image file deleted -> expired
			return ExpiredFileDeleted;

		return currModifTime > lastModifyTime ? ExpiredFileModified : FileNotExpired;
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

		fs::TPathSet subDirPaths;			// sorted intuitively
		std::tstring dirPathSpecAll = path::Combine( pDirPath, _T("*") );

		CFileFind finder;
		for ( BOOL found = finder.FindFile( dirPathSpecAll.c_str() ); found;  )
		{
			found = finder.FindNextFile();
			std::tstring foundPath = finder.GetFilePath().GetString();

			if ( finder.IsDirectory() )
			{
				if ( !finder.IsDots() )						// skip "." and ".." dir entries
				{
					pEnumerator->AddFoundSubDir( foundPath.c_str() );
					if ( Deep == depth )
						subDirPaths.insert( fs::CPath( foundPath ) );
				}
			}
			else
			{
				if ( path::MatchWildcard( foundPath.c_str(), pWildSpec ) )
					pEnumerator->AddFile( finder );
			}
		}

		if ( Deep == depth )
			for ( fs::TPathSet::const_iterator itSubDirPath = subDirPaths.begin(); itSubDirPath != subDirPaths.end(); ++itSubDirPath )
				EnumFiles( pEnumerator, itSubDirPath->GetPtr(), pWildSpec, depth );
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
