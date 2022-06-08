#ifndef PathMaker_h
#define PathMaker_h
#pragma once

#include "Path.h"
#include "PathRenamePairs.h"


// simple path maker from SRC to DEST; not using a format

class CPathMaker : private utl::noncopyable
{
public:
	CPathMaker( void );									// allocates internal file map, with ownership
	CPathMaker( CPathRenamePairs* pRenamePairs );		// reference external file map
	~CPathMaker();

	bool MakeDestRelative( const std::tstring& prefixDirPath );		// SRC path -> relative DEST path
	bool MakeDestStripCommonPrefix( void );							// SRC path -> DEST path, with stripped common prefix

	std::tstring FindSrcCommonPrefix( void ) const;

	// src map setup
	template< typename PairContainer >
	void StoreSrcFromPairs( const PairContainer& srcPairs )
	{
		m_pRenamePairs->Clear();
		for ( typename PairContainer::const_iterator it = srcPairs.begin(); it != srcPairs.end(); ++it )
			m_pRenamePairs->AddSrc( fs::traits::GetPath( it->first ) );
	}

	template< typename Container >
	void StoreSrcFromPaths( const Container& srcPaths )
	{
		m_pRenamePairs->Clear();
		for ( typename Container::const_iterator it = srcPaths.begin(); it != srcPaths.end(); ++it )
			m_pRenamePairs->AddSrc( fs::traits::GetPath( *it ) );
	}
protected:
	CPathRenamePairs* m_pRenamePairs;
private:
	bool m_mapOwnership;					// internal built map, deleted on destructor
};


#endif // PathMaker_h
