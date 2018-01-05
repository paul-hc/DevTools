#ifndef FileSystem_h
#define FileSystem_h
#pragma once

#include "Path.h"


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

	bool CreateDir( const TCHAR* pDirPath );
	bool CreateDirPath( const TCHAR* pDirPath );								// creates deep directory path, returns true if a valid directory path
	void EnsureDirPath( const TCHAR* pDirPath ) throws_( CFileException );		// same as CreateDirPath() but throws on error
	bool DeleteDir( const TCHAR* pDirPath );
	bool DeleteAllFiles( const TCHAR* pDirPath );


	ULONGLONG BufferedCopy( CFile& rDestFile, CFile& srcFile, size_t chunkSize = 4 * KiloByte );		// 4096 works well because it's quicker to allocate

	CComPtr< IStream > DuplicateToMemoryStream( IStream* pSrcStream, bool autoDelete = true );
}


namespace fs
{
	interface IEnumerator
	{
		virtual void AddFoundFile( const TCHAR* pFilePath ) = 0;
		virtual void AddFoundSubDir( const TCHAR* pSubDirPath ) { pSubDirPath; }

		// advanced, provides extra info
		virtual void AddFile( const CFileFind& foundFile ) { AddFoundFile( foundFile.GetFilePath() ); }

		// override to find first file, then abort searching
		virtual bool MustStop( void ) { return false; }
	};


	// files and sub-dirs as std::tstring; relative to m_refDirPath if specified

	struct CEnumerator : public IEnumerator, private utl::noncopyable
	{
		CEnumerator( const std::tstring refDirPath = std::tstring() ) : m_refDirPath( refDirPath ) {}

		// IEnumerator interface
		virtual void AddFoundFile( const TCHAR* pFilePath );
		virtual void AddFoundSubDir( const TCHAR* pSubDirPath );
	protected:
		std::tstring m_refDirPath;						// to remove if common prefix
	public:
		std::vector< std::tstring > m_filePaths;
		std::vector< std::tstring > m_subDirPaths;
	};


	// files and sub-dirs as fs::CPath, sorted intuitively; relative to m_refDirPath if specified

	struct CPathEnumerator : public IEnumerator
	{
		CPathEnumerator( const std::tstring refDirPath = std::tstring() ) : m_refDirPath( refDirPath ) {}

		// IEnumerator interface
		virtual void AddFoundFile( const TCHAR* pFilePath );
		virtual void AddFoundSubDir( const TCHAR* pSubDirPath );
	protected:
		std::tstring m_refDirPath;						// to remove if common prefix
	public:
		fs::PathSet m_filePaths;						// sorted intuitively
		fs::PathSet m_subDirPaths;
	};


	// pWildSpec can be multiple: "*.*", "*.doc;*.txt"

	void EnumFiles( IEnumerator* pEnumerator, const TCHAR* pDirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );

	void EnumFiles( std::vector< std::tstring >& rFilePaths, const TCHAR* pDirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );
	void EnumSubDirs( std::vector< std::tstring >& rSubDirPaths, const TCHAR* pDirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );

	fs::CPath FindFirstFile( const TCHAR* pDirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );
}


#endif // FileSystem_h
