
#include "stdafx.h"
#include "FileEnumerator.h"
#include "FileState.h"
#include "RuntimeException.h"
#include "Algorithms.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "Path.hxx"		// ?? needed for VC12


namespace fs
{
	static TResolveShortcutProc s_resolveShortcutProc = NULL;

	void StoreResolveShortcutProc( TResolveShortcutProc resolveShortcutProc )
	{
		s_resolveShortcutProc = resolveShortcutProc;
	}

	bool IsDots( const fs::CPath& filePath )
	{
		const TCHAR* pFilename = filePath.GetFilenamePtr();

		return
			'.' == pFilename[0] && ( 0 == pFilename[1] || ( '.' == pFilename[1] && 0 == pFilename[2] ) );
	}
}


namespace fs
{
	void EnumFiles( IEnumerator* pEnumerator, const fs::TDirPath& dirPath, const TCHAR* pWildSpec /*= _T("*.*")*/ )
	{
		ASSERT_PTR( pEnumerator );

		utl::CScopedIncrement depthLevel( pEnumerator->GetDepthCounter() );

		if ( str::IsEmpty( pWildSpec ) )
			pWildSpec = _T("*");

		fs::TPatternPath dirPathFilter = dirPath / pWildSpec;

		if ( pEnumerator->HasEnumFlag( fs::EF_Recurse ) || path::IsMultipleWildcard( pWildSpec ) )
			if ( !fs::IsValidFile( dirPathFilter.GetPtr() ) )		// not a single file search path?
				dirPathFilter = dirPath / _T("*");					// need to relax filter to all so that it covers sub-directories

		std::vector< fs::CPath > subDirPaths;

		CFileFind finder;
		for ( BOOL found = finder.FindFile( dirPathFilter.GetPtr() ); found && !pEnumerator->MustStop(); )
		{
			found = finder.FindNextFile();
			fs::CFileState nodeState( finder );

			if ( pEnumerator->HasEnumFlag( fs::EF_ResolveShellLinks ) )
				if ( s_resolveShortcutProc != NULL )				// links with UTL_UI.lib?
					if ( fs::IsValidShellLink( nodeState.m_fullPath.GetPtr() ) )
					{
						fs::CPath linkTargetPath;

						if ( s_resolveShortcutProc( linkTargetPath, nodeState.m_fullPath.GetPtr(), NULL ) )
							nodeState = fs::CFileState::ReadFromFile( linkTargetPath );		// resolve the link to taget path
					}

			if ( nodeState.IsDirectory() )
			{
				if ( !fs::IsDots( nodeState.m_fullPath ) )			// skip "." and ".." dir entries
					if ( pEnumerator->CanIncludeNode( nodeState ) )	// pass found sub-dir filter?
						subDirPaths.push_back( nodeState.m_fullPath );
			}
			else
			{
				if ( pEnumerator->CanIncludeNode( nodeState ) )		// pass found file filter?
					if ( path::MatchWildcard( nodeState.m_fullPath.GetPtr(), pWildSpec ) )
						pEnumerator->OnAddFileInfo( nodeState );
			}
		}

		if ( !pEnumerator->HasEnumFlag( fs::EF_NoSortSubDirs ) )
			fs::SortPaths( subDirPaths );		// natural path order

		const bool canRecurse = pEnumerator->CanRecurse();

		// progress reporting: ensure the sub-directory (stage) is always displayed first, then the files (items) under it
		for ( std::vector< fs::TDirPath >::const_iterator itSubDirPath = subDirPaths.begin(); itSubDirPath != subDirPaths.end(); ++itSubDirPath )
			if ( pEnumerator->AddFoundSubDir( *itSubDirPath ) )		// sub-directory is not ignored?
				if ( canRecurse && !pEnumerator->MustStop() )
					EnumFiles( pEnumerator, *itSubDirPath, pWildSpec );
	}

