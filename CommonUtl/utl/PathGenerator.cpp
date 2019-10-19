
#include "stdafx.h"
#include "PathGenerator.h"
#include "ContainerUtilities.h"
#include "FileSystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPathGenerator::CPathGenerator( const CPathFormatter& formatter, UINT seqCount, bool avoidDups /*= true*/ )
	: CPathMaker()
	, m_formatter( formatter )
	, m_seqCount( seqCount )
	, m_avoidDups( avoidDups )
	, m_mutablePairs( true )
{
	ASSERT( m_seqCount < MaxSeqCount );
}

CPathGenerator::CPathGenerator( fs::TPathPairMap* pOutRenamePairs, const CPathFormatter& formatter, UINT seqCount, bool avoidDups /*= true*/ )
	: CPathMaker( pOutRenamePairs )
	, m_formatter( formatter )
	, m_seqCount( seqCount )
	, m_avoidDups( avoidDups )
	, m_mutablePairs( true )
{
	ASSERT( m_seqCount < MaxSeqCount );
}

CPathGenerator::CPathGenerator( const fs::TPathPairMap& renamePairs, const CPathFormatter& formatter, UINT seqCount /*= 1*/, bool avoidDups /*= true*/ )
	: CPathMaker( const_cast< fs::TPathPairMap* >( &renamePairs ) )
	, m_formatter( formatter )
	, m_seqCount( seqCount )
	, m_avoidDups( avoidDups )
	, m_mutablePairs( false )
{
	ASSERT( m_seqCount < MaxSeqCount );
}

void CPathGenerator::SetMoveDestDirPath( const fs::CPath& moveDestDirPath )
{
	m_formatter.SetMoveDestDirPath( moveDestDirPath );
}

bool CPathGenerator::GeneratePairs( void )
{
	ASSERT( m_mutablePairs );

	std::vector< fs::CPath > destPaths;
	if ( !Generate( destPaths ) )
	{
		ResetDestPaths();
		return false;
	}

	CopyDestPaths( destPaths );
	return true;
}

bool CPathGenerator::Generate( std::vector< fs::CPath >& rDestPaths )
{
	m_destSet.clear();
	rDestPaths.clear();

	if ( !m_formatter.IsValidFormat() )
		return false;

	std::vector< fs::CPath > destPaths;
	destPaths.reserve( m_pRenamePairs->size() );

	for ( fs::TPathPairMap::const_iterator it = m_pRenamePairs->begin(); it != m_pRenamePairs->end(); ++it )
	{
		fs::CPath newPath;
		if ( GeneratePath( newPath, it->first ) )
			destPaths.push_back( newPath );
		else
			return false;
	}

	rDestPaths.swap( destPaths );
	return true;
}

UINT CPathGenerator::FindNextAvailSeqCount( void ) const
{
	UINT nextAvailSeqCount = 1;

	if ( m_formatter.IsNumericFormat() && !m_pRenamePairs->empty() )
	{
		// sweep existing files (source + outside) that match current format for the lowest seq count that is not used
		fs::CPath dirPath = m_pRenamePairs->begin()->first.GetParentPath();			// use first source file dir path

		std::vector< fs::CPath > existingFilePaths;
		fs::EnumFiles( existingFilePaths, dirPath );

		for ( std::vector< fs::CPath >::const_iterator itFilePath = existingFilePaths.begin(); itFilePath != existingFilePaths.end(); ++itFilePath )
		{
			UINT seqCount;
			if ( m_formatter.ParseSeqCount( seqCount, *itFilePath ) )
				nextAvailSeqCount = std::max( nextAvailSeqCount, ++seqCount );		// increment for next avail
		}
	}
	return nextAvailSeqCount;
}

bool CPathGenerator::GeneratePath( fs::CPath& rNewPath, const fs::CPath& srcPath )
{
	UINT newSeqCount = m_seqCount;

	if ( m_avoidDups )
	{
		for ( UINT dupCount = 1; ; )
		{
			rNewPath = m_formatter.FormatPath( srcPath, newSeqCount, dupCount );
			if ( m_formatter.IsNumericFormat() )
			{
				if ( newSeqCount >= MaxSeqCount )
					return false;					// sequence num too large

				++newSeqCount;						// numeric format: increment seq count to fill the available seq numbers (no dup suffix, which looks ugly)
			}
			else
			{
				ASSERT( m_formatter.IsWildcardFormat() );
				++dupCount;							// wildcard format: increment dup count suffix
				if ( dupCount > MaxDupCount )
					return false;					// too many colisions with existing duplicates
			}

			if ( IsPathUnique( rNewPath ) )
				break;
		}
	}
	else
		rNewPath = m_formatter.FormatPath( srcPath, newSeqCount++ );

	if ( !m_destSet.insert( rNewPath ).second )
		return false;								// collision with existing dest path

	if ( m_formatter.IsNumericFormat() )
		m_seqCount = newSeqCount;					// advance seq count only if successful
	return true;
}

bool CPathGenerator::IsPathUnique( const fs::CPath& newPath ) const
{
	if ( newPath.FileExist() )
		if ( m_pRenamePairs->find( newPath ) == m_pRenamePairs->end() )
			return false;							// collision with an existing file outside of current working set

	return m_destSet.find( newPath ) == m_destSet.end();
}
