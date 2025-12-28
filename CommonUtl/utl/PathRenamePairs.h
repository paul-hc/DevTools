#ifndef PathRenamePairs_h
#define PathRenamePairs_h
#pragma once

#include "Path.h"
#include <unordered_map>


namespace pred
{
	struct IsPathChanged : public std::unary_function<fs::TPathPair, bool>
	{
		bool operator()( const fs::TPathPair& pathPair ) const
		{
			return pathPair.first.Get() != pathPair.second.Get();
		}
	};
}


class CPathRenamePairs
{
public:
	typedef std::vector<fs::TPathPair> TPairVector;
	typedef TPairVector::const_iterator const_iterator;
public:
	CPathRenamePairs( void ) {}

	bool IsEmpty( void ) const { return m_pairs.empty(); }
	bool AnyChanges( void ) const;

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
	void CopyDestPaths( const std::vector<fs::CPath>& destPaths );		// assume destPaths is in same order as m_pairs

	// SRC setup
	template< typename PairContainerT >
	void StoreSrcFromPairs( const PairContainerT& srcPairs );

	template< typename ContainerT >
	void StoreSrcFromPaths( const ContainerT& srcPaths );

	// DEST query
	template< typename PairContainerT >
	void QueryDestToPairs( OUT PairContainerT& rDestPairs ) const;

	template< typename DestIterT >
	void QueryDestPaths( OUT DestIterT itOutDest ) const;

	template< typename FuncT >
	FuncT ForEachDestPath( FuncT func );
private:
	bool IsConsistent( void ) const { return m_pairs.size() == m_pathToIndexMap.size(); }
private:
	TPairVector m_pairs;										// maintains the order for sequence generation
	std::unordered_map<fs::CPath, size_t> m_pathToIndexMap;		// map src -> m_pairs.index
};


// CPathRenamePairs template methods

template< typename PairContainerT >
void CPathRenamePairs::StoreSrcFromPairs( const PairContainerT& srcPairs )
{
	Clear();
	for ( typename PairContainerT::const_iterator it = srcPairs.begin(); it != srcPairs.end(); ++it )
		AddSrc( fs::traits::GetPath( it->first ) );
}

template< typename ContainerT >
void CPathRenamePairs::StoreSrcFromPaths( const ContainerT& srcPaths )
{
	Clear();
	for ( typename ContainerT::const_iterator it = srcPaths.begin(); it != srcPaths.end(); ++it )
		AddSrc( fs::traits::GetPath( *it ) );
}

template< typename PairContainerT >
void CPathRenamePairs::QueryDestToPairs( OUT PairContainerT& rDestPairs ) const
{	// copy dest paths from map to the vector-like PairContainerT
	ASSERT( m_pairs.size() == rDestPairs.size() );			// ensure we don't have extra duplicates in rDestPairs

	for ( typename PairContainerT::iterator itDestPair = rDestPairs.begin(); itDestPair != rDestPairs.end(); ++itDestPair )
	{
		const fs::CPath* pDestPath = FindDestPath( itDestPair->first );
		ASSERT_PTR( pDestPath );
		itDestPair->second.Set( pDestPath->Get() );		// may convert from fs::CPath to fs::CFlexPath, if PairContainerT uses fs::CFlexPath
	}
}

template< typename DestIterT >
void CPathRenamePairs::QueryDestPaths( OUT DestIterT itOutDest ) const
{	// copy dest paths from pairs
	for ( TPairVector::const_iterator itPair = m_pairs.begin(); itPair != m_pairs.end(); ++itPair )
		*itOutDest++ = itPair->second;
}

template< typename FuncT > inline
FuncT CPathRenamePairs::ForEachDestPath( FuncT func )
{
	for ( TPairVector::iterator itPair = m_pairs.begin(); itPair != m_pairs.end(); ++itPair )
		func( itPair->second );

	return func;
}


#endif // PathRenamePairs_h
