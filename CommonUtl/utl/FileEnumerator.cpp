
#include "stdafx.h"
#include "FileEnumerator.h"
#include "RuntimeException.h"
#include "ContainerUtilities.h"
#include "StringUtilities.h"
#include <hash_set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
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
					subDirPaths.push_back( fs::CPath( foundPath ) );
			}
			else
			{
				if ( path::MatchWildcard( foundPath.c_str(), pWildSpec ) )
					pEnumerator->AddFile( finder );
			}
		}

		fs::SortPaths( subDirPaths );		// natural path order

		// progress reporting: ensure the sub-directory (stage) is always displayed first, then the files (items) under it
		for ( std::vector< fs::CPath >::const_iterator itSubDirPath = subDirPaths.begin(); itSubDirPath != subDirPaths.end(); ++itSubDirPath )
			if ( pEnumerator->AddFoundSubDir( itSubDirPath->GetPtr() ) )		// sub-directory is not ignored?
				if ( Deep == depth && !pEnumerator->MustStop() )
					EnumFiles( pEnumerator, *itSubDirPath, pWildSpec, Deep );
	}

	size_t EnumFilePaths( std::vector< fs::CPath >& rFilePaths, const fs::CPath& dirPath, const TCHAR* pWildSpec /*= _T("*")*/, RecursionDepth depth /*= Shallow*/ )
	{
		CEnumerator found;
		EnumFiles( &found, dirPath, pWildSpec, depth );

		return fs::JoinUniquePaths( rFilePaths, found.m_filePaths );		// added count
	}

	size_t EnumSubDirPaths( std::vector< fs::CPath >& rSubDirPaths, const fs::CPath& dirPath, const TCHAR* pWildSpec /*= _T("*.*")*/, RecursionDepth depth /*= Shallow*/ )
	{
		CEnumerator found;
		EnumFiles( &found, dirPath, pWildSpec, depth );

		return fs::JoinUniquePaths( rSubDirPaths, found.m_subDirPaths );	// added count
	}


	fs::CPath FindFirstFile( const fs::CPath& dirPath, const TCHAR* pWildSpec /*= _T("*.*")*/, RecursionDepth depth /*= Shallow*/ )
	{
		CFirstFileEnumerator singleEnumer;
		EnumFiles( &singleEnumer, dirPath, pWildSpec, depth );
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

		const UINT hashKey = static_cast< UINT >( path::GetHashValue( filePath.GetPtr() ) );		// hash key is unique for the whole path
		parts.m_fname += str::Format( fmtHashSuffix, hashKey );		// e.g. "fname_hexHashKey"
		fs::CPath uniqueFilePath = parts.MakePath();

		ENSURE( !uniqueFilePath.FileExist() );		// hashed path is supposed to be be unique

		return uniqueFilePath;
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

	void CEnumerator::SetIgnorePathMatches( const std::vector< fs::CPath >& ignorePaths )
	{
		m_pIgnorePathMatches.reset( !ignorePaths.empty() ? new CPathMatches( ignorePaths ) : NULL );

		if ( m_pIgnorePathMatches.get() != NULL && m_pIgnorePathMatches->IsEmpty() )
			m_pIgnorePathMatches.reset();
	}

	void CEnumerator::AddFoundFile( const TCHAR* pFilePath )
	{
		fs::CPath filePath( pFilePath );

		if ( m_pIgnorePathMatches.get() != NULL )
			if ( m_pIgnorePathMatches->IsFileMatch( filePath ) )
			{
				TRACE( _T(" CEnumerator::AddFoundFile(): Ignoring file: %s\n"), filePath.GetPtr() );
				return;
			}

		if ( !m_relativeDirPath.IsEmpty() )
			filePath = fs::StripDirPrefix( filePath, m_relativeDirPath );

		if ( !filePath.IsEmpty() )
			m_filePaths.push_back( filePath );

		if ( m_pChainEnum != NULL )
			m_pChainEnum->AddFoundFile( pFilePath );
	}

	bool CEnumerator::AddFoundSubDir( const TCHAR* pSubDirPath )
	{
		fs::CPath subDirPath( pSubDirPath );

		if ( m_pIgnorePathMatches.get() != NULL )
			if ( m_pIgnorePathMatches->IsDirMatch( subDirPath ) )
			{
				TRACE( _T(" CEnumerator::AddFoundFile(): Ignoring sub-directory: %s\n"), subDirPath.GetPtr() );
				return false;
			}

		if ( !m_relativeDirPath.IsEmpty() )
			subDirPath = fs::StripDirPrefix( subDirPath, m_relativeDirPath );

		if ( !subDirPath.IsEmpty() )
			m_subDirPaths.push_back( subDirPath );

		if ( m_pChainEnum != NULL )
			m_pChainEnum->AddFoundSubDir( pSubDirPath );

		return true;
	}

	bool CEnumerator::MustStop( void ) const
	{
		return m_filePaths.size() >= m_maxFiles;
	}

	size_t CEnumerator::UniquifyAll( void )
	{
		stdext::hash_set< fs::CPath > uniquePaths;

		return fs::UniquifyPaths( m_subDirPaths, uniquePaths ) + fs::UniquifyPaths( m_filePaths, uniquePaths );
	}

} //namespace fs


namespace fs
{
	// CPathMatches implementation

	CPathMatches::CPathMatches( const std::vector< fs::CPath >& paths )
	{
		for ( std::vector< fs::CPath >::const_iterator itPath = paths.begin(); itPath != paths.end(); ++itPath )
			if ( path::ContainsWildcards( ( *itPath ).GetPtr() ) )
				m_wildSpecs.push_back( ( *itPath ).Get() );
			else if ( fs::IsValidDirectory( ( *itPath ).GetPtr() ) )
				m_dirPaths.push_back( *itPath );
			else if ( fs::IsValidFile( ( *itPath ).GetPtr() ) )
				m_filePaths.push_back( *itPath );
	}

	CPathMatches::~CPathMatches()
	{
	}

	bool CPathMatches::IsDirMatch( const fs::CPath& dirPath ) const
	{
		for ( std::vector< fs::CPath >::const_iterator itDirPath = m_dirPaths.begin(); itDirPath != m_dirPaths.end(); ++itDirPath )
			if ( path::HasPrefix( dirPath.GetPtr(), ( *itDirPath ).GetPtr() ) )
				return true;

		return IsWildcardMatch( dirPath );
	}

	bool CPathMatches::IsFileMatch( const fs::CPath& filePath ) const
	{
		for ( std::vector< fs::CPath >::const_iterator itFilePath = m_filePaths.begin(); itFilePath != m_filePaths.end(); ++itFilePath )
			if ( filePath == *itFilePath )
				return true;

		return IsWildcardMatch( filePath );
	}

	bool CPathMatches::IsWildcardMatch( const fs::CPath& anyPath ) const
	{
		for ( std::vector< std::tstring >::const_iterator itWildSpec = m_wildSpecs.begin(); itWildSpec != m_wildSpecs.end(); ++itWildSpec )
			if ( path::MatchWildcard( anyPath.GetPtr(), ( *itWildSpec ).c_str() ) )
				return true;

		return false;
	}
}


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
					fs::EnumSubDirPaths( existingPaths, parentDirPath, wildSpec.c_str(), Shallow );		// sibling subdirs of the parent-dir-path
				else
					fs::EnumFilePaths( existingPaths, parentDirPath, wildSpec.c_str(), Shallow );		// sibling file leafs

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

	} //namespace impl
} //namespace fs
