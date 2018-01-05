
#include "stdafx.h"
#include "PathMaker.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPathMaker::CPathMaker( void )
	: m_pRenamePairs( new fs::PathPairMap )
	, m_mapOwnership( true )
{
	ASSERT_PTR( m_pRenamePairs );
}

CPathMaker::CPathMaker( fs::PathPairMap* pRenamePairs )
	: m_pRenamePairs( pRenamePairs )
	, m_mapOwnership( false )
{
	ASSERT_PTR( m_pRenamePairs );
}

CPathMaker::~CPathMaker()
{
	if ( m_mapOwnership )
		delete m_pRenamePairs;
}

bool CPathMaker::MakeDestRelative( const std::tstring& prefixDirPath )
{
	// for each copy SRC path to DEST path, and make relative DEST path by removing the reference dir path suffix.

	for ( fs::PathPairMap::iterator it = m_pRenamePairs->begin(); it != m_pRenamePairs->end(); ++it )
	{
		std::tstring destRelPath = it->first.GetPtr();

		if ( path::StripPrefix( destRelPath, prefixDirPath.c_str() ) )
			it->second.Set( destRelPath );
		else
			return false;			// SRC path does not share a common prefix with prefixDirPath
	}
	return true;
}

bool CPathMaker::MakeDestStripCommonPrefix( void )
{
	std::tstring commonPrefix = FindSrcCommonPrefix();
	return !commonPrefix.empty() && MakeDestRelative( commonPrefix );
}

std::tstring CPathMaker::FindSrcCommonPrefix( void ) const
{
	std::tstring commonPrefix;

	if ( !m_pRenamePairs->empty() )
	{
		fs::PathPairMap::const_iterator it = m_pRenamePairs->begin();
		commonPrefix = it->first.GetDirPath();

		for ( ++it; it != m_pRenamePairs->end(); ++it )
		{
			commonPrefix = path::FindCommonPrefix( it->first.GetPtr(), commonPrefix.c_str() );
			if ( commonPrefix.empty() )
				break;						// no common prefix, abort search
		}
	}
	return commonPrefix;
}

void CPathMaker::CopyDestPaths( const std::vector< fs::CPath >& destPaths )
{
	REQUIRE( m_pRenamePairs->size() == destPaths.size() );

	size_t pos = 0;
	for ( fs::PathPairMap::iterator it = m_pRenamePairs->begin(); it != m_pRenamePairs->end(); ++it, ++pos )
		it->second = destPaths[ pos ];
}

void CPathMaker::ResetDestPaths( void )
{
	for ( fs::PathPairMap::iterator it = m_pRenamePairs->begin(); it != m_pRenamePairs->end(); ++it )
		it->second = fs::CPath();
}
