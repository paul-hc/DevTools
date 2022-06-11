
#include "stdafx.h"
#include "PathRenamePairs.h"
#include "Algorithms.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void CPathRenamePairs::Clear( void )
{
	m_pairs.clear();
	m_pathToIndexMap.clear();
}

void CPathRenamePairs::AddPair( const fs::CPath& srcPath, const fs::CPath& destPath )
{
	REQUIRE( !ContainsSrc( srcPath ) );

	size_t index = m_pairs.size();

	m_pairs.push_back( fs::TPathPair( srcPath, destPath ) );
	m_pathToIndexMap[ srcPath ] = index;

	ENSURE( IsConsistent() );
}

fs::CPath& CPathRenamePairs::operator[]( const fs::CPath& srcPath )
{
	fs::CPath* pDestPath = FindDestPath( srcPath );

	if ( NULL == pDestPath )
	{
		AddSrc( srcPath );
		pDestPath = &m_pairs.back().second;
	}

	ASSERT_PTR( pDestPath );
	return *pDestPath;
}

fs::CPath* CPathRenamePairs::FindDestPath( const fs::CPath& srcPath )
{
	REQUIRE( IsConsistent() );

	if ( size_t* pIndex = utl::FindValuePtr( m_pathToIndexMap, srcPath ) )
	{
		ASSERT( *pIndex < m_pairs.size() );
		return &m_pairs[ *pIndex ].second;
	}
	return NULL;
}

void CPathRenamePairs::ResetDestPaths( void )
{
	REQUIRE( IsConsistent() );

	for ( TPairVector::iterator itPair = m_pairs.begin(); itPair != m_pairs.end(); ++itPair )
		itPair->second.Clear();
}

void CPathRenamePairs::CopySrcToDestPaths( void )
{
	REQUIRE( IsConsistent() );

	for ( TPairVector::iterator itPair = m_pairs.begin(); itPair != m_pairs.end(); ++itPair )
		itPair->second = itPair->first;
}

void CPathRenamePairs::CopyDestPaths( const std::vector< fs::CPath >& destPaths )		// assume destPaths is in same order as m_pairs
{
	REQUIRE( IsConsistent() );

	size_t pos = 0;
	for ( TPairVector::iterator itPair = m_pairs.begin(); itPair != m_pairs.end(); ++itPair, ++pos )
		itPair->second = destPaths[ pos ];

	ENSURE( IsConsistent() );
}
