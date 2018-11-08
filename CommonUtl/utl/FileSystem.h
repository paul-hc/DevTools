#ifndef FileSystem_h
#define FileSystem_h
#pragma once

#include "FileSystem_fwd.h"


namespace fs
{
	bool IsValidFile( const TCHAR* pFilePath );
	bool IsValidDirectory( const TCHAR* pDirPath );

	std::tstring MakeAbsoluteToCWD( const TCHAR* pRelativePath );				// normalize, strip "./", "../", relative to CWD - current working directory
	inline std::tstring& AbsolutizeToCWD( std::tstring& rPath ) { return rPath = MakeAbsoluteToCWD( rPath.c_str() ); }

	std::tstring GetTempDirPath( void );
	std::tstring MakeTempDirPath( const TCHAR* pSubPath );

	std::tstring MakeUniqueNumFilename( const TCHAR* pFullPath );				// make file name unique by avoiding collisions with existing files
	std::tstring MakeUniqueHashedFilename( const TCHAR* pFullPath );			// make file name unique by avoiding collisions with existing files

	bool DeleteFile( const TCHAR* pFilePath );									// delete even if read-only file

	bool CreateDir( const TCHAR* pDirPath );
	bool CreateDirPath( const TCHAR* pDirPath );								// creates deep directory path, returns true if a valid directory path
	bool DeleteDir( const TCHAR* pDirPath );
	bool DeleteAllFiles( const TCHAR* pDirPath );								// delete even if read-only sub-directories or embedded files

	void EnsureDirPath( const TCHAR* pDirPath ) throws_( CRuntimeException );	// same as CreateDirPath() but throws on error
	void EnsureDirPath_Mfc( const TCHAR* pDirPath ) throws_( CFileException );	// throws CFileException on error


	UINT64 BufferedCopy( CFile& rDestFile, CFile& srcFile, size_t chunkSize = 4 * KiloByte );		// 4096 works well because it's quicker to allocate

	CComPtr< IStream > DuplicateToMemoryStream( IStream* pSrcStream, bool autoDelete = true );
}


class CEnumTags;


namespace fs
{
	const WIN32_FIND_DATA* GetFindData( const CFileFind& foundFile );
	inline DWORD GetFileAttributes( const CFileFind& foundFile ) { return GetFindData( foundFile )->dwFileAttributes; }

	UINT64 GetFileSize( const TCHAR* pFilePath );

	CTime ReadLastModifyTime( const fs::CPath& filePath );

	enum FileExpireStatus { FileNotExpired, ExpiredFileModified, ExpiredFileDeleted };

	const CEnumTags& GetTags_FileExpireStatus( void );

	FileExpireStatus CheckExpireStatus( const fs::CPath& filePath, const CTime& lastModifyTime );
}


namespace fs
{
	// files and sub-dirs as std::tstring; relative to m_refDirPath if specified

	struct CEnumerator : public IEnumerator, private utl::noncopyable
	{
		CEnumerator( const std::tstring refDirPath = std::tstring() ) : m_pChainEnum( NULL ), m_refDirPath( refDirPath ) {}
		CEnumerator( IEnumerator* pChainEnum ) : m_pChainEnum( pChainEnum ) {}

		// IEnumerator interface
		virtual void AddFoundFile( const TCHAR* pFilePath );
		virtual void AddFoundSubDir( const TCHAR* pSubDirPath );
	protected:
		IEnumerator* m_pChainEnum;					// allows chaining for progress reporting
		std::tstring m_refDirPath;					// to remove if common prefix
	public:
		std::vector< std::tstring > m_filePaths;
		std::vector< std::tstring > m_subDirPaths;
	};


	// files and sub-dirs as fs::CPath, sorted intuitively; relative to m_refDirPath if specified

	struct CPathEnumerator : public IEnumerator
	{
		CPathEnumerator( const std::tstring refDirPath = std::tstring() ) : m_pChainEnum( NULL ), m_refDirPath( refDirPath ) {}
		CPathEnumerator( IEnumerator* pChainEnum ) : m_pChainEnum( pChainEnum ) {}

		// IEnumerator interface
		virtual void AddFoundFile( const TCHAR* pFilePath );
		virtual void AddFoundSubDir( const TCHAR* pSubDirPath );
	protected:
		IEnumerator* m_pChainEnum;					// allows chaining for progress reporting
		std::tstring m_refDirPath;					// to remove if common prefix
	public:
		fs::TPathSet m_filePaths;					// sorted intuitively
		fs::TPathSet m_subDirPaths;
	};


	// pWildSpec can be multiple: "*.*", "*.doc;*.txt"

	void EnumFiles( IEnumerator* pEnumerator, const TCHAR* pDirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );

	void EnumFiles( std::vector< std::tstring >& rFilePaths, const TCHAR* pDirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );
	void EnumSubDirs( std::vector< std::tstring >& rSubDirPaths, const TCHAR* pDirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );

	fs::CPath FindFirstFile( const TCHAR* pDirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );
}


namespace pred
{
	struct IsValidFile
	{
		bool operator()( const TCHAR* pFilePath ) const { return fs::IsValidFile( pFilePath ); }
		bool operator()( const fs::CPath& filePath ) const { return fs::IsValidFile( filePath.GetPtr() ); }
	};

	struct IsValidDirectory
	{
		bool operator()( const TCHAR* pFilePath ) const { return fs::IsValidDirectory( pFilePath ); }
		bool operator()( const fs::CPath& filePath ) const { return fs::IsValidDirectory( filePath.GetPtr() ); }
	};
}


namespace fs
{
	template< typename PathContainerT >
	void SortDirectoriesFirst( PathContainerT& rPaths )
	{
		typename PathContainerT::iterator itFirstFile = std::stable_partition( rPaths.begin(), rPaths.end(), pred::IsValidDirectory() );

		std::sort( rPaths.begin(), itFirstFile );		// directories come first
		std::sort( itFirstFile, rPaths.end() );
	}
}


#endif // FileSystem_h
