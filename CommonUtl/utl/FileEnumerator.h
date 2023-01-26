#ifndef FileEnumerator_h
#define FileEnumerator_h
#pragma once

#include "FileSystem_fwd.h"
#include "ICounter.h"
#include "Range.h"
#include <set>
#include <unordered_set>


namespace fs
{
	// pWildSpec can be multiple: "*.*", "*.doc;*.txt"

	void EnumFiles( IEnumerator* pEnumerator, const fs::TDirPath& dirPath, const TCHAR* pWildSpec = _T("*.*") );
	fs::PatternResult SearchEnumFiles( IEnumerator* pEnumerator, const fs::TPatternPath& searchPath );

	size_t EnumFilePaths( std::vector< fs::CPath >& rFilePaths, const fs::TDirPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), fs::TEnumFlags flags = fs::TEnumFlags() );
	size_t EnumSubDirPaths( std::vector< fs::TDirPath >& rSubDirPaths, const fs::TDirPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), fs::TEnumFlags flags = fs::TEnumFlags() );

	fs::CPath FindFirstFile( const fs::TDirPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), fs::TEnumFlags flags = fs::TEnumFlags() );


	// generate a path of a unique filename using a suffix, by avoiding collisions with existing files
	fs::CPath MakeUniqueNumFilename( const fs::CPath& filePath, const TCHAR fmtNumSuffix[] = path::StdFormatNumSuffix() ) throws_( CRuntimeException );	// with numeric suffix
	fs::CPath MakeUniqueHashedFilename( const fs::CPath& filePath, const TCHAR fmtHashSuffix[] = _T("_%08X") );											// with hash suffix

	// late binding to shell::ResolveShortcut defined in UTL_UI.lib
	typedef bool (*TResolveShortcutProc)( fs::CPath& rDestPath, const TCHAR* pShortcutLnkPath, CWnd* pWnd );

	void StoreResolveShortcutProc( TResolveShortcutProc resolveShortcutProc );
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
		void AddPath( const fs::CPath& path );

		bool IsEmpty( void ) const { return m_dirPaths.empty() && m_filePaths.empty() && m_wildSpecs.empty(); }

		bool IsDirMatch( const fs::TDirPath& dirPath ) const;
		bool IsFileMatch( const fs::CPath& filePath ) const;
	private:
		bool IsWildcardMatch( const fs::CPath& anyPath ) const;
	private:
		std::vector< fs::TDirPath > m_dirPaths;			// deep matching (including subdirectories)
		std::vector< fs::CPath > m_filePaths;
		std::vector< std::tstring > m_wildSpecs;
	};


	struct CEnumOptions
	{
		CEnumOptions( fs::TEnumFlags enumFlags );

		template< typename SizeT >
		void SetFileSizeRange( const Range<SizeT>& fileSizeRange ) { m_fileSizeRange = fileSizeRange; ENSURE( m_fileSizeRange.IsNormalized() ); }
	public:
		fs::TEnumFlags m_enumFlags;
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
		CBaseEnumerator( fs::TEnumFlags enumFlags, IEnumerator* pChainEnum = NULL );
	public:
		// IEnumerator interface (partial)
		virtual const TEnumFlags& GetEnumFlags( void ) const override { return m_options.m_enumFlags; }
	protected:
		virtual void OnAddFileInfo( const fs::CFileState& fileState ) override;		// no chaining via m_pChainEnum
		virtual void AddFoundFile( const fs::CPath& filePath ) = 0 override;		// has implementation
		virtual bool AddFoundSubDir( const fs::TDirPath& subDirPath ) override;
		virtual bool CanIncludeNode( const fs::CFileState& nodeState ) const override;
		virtual bool CanRecurse( void ) const override;
		virtual bool MustStop( void ) const override;
		virtual utl::ICounter* GetDepthCounter( void ) override { return &m_depthCounter; }

		bool PassFileFilter( const fs::CFileState& fileState ) const;

		bool RegisterUnique( const fs::CPath& nodePath ) const;
		bool IgnorePath( const fs::CPath& ignoredPath ) const;			// returns false for convenience
	public:
		bool IsEmpty( void ) const { return m_subDirPaths.empty() && 0 == GetFileCount(); }

		// overridables
		virtual size_t GetFileCount( void ) const = 0;
		virtual void Clear( void ) = 0 { m_subDirPaths.clear(); m_uniquePaths.clear(); m_ignoredPaths.clear(); }

		fs::TEnumFlags& RefFlags( void ) { REQUIRE( IsEmpty() ); return m_options.m_enumFlags; }

		const fs::CEnumOptions& GetOptions( void ) const { return m_options; }
		fs::CEnumOptions& RefOptions( void ) { REQUIRE( IsEmpty() ); return m_options; }

		const fs::TDirPath& GetRelativeDirPath( void ) const { return m_relativeDirPath; }
		void SetRelativeDirPath( const fs::TDirPath& relativeDirPath ) { ASSERT( IsEmpty() ); m_relativeDirPath = relativeDirPath; }

		void SetIgnorePathMatches( const std::vector< fs::CPath >& ignorePaths );

		const std::set< fs::CPath >& GetIgnoredPaths( void ) const { return m_ignoredPaths; }
	protected:
		IEnumerator* m_pChainEnum;					// allows chaining for progress reporting
		CEnumOptions m_options;
		fs::TDirPath m_relativeDirPath;				// to remove if it's common prefix

		utl::CCounter m_depthCounter;				// counts recursion depth
	private:
		mutable std::unordered_set< fs::CPath > m_uniquePaths;	// files + sub-directories found
		mutable std::set< fs::CPath > m_ignoredPaths;			// files + sub-dirs ignored or filtered-out
	public:
		std::vector< fs::TDirPath > m_subDirPaths;	// found sub-directories
	};
}


namespace fs
{
	// files and sub-dirs in existing file-system order; stored paths are relative to m_relativeDirPath (if specified).
	//
	struct CPathEnumerator : public CBaseEnumerator
	{
		CPathEnumerator( fs::TEnumFlags enumFlags = fs::TEnumFlags(), IEnumerator* pChainEnum = NULL ) : CBaseEnumerator( enumFlags, pChainEnum ) {}

		// base overrides
		virtual size_t GetFileCount( void ) const override { return m_filePaths.size(); }
		virtual void Clear( void ) override;
	protected:
		// IEnumerator interface
		virtual void AddFoundFile( const fs::CPath& filePath ) override;
	public:
		std::vector< fs::CPath > m_filePaths;
	};


	struct CRelativePathEnumerator : public CPathEnumerator
	{
		CRelativePathEnumerator( const fs::TDirPath& relativeDirPath, fs::TEnumFlags enumFlags = fs::TEnumFlags(), IEnumerator* pChainEnum = NULL )
			: CPathEnumerator( enumFlags, pChainEnum )
		{
			SetRelativeDirPath( relativeDirPath );
		}
	};


	struct CFirstFileEnumerator : public CPathEnumerator
	{
		CFirstFileEnumerator( fs::TEnumFlags enumFlags = fs::TEnumFlags() ) : CPathEnumerator( enumFlags ) { RefOptions().m_maxFiles = 1; }

		fs::CPath GetFoundPath( void ) const { return !m_filePaths.empty() ? m_filePaths.front() : fs::CPath(); }
	};
}


#endif // FileEnumerator_h