	fs::PatternResult SearchEnumFiles( IEnumerator* pEnumerator, const fs::TPatternPath& searchPath )
	{
		fs::TDirPath dirPath;
		std::tstring wildSpec = _T("*");
		fs::PatternResult result;

		if ( fs::IsValidFile( searchPath.GetPtr() ) )
		{
			dirPath = searchPath.GetParentPath();
			wildSpec = searchPath.GetFilename();
			result = fs::ValidFile;
		}
		else
			switch ( result = fs::SplitPatternPath( &dirPath, &wildSpec, searchPath ) )
			{
				case fs::ValidDirectory:
					break;
				case fs::ValidFile:
					ASSERT( false );		// should've been handled by the if statement
				case fs::InvalidPattern:
					return result;
			}

		fs::EnumFiles( pEnumerator, dirPath, wildSpec.c_str() );
		return result;
	}


	size_t EnumFilePaths( std::vector< fs::CPath >& rFilePaths, const fs::TDirPath& dirPath, const TCHAR* pWildSpec /*= _T("*")*/, fs::TEnumFlags flags /*= fs::TEnumFlags()*/ )
	{
		CPathEnumerator found( flags );
		EnumFiles( &found, dirPath, pWildSpec );

		return path::JoinUniquePaths( rFilePaths, found.m_filePaths );		// added count
	}

	size_t EnumSubDirPaths( std::vector< fs::TDirPath >& rSubDirPaths, const fs::TDirPath& dirPath, const TCHAR* pWildSpec /*= _T("*.*")*/, fs::TEnumFlags flags /*= fs::TEnumFlags()*/ )
	{
		CPathEnumerator found( flags );
		EnumFiles( &found, dirPath, pWildSpec );

		return path::JoinUniquePaths( rSubDirPaths, found.m_subDirPaths );	// added count
	}


	fs::CPath FindFirstFile( const fs::TDirPath& dirPath, const TCHAR* pWildSpec /*= _T("*.*")*/, fs::TEnumFlags flags /*= fs::TEnumFlags()*/ )
	{
		CFirstFileEnumerator singleEnumer( flags );
		EnumFiles( &singleEnumer, dirPath, pWildSpec );
		return singleEnumer.GetFoundPath();
	}


	namespace impl { UINT QueryExistingSequenceCount( const fs::CPathParts& filePathParts ); }


	fs::CPath MakeUniqueNumFilename( const fs::CPath& filePath, const TCHAR fmtNumSuffix[] /*= path::StdFormatNumSuffix()*/ ) throws_( CRuntimeException )
	{
		ASSERT( !filePath.IsEmpty() );
		if ( !filePath.FileExist() )
			return filePath;						// no file-path collision

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

		const UINT hashKey = static_cast<UINT>( path::GetHashValue( filePath.Get() ) );		// hash key is unique for the whole path
		parts.m_fname += str::Format( fmtHashSuffix, hashKey );		// e.g. "fname_hexHashKey"
		fs::CPath uniqueFilePath = parts.MakePath();

		ENSURE( !uniqueFilePath.FileExist() );		// hashed path is supposed to be be unique

		return uniqueFilePath;
	}
}


namespace fs
{
	// CPathMatchLookup implementation

	void CPathMatchLookup::Clear( void )
	{
		m_dirPaths.clear();
		m_filePaths.clear();
		m_wildSpecs.clear();
	}

	void CPathMatchLookup::Reset( const std::vector< fs::CPath >& paths )
	{
		Clear();

		for ( std::vector< fs::CPath >::const_iterator itPath = paths.begin(); itPath != paths.end(); ++itPath )
			AddPath( *itPath );
	}

	void CPathMatchLookup::AddPath( const fs::CPath& path )
	{
		if ( path::ContainsWildcards( path.GetPtr() ) )
			m_wildSpecs.push_back( path.Get() );
		else if ( fs::IsValidDirectory( path.GetPtr() ) )
			m_dirPaths.push_back( path );
		else if ( fs::IsValidFile( path.GetPtr() ) )
			m_filePaths.push_back( path );
	}

	bool CPathMatchLookup::IsDirMatch( const fs::TDirPath& dirPath ) const
	{
		if ( !m_dirPaths.empty() )		// speed things up if empty - skip iterators creation
			for ( std::vector< fs::TDirPath >::const_iterator itDirPath = m_dirPaths.begin(); itDirPath != m_dirPaths.end(); ++itDirPath )
				if ( path::HasPrefix( dirPath.GetPtr(), itDirPath->GetPtr() ) )
					return true;

		return IsWildcardMatch( dirPath );
	}

