
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/TempFilePairPool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	// CPathPairPool implementation

	CPathPairPool::CPathPairPool( const TCHAR* pSourceFilenames, bool fullDestPaths /*= false*/ )
		: m_fullDestPaths( fullDestPaths )
	{
		std::vector< std::tstring > srcFilenames;
		str::Split( srcFilenames, pSourceFilenames, ut::CTempFilePool::m_sep );

		m_pathPairs.StoreSrcFromPaths( srcFilenames );

		ENSURE( m_pathPairs.GetPairs().size() == srcFilenames.size() );
	}

	std::tstring CPathPairPool::JoinDest( void )
	{
		std::vector< std::tstring > destFilenames; destFilenames.reserve( m_pathPairs.GetPairs().size() );

		for ( CPathRenamePairs::const_iterator itPair = m_pathPairs.Begin(); itPair != m_pathPairs.End(); ++itPair )
			destFilenames.push_back( m_fullDestPaths ? itPair->second.Get() : fs::CPathParts( itPair->second.Get() ).GetFilename() );

		return str::Join( destFilenames, ut::CTempFilePool::m_sep );
	}

	void CPathPairPool::CopySrc( void )
	{
		m_pathPairs.CopySrcToDestPaths();
	}


	// CTempFilePairPool implementation

	CTempFilePairPool::CTempFilePairPool( const TCHAR* pSourceFilenames )
		: CTempFilePool( pSourceFilenames )
	{
		if ( IsValidPool() )
			m_pathPairs.StoreSrcFromPaths( GetFilePaths() );
	}
}


#endif //USE_UT
