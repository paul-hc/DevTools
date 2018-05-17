#ifndef PathMaker_h
#define PathMaker_h
#pragma once

#include "Path.h"


namespace fs
{
	namespace traits
	{
		inline const fs::CPath& GetPath( const fs::CPath& filePath ) { return filePath; }

		template< typename DestType >
		inline void SetPath( DestType& rDest, const fs::CPath& filePath ) { rDest.Set( filePath.Get() ); }
	}
}


// simple path maker from SRC to DEST; not using a format

class CPathMaker : private utl::noncopyable
{
public:
	CPathMaker( void );									// allocates internal file map, with ownership
	CPathMaker( fs::TPathPairMap* pRenamePairs );		// reference external file map
	~CPathMaker();

	bool MakeDestRelative( const std::tstring& prefixDirPath );		// SRC path -> relative DEST path
	bool MakeDestStripCommonPrefix( void );							// SRC path -> DEST path, with stripped common prefix

	std::tstring FindSrcCommonPrefix( void ) const;

	// src map setup
	template< typename PairContainer >
	void StoreSrcFromPairs( const PairContainer& srcPairs );

	template< typename Container >
	void StoreSrcFromPaths( const Container& srcPaths );

	// dest map query
	template< typename PairContainer >
	void QueryDestToPairs( PairContainer& rDestPairs ) const;

	template< typename Container >
	void QueryDestToPaths( Container& rDestPaths ) const;

	template< typename Func >
	Func ForEachDestPath( Func func );
protected:
	void CopyDestPaths( const std::vector< fs::CPath >& destPaths );
	void ResetDestPaths( void );
protected:
	fs::TPathPairMap* m_pRenamePairs;
private:
	bool m_mapOwnership;					// internal built map, deleted on destructor
};


// CPathMaker template code

template< typename PairContainer >
void CPathMaker::StoreSrcFromPairs( const PairContainer& srcPairs )
{
	m_pRenamePairs->clear();
	for ( typename PairContainer::const_iterator it = srcPairs.begin(); it != srcPairs.end(); ++it )
		( *m_pRenamePairs )[ fs::traits::GetPath( it->first ) ];
}

template< typename Container >
void CPathMaker::StoreSrcFromPaths( const Container& srcPaths )
{
	m_pRenamePairs->clear();
	for ( typename Container::const_iterator it = srcPaths.begin(); it != srcPaths.end(); ++it )
		( *m_pRenamePairs )[ fs::traits::GetPath( *it ) ];
}

template< typename PairContainer >
void CPathMaker::QueryDestToPairs( PairContainer& rDestPairs ) const
{	// copy dest paths from map to the vector-like PairContainer
	ASSERT( m_pRenamePairs->size() == rDestPairs.size() );			// ensure we don't have extra duplicates in rDestPairs

	for ( typename PairContainer::iterator itDestPair = rDestPairs.begin(); itDestPair != rDestPairs.end(); ++itDestPair )
	{
		fs::TPathPairMap::const_iterator itFound = m_pRenamePairs->find( itDestPair->first );
		ASSERT( itFound != m_pRenamePairs->end() );
		itDestPair->second.Set( itFound->second.Get() );			// may convert from fs::CPath to fs::CFlexPath, if PairContainer uses fs::CFlexPath
	}
}

template< typename Container >
void CPathMaker::QueryDestToPaths( Container& rDestPaths ) const
{	// copy dest paths from map to the vector-like Container using path adapters
	ASSERT( m_pRenamePairs->size() == rDestPaths.size() );

	for ( typename Container::iterator itDestPath = rDestPaths.begin(); itDestPath != rDestPaths.end(); ++itDestPath )
	{
		fs::TPathPairMap::const_iterator itFound = m_pRenamePairs->find( fs::traits::GetPath( *itDestPath ) );
		ASSERT( itFound != m_pRenamePairs->end() );
		fs::traits::SetPath( *itDestPath, itFound->second );
	}
}

template< typename Func > inline
Func CPathMaker::ForEachDestPath( Func func )
{
	for ( fs::TPathPairMap::iterator it = m_pRenamePairs->begin(); it != m_pRenamePairs->end(); ++it )
		func( it->second );

	return func;
}


#endif // PathMaker_h