	bool CPathMatchLookup::IsFileMatch( const fs::CPath& filePath ) const
	{
		if ( !m_filePaths.empty() )		// speed things up if empty - skip iterators creation
			for ( std::vector< fs::CPath >::const_iterator itFilePath = m_filePaths.begin(); itFilePath != m_filePaths.end(); ++itFilePath )
				if ( filePath == *itFilePath )
					return true;

		return IsWildcardMatch( filePath );
	}

	bool CPathMatchLookup::IsWildcardMatch( const fs::CPath& anyPath ) const
	{
		if ( !m_wildSpecs.empty() )		// speed things up if empty - skip iterators creation
			for ( std::vector< std::tstring >::const_iterator itWildSpec = m_wildSpecs.begin(); itWildSpec != m_wildSpecs.end(); ++itWildSpec )
				if ( path::MatchWildcard( anyPath.GetPtr(), itWildSpec->c_str() ) )
					return true;

		return false;
	}


	// CEnumOptions implementation

	const Range<UINT64> CEnumOptions::s_fullFileSizesRange( 0, std::numeric_limits<UINT64>::max() );

	CEnumOptions::CEnumOptions( fs::TEnumFlags enumFlags )
		: m_enumFlags( enumFlags )
		, m_maxFiles( utl::npos )
		, m_maxDepthLevel( utl::npos )
		, m_fileSizeRange( s_fullFileSizesRange )
	{
	}
}


namespace fs
{
	// IEnumerator implementation

	void IEnumerator::OnAddFileInfo( const fs::CFileState& fileState )
	{
		AddFoundFile( fileState.m_fullPath );
	}


	// IEnumeratorImpl implementation

	bool IEnumeratorImpl::CanIncludeNode( const fs::CFileState& nodeState ) const
	{
		if ( HasEnumFlag( fs::EF_IgnoreHiddenNodes ) && nodeState.IsHidden() )
			return false;

		if ( HasEnumFlag( fs::EF_IgnoreFiles ) && nodeState.IsRegularFile() )
			return false;

		return true;
	}


	// CBaseEnumerator implementation

	CBaseEnumerator::CBaseEnumerator( fs::TEnumFlags enumFlags, IEnumerator* pChainEnum /*= NULL*/ )
		: m_options( enumFlags )
		, m_pChainEnum( pChainEnum )
		, m_depthCounter( utl::npos )
	{
	}

	void CBaseEnumerator::SetIgnorePathMatches( const std::vector< fs::CPath >& ignorePaths )
	{
		m_options.m_ignorePathMatches.Reset( ignorePaths );
	}

	bool CBaseEnumerator::RegisterUnique( const fs::CPath& nodePath ) const
	{
		return m_uniquePaths.insert( nodePath ).second;			// is path unique?
	}

	bool CBaseEnumerator::IgnorePath( const fs::CPath& ignoredPath ) const
	{
		if ( m_ignoredPaths.insert( ignoredPath ).second )
		{
		#ifdef _DEBUG
			static const TCHAR* s_typeTags[] = { _T("file"), _T("sub-directory"), _T("non-existent path") };
			enum { File, SubDir, NonExistent } fileType = NonExistent;

			if ( fs::IsValidFile( ignoredPath.GetPtr() ) )
				fileType = File;
			else if ( fs::IsValidDirectory( ignoredPath.GetPtr() ) )
				fileType = SubDir;

			TRACE( _T(" - CBaseEnumerator::IgnorePath(): ignoring %s #%d: %s\n"), s_typeTags[ fileType ], m_ignoredPaths.size(), ignoredPath.GetPtr() );
		#endif //_DEBUG
		}

		return false;			// returns false for convenience
	}


	void CBaseEnumerator::OnAddFileInfo( const fs::CFileState& fileState ) override
	{	// note: we should not chain this method to m_pChainEnum!
		REQUIRE( !HasEnumFlag( fs::EF_IgnoreFiles ) );	// should've been filtered by now

		__super::OnAddFileInfo( fileState );
	}

