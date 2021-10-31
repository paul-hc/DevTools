#ifndef FileEnumerator_h
#define FileEnumerator_h
#pragma once

#include "FileSystem_fwd.h"
#include "ICounter.h"
#include "Range.h"


namespace fs
{
	// pWildSpec can be multiple: "*.*", "*.doc;*.txt"

	void EnumFiles( IEnumerator* pEnumerator, const fs::TDirPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), fs::TEnumFlags flags = fs::TEnumFlags() );
	fs::PatternResult SearchEnumFiles( IEnumerator* pEnumerator, const fs::TPatternPath& searchPath, fs::TEnumFlags flags = fs::TEnumFlags() );

	size_t EnumFilePaths( std::vector< fs::CPath >& rFilePaths, const fs::CPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), fs::TEnumFlags flags = fs::TEnumFlags() );
	size_t EnumSubDirPaths( std::vector< fs::CPath >& rSubDirPaths, const fs::CPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), fs::TEnumFlags flags = fs::TEnumFlags() );

	fs::CPath FindFirstFile( const fs::CPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), fs::TEnumFlags flags = fs::TEnumFlags() );


	// generate a path of a unique filename using a suffix, by avoiding collisions with existing files
	fs::CPath MakeUniqueNumFilename( const fs::CPath& filePath, const TCHAR fmtNumSuffix[] = path::StdFormatNumSuffix() ) throws_( CRuntimeException );	// with numeric suffix
	fs::CPath MakeUniqueHashedFilename( const fs::CPath& filePath, const TCHAR fmtHashSuffix[] = _T("_%08X") );											// with hash suffix

	// late binding to shell::ResolveShortcut defined in UTL_UI.lib
	typedef bool (*ResolveShortcutProc)( fs::CPath& rDestPath, const TCHAR* pShortcutLnkPath, CWnd* pWnd );

	void StoreResolveShortcutProc( ResolveShortcutProc resolveShortcutProc );
}


namespace fs
{
	class CPathMatchLookup
	{
	public:
		CPathMatchLookup( void ) {}
		CPathMatchLookup( const std::vector< fs::CPath >& paths ) { Reset( paths ); }

		void Clear( void );
		void Reset( const std::vector< fs::CPath >& paths );

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


	struct CEnumOptions
	{
		CEnumOptions( void );

		template< typename SizeT >
		void SetFileSizeRange( const Range<SizeT>& fileSizeRange ) { m_fileSizeRange = fileSizeRange; ENSURE( m_fileSizeRange.IsNormalized() ); }
	public:
		bool m_ignoreFiles;
		bool m_ignoreHiddenNodes;
		size_t m_maxFiles;
		size_t m_maxDepthLevel;
		Range<UINT64> m_fileSizeRange;
		fs::CPathMatchLookup m_ignorePathMatches;

		static const Range<UINT64> s_fullFileSizesRange;
	};


	// Base for concrete file leafs enumerator classes, provides the heavy duty functionality of filtering, etc.
	//
	abstract class CBaseEnumerator : public IEnumerator, private utl::noncopyable
	{
	protected:
		CBaseEnumerator( IEnumerator* pChainEnum = NULL );

		// IEnumerator interface (partial)
		virtual void OnAddFileInfo( const CFileFind& foundFile );
		virtual void AddFoundFile( const TCHAR* pFilePath ) = 0;		// has implementation
		virtual bool AddFoundSubDir( const TCHAR* pSubDirPath );
		virtual bool IncludeNode( const CFileFind& foundNode );
		virtual bool CanRecurse( void ) const;
		virtual bool MustStop( void ) const;
		virtual utl::ICounter* GetDepthCounter( void ) { return &m_depthCounter; }

		bool PassFileFilter( UINT64 fileSize ) const;
	public:
		bool IsEmpty( void ) const { return m_subDirPaths.empty() && 0 == GetFileCount(); }

		// overridables
		virtual size_t GetFileCount( void ) const = 0;
		virtual void Clear( void ) = 0 { m_subDirPaths.clear(); }

		const fs::CEnumOptions& GetOptions( void ) const { return m_options; }
		fs::CEnumOptions& RefOptions( void ) { REQUIRE( IsEmpty() ); return m_options; }

		const fs::CPath& GetRelativeDirPath( void ) const { return m_relativeDirPath; }
		void SetRelativeDirPath( const fs::CPath& relativeDirPath ) { ASSERT( IsEmpty() ); m_relativeDirPath = relativeDirPath; }

		void SetIgnorePathMatches( const std::vector< fs::CPath >& ignorePaths );
	protected:
		IEnumerator* m_pChainEnum;					// allows chaining for progress reporting
		CEnumOptions m_options;
		fs::CPath m_relativeDirPath;				// to remove if it's common prefix

		utl::CCounter m_depthCounter;				// counts recursion depth
	public:
		std::vector< fs::CPath > m_subDirPaths;		// found sub-directories
	};
}


namespace fs
{
	// files and sub-dirs in existing file-system order; stored paths are relative to m_relativeDirPath (if specified).
	//
	struct CPathEnumerator : public CBaseEnumerator
	{
		CPathEnumerator( IEnumerator* pChainEnum = NULL ) : CBaseEnumerator( pChainEnum ) {}

		// base overrides
		virtual size_t GetFileCount( void ) const { return m_filePaths.size(); }
		virtual void Clear( void );

		size_t UniquifyAll( void );					// post-search
	protected:
		// IEnumerator interface
		virtual void AddFoundFile( const TCHAR* pFilePath );
	public:
		std::vector< fs::CPath > m_filePaths;
	};


	struct CRelativePathEnumerator : public CPathEnumerator
	{
		CRelativePathEnumerator( const fs::CPath& relativeDirPath ) : CPathEnumerator( NULL ) { SetRelativeDirPath( relativeDirPath ); }
	};


	struct CFirstFileEnumerator : public CPathEnumerator
	{
		CFirstFileEnumerator( void ) : CPathEnumerator() { RefOptions().m_maxFiles = 1; }

		fs::CPath GetFoundPath( void ) const { return !m_filePaths.empty() ? m_filePaths.front() : fs::CPath(); }
	};
}


#endif // FileEnumerator_h
