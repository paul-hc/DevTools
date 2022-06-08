#ifndef PathRenamePairs_h
#define PathRenamePairs_h
#pragma once

#include "Path.h"
#include <hash_map>


class CPathRenamePairs
{
public:
	typedef std::vector< fs::TPathPair > TPairVector;
	typedef TPairVector::const_iterator const_iterator;
public:
	CPathRenamePairs( void ) {}

	bool IsEmpty( void ) const { return m_pairs.empty(); }

	const TPairVector& GetPairs( void ) const { return m_pairs; }
	TPairVector& RefPairs( void ) { return m_pairs; }		// careful never to change the pair.first SRC key!

	const_iterator Begin( void ) const { return m_pairs.begin(); }
	const_iterator End( void ) const { return m_pairs.end(); }

	bool ContainsSrc( const fs::CPath& srcPath ) const { REQUIRE( !srcPath.IsEmpty() ); return m_pathToIndexMap.find( srcPath ) != m_pathToIndexMap.end(); }

	void Clear( void );
	void AddPair( const fs::CPath& srcPath, const fs::CPath& destPath );
	void AddSrc( const fs::CPath& srcPath ) { AddPair( srcPath, fs::CPath() ); }

	// map interface
	fs::CPath& operator[]( const fs::CPath& srcPath );
	fs::CPath* FindDestPath( const fs::CPath& srcPath );
	const fs::CPath* FindDestPath( const fs::CPath& srcPath ) const { return const_cast<CPathRenamePairs*>( this )->FindDestPath( srcPath ); }

	// DEST assignment
	void ResetDestPaths( void );
	void CopySrcToDestPaths( void );
	void CopyDestPaths( const std::vector< fs::CPath >& destPaths );		// assume destPaths is in same order as m_pairs
private:
	bool IsConsistent( void ) const { return m_pairs.size() == m_pathToIndexMap.size(); }
private:
	TPairVector m_pairs;										// maintains the order for sequence generation
	stdext::hash_map< fs::CPath, size_t > m_pathToIndexMap;		// map src -> m_pairs.index
};


#endif // PathRenamePairs_h
