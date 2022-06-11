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

	// SRC setup
	template< typename PairContainer >
	void StoreSrcFromPairs( const PairContainer& srcPairs );

	template< typename Container >
	void StoreSrcFromPaths( const Container& srcPaths );

	// DEST query
	template< typename PairContainer >
	void QueryDestToPairs( PairContainer& rDestPairs ) const;

	template< typename Container >
	void QueryDestToPaths( Container& rDestPaths ) const;

	template< typename Func >
	Func ForEachDestPath( Func func );
private:
	bool IsConsistent( void ) const { return m_pairs.size() == m_pathToIndexMap.size(); }
private:
	TPairVector m_pairs;										// maintains the order for sequence generation
	stdext::hash_map< fs::CPath, size_t > m_pathToIndexMap;		// map src -> m_pairs.index
};


// CPathRenamePairs template methods

template< typename PairContainer >
void CPathRenamePairs::StoreSrcFromPairs( const PairContainer& srcPairs )
{
	Clear();
	for ( typename PairContainer::const_iterator it = srcPairs.begin(); it != srcPairs.end(); ++it )
		AddSrc( fs::traits::GetPath( it->first ) );
}

template< typename Container >
void CPathRenamePairs::StoreSrcFromPaths( const Container& srcPaths )
{
	Clear();
	for ( typename Container::const_iterator it = srcPaths.begin(); it != srcPaths.end(); ++it )
		AddSrc( fs::traits::GetPath( *it ) );
}

template< typename PairContainer >
void CPathRenamePairs::QueryDestToPairs( PairContainer& rDestPairs ) const
{	// copy dest paths from map to the vector-like PairContainer
	ASSERT( m_pairs.size() == rDestPairs.size() );			// ensure we don't have extra duplicates in rDestPairs

	for ( typename PairContainer::iterator itDestPair = rDestPairs.begin(); itDestPair != rDestPairs.end(); ++itDestPair )
	{
		const fs::CPath* pDestPath = FindDestPath( itDestPair->first );
		ASSERT_PTR( pDestPath );
		itDestPair->second.Set( pDestPath->Get() );		// may convert from fs::CPath to fs::CFlexPath, if PairContainer uses fs::CFlexPath
	}
}

template< typename Container >
void CPathRenamePairs::QueryDestToPaths( Container& rDestPaths ) const
{	// copy dest paths from map to the vector-like Container using path adapters
	ASSERT( m_pairs.size() == rDestPaths.size() );

	for ( typename Container::iterator itDestPath = rDestPaths.begin(); itDestPath != rDestPaths.end(); ++itDestPath )
	{
		const fs::CPath* pDestPath = FindDestPath( fs::traits::GetPath( *itDestPath ) );
		ASSERT_PTR( pDestPath );

		fs::traits::SetPath( *itDestPath, pDestPath->Get() );
	}
}

template< typename Func > inline
Func CPathRenamePairs::ForEachDestPath( Func func )
{
	for ( TPairVector::iterator it = m_pairs.begin(); it != m_pairs.end(); ++it )
		func( it->second );

	return func;
}


#endif // PathRenamePairs_h
