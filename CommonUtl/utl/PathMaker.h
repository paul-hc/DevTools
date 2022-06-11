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

	const CPathRenamePairs* GetRenamePairs( void ) const { return m_pRenamePairs; }
	CPathRenamePairs* RefRenamePairs( void ) { return m_pRenamePairs; }
protected:
	CPathRenamePairs* m_pRenamePairs;
private:
	bool m_mapOwnership;					// internal built map, deleted on destructor
};


#endif // PathMaker_h