	void CBaseEnumerator::AddFoundFile( const fs::CPath& filePath ) override
	{
		REQUIRE( !HasEnumFlag( fs::EF_IgnoreFiles ) );	// should've been filtered by now

		if ( m_pChainEnum != NULL )
			m_pChainEnum->AddFoundFile( filePath );
	}

	bool CBaseEnumerator::AddFoundSubDir( const fs::TDirPath& subDirPath ) override
	{
		fs::CPath _subDirPath = subDirPath;

		if ( !m_relativeDirPath.IsEmpty() )
			_subDirPath = path::StripDirPrefix( _subDirPath, m_relativeDirPath );

		if ( !_subDirPath.IsEmpty() )
			m_subDirPaths.push_back( _subDirPath );

		if ( m_pChainEnum != NULL )
			m_pChainEnum->AddFoundSubDir( subDirPath );

		return true;
	}

	bool CBaseEnumerator::CanIncludeNode( const fs::CFileState& nodeState ) const override
	{
		if ( nodeState.IsDirectory() )			// regular file?
		{
			if ( HasEnumFlag( fs::EF_IgnoreHiddenNodes ) && nodeState.IsHidden() )
				return IgnorePath( nodeState.m_fullPath );

			if ( m_options.m_ignorePathMatches.IsDirMatch( nodeState.m_fullPath ) )
				return IgnorePath( nodeState.m_fullPath );
		}
		else if ( !PassFileFilter( nodeState ) )
			return false;

		return RegisterUnique( nodeState.m_fullPath );		// true if unique file added
	}

	bool CBaseEnumerator::PassFileFilter( const fs::CFileState& fileState ) const
	{
		if ( HasEnumFlag( fs::EF_IgnoreFiles ) )
			return false;

		if ( HasEnumFlag( fs::EF_IgnoreHiddenNodes ) && fileState.IsHidden() )
			return IgnorePath( fileState.m_fullPath );

		if ( !m_options.m_fileSizeRange.Contains( fileState.m_fileSize ) )
			return IgnorePath( fileState.m_fullPath );

		if ( m_options.m_ignorePathMatches.IsFileMatch( fileState.m_fullPath ) )
			return IgnorePath( fileState.m_fullPath );

		return !MustStop();
	}

	bool CBaseEnumerator::MustStop( void ) const override
	{
		return GetFileCount() >= m_options.m_maxFiles;
	}

	bool CBaseEnumerator::CanRecurse( void ) const override
	{
		return HasEnumFlag( fs::EF_Recurse ) && m_depthCounter.GetCount() < m_options.m_maxDepthLevel;
	}
}

namespace fs
{
	// CPathEnumerator implementation

	void CPathEnumerator::Clear( void ) override
	{
		__super::Clear();
		m_filePaths.clear();
	}

	void CPathEnumerator::AddFoundFile( const fs::CPath& filePath ) override
	{
		fs::CPath _filePath = filePath;

		if ( !m_relativeDirPath.IsEmpty() )
			_filePath = path::StripDirPrefix( _filePath, m_relativeDirPath );

		if ( !_filePath.IsEmpty() )
			m_filePaths.push_back( _filePath );

		__super::AddFoundFile( _filePath );
	}

} //namespace fs


namespace fs
{
	namespace impl
	{
		UINT QueryExistingSequenceCount( const fs::CPathParts& filePathParts )
		{
			const fs::CPath filePath = filePathParts.MakePath();
			const fs::CPath parentDirPath = filePathParts.GetDirPath();

			std::vector< fs::CPath > existingPaths;
			UINT seqCount = 0;

			if ( fs::IsValidDirectory( parentDirPath.GetPtr() ) )
			{
				std::tstring wildSpec = filePathParts.m_fname + _T("*") + filePathParts.m_ext;

				if ( fs::IsValidDirectory( filePath.GetPtr() ) )
					fs::EnumSubDirPaths( existingPaths, parentDirPath, wildSpec.c_str() );		// sibling subdirs of the parent-dir-path
				else
					fs::EnumFilePaths( existingPaths, parentDirPath, wildSpec.c_str() );		// sibling file leafs

				seqCount = static_cast<UINT>( existingPaths.size() );
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

	} //namespace impl
} //namespace fs
