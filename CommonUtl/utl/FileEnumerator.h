#ifndef FileEnumerator_h
#define FileEnumerator_h
#pragma once

#include "FileSystem_fwd.h"


namespace fs
{
	// pWildSpec can be multiple: "*.*", "*.doc;*.txt"

	void EnumFiles( IEnumerator* pEnumerator, const fs::CPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow, bool sortSubDirs = true );

	size_t EnumFilePaths( std::vector< fs::CPath >& rFilePaths, const fs::CPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );
	size_t EnumSubDirPaths( std::vector< fs::CPath >& rSubDirPaths, const fs::CPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );

	fs::CPath FindFirstFile( const fs::CPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );


	// generate a path of a unique filename using a suffix, by avoiding collisions with existing files
	fs::CPath MakeUniqueNumFilename( const fs::CPath& filePath, const TCHAR fmtNumSuffix[] = path::StdFormatNumSuffix() ) throws_( CRuntimeException );	// with numeric suffix
	fs::CPath MakeUniqueHashedFilename( const fs::CPath& filePath, const TCHAR fmtHashSuffix[] = _T("_%08X") );											// with hash suffix

	// late binding to shell::ResolveShortcut defined in UTL_UI.lib
	typedef bool (*ResolveShortcutProc)( fs::CPath& rDestPath, const TCHAR* pShortcutLnkPath, CWnd* pWnd );

	void StoreResolveShortcutProc( ResolveShortcutProc resolveShortcutProc );
}


namespace fs
{
	class CPathMatches;


	// files and sub-dirs in existing file-system order; stored paths are relative to m_relativeDirPath (if specified).
	//
	struct CEnumerator : public IEnumerator, private utl::noncopyable
	{
		CEnumerator( IEnumerator* pChainEnum = NULL ) : m_pChainEnum( pChainEnum ), m_maxFiles( utl::npos ) {}

		const fs::CPath& GetRelativeDirPath( void ) const { return m_relativeDirPath; }
		void SetRelativeDirPath( const fs::CPath& relativeDirPath ) { ASSERT( IsEmpty() ); m_relativeDirPath = relativeDirPath; }

		void SetMaxFiles( size_t maxFiles ) { ASSERT( IsEmpty() ); m_maxFiles = maxFiles; }
		void SetIgnorePathMatches( const std::vector< fs::CPath >& ignorePaths );

		bool IsEmpty( void ) const { return m_filePaths.empty() && m_subDirPaths.empty(); }
		void Clear( void );
		size_t UniquifyAll( void );					// post-search

		// IEnumerator interface
		virtual void AddFoundFile( const TCHAR* pFilePath );
		virtual bool AddFoundSubDir( const TCHAR* pSubDirPath );
		virtual bool MustStop( void ) const;
	protected:
		IEnumerator* m_pChainEnum;					// allows chaining for progress reporting
		fs::CPath m_relativeDirPath;				// to remove if it's common prefix
		size_t m_maxFiles;
	private:
		std::auto_ptr< CPathMatches > m_pIgnorePathMatches;
	public:
		std::vector< fs::CPath > m_filePaths;
		std::vector< fs::CPath > m_subDirPaths;
	};


	struct CRelativeEnumerator : public CEnumerator
	{
		CRelativeEnumerator( const fs::CPath& relativeDirPath ) : CEnumerator( NULL ) { SetRelativeDirPath( relativeDirPath ); }
	};


	struct CFirstFileEnumerator : public CEnumerator
	{
		CFirstFileEnumerator( void ) : CEnumerator() { SetMaxFiles( 1 ); }

		fs::CPath GetFoundPath( void ) const { return !m_filePaths.empty() ? m_filePaths.front() : fs::CPath(); }
	};
}


namespace fs
{
	class CPathMatches : private utl::noncopyable
	{
	public:
		CPathMatches( const std::vector< fs::CPath >& paths );
		~CPathMatches();

		bool IsEmpty( void ) const { return m_dirPaths.empty() && m_filePaths.empty() && m_wildSpecs.empty(); }

		bool IsDirMatch( const fs::CPath& dirPath ) const;
		bool IsFileMatch( const fs::CPath& filePath ) const;
	private:
		bool IsWildcardMatch( const fs::CPath& anyPath ) const;
	private:
		std::vector< fs::CPath > m_dirPaths;			// deep matching (including subdirectories)
		std::vector< fs::CPath > m_filePaths;
		std::vector< std::tstring > m_wildSpecs;
	};
}


#endif // FileEnumerator_h
