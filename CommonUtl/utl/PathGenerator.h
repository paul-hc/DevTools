#ifndef PathGenerator_h
#define PathGenerator_h
#pragma once

#include "PathFormatter.h"
#include "PathMaker.h"


// formatted path generator, with wildcard and/or numeric sequence

class CPathGenerator : public CPathMaker
{
public:
	CPathGenerator( const CPathFormatter& formatter, UINT seqCount, bool avoidDups = true );			// allocates internal file map, with ownership
	CPathGenerator( fs::TPathPairMap* pOutRenamePairs, const CPathFormatter& formatter, UINT seqCount, bool avoidDups = true );
	CPathGenerator( const fs::TPathPairMap& renamePairs, const CPathFormatter& formatter, UINT seqCount = 1, bool avoidDups = true );		// read-only; for FindNextAvailSeqCount()

	const CPathFormatter& GetFormat( void ) const { return m_formatter; }
	void SetMoveDestDirPath( const fs::CPath& moveDestDirPath );		// generate for moving to destDirPath

	UINT GetSeqCount( void ) const { return m_seqCount; }

	bool GeneratePairs( void );										// generates to m_rRenamePairs
	bool Generate( std::vector< fs::CPath >& rDestPaths );			// generates to rDestPaths

	UINT FindNextAvailSeqCount( void ) const;

	enum { MaxSeqCount = USHRT_MAX, MaxDupCount = 200 };
private:
	bool IsPathUnique( const fs::CPath& newPath ) const;			// in current working set (may collide with existing source set for resequencing a range)

	bool GeneratePath( fs::CPath& rNewPath, const fs::CPath& srcPath );
private:
	CPathFormatter m_formatter;
	UINT m_seqCount;
	bool m_avoidDups;						// attempts to generate a unique path using a dupCount
	bool m_mutablePairs;
private:
	fs::TPathSet m_destSet;					// transient during generation
};


#endif // PathGenerator_h
