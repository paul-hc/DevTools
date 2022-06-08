#ifndef TempFilePairPool_h
#define TempFilePairPool_h
#pragma once

#ifdef USE_UT		// no UT code in release builds

#include "utl/PathRenamePairs.h"
#include "UnitTest.h"


namespace ut
{
	// no physical files
	//
	struct CPathPairPool
	{
		CPathPairPool( void ) : m_fullDestPaths( false ) {}
		CPathPairPool( const TCHAR* pSourceFilenames, bool fullDestPaths = false );

		std::tstring JoinDest( void );
		void CopySrc( void );
	public:
		CPathRenamePairs m_pathPairs;
		bool m_fullDestPaths;				// format DEST in JoinDest using full paths
	};


	// physical files managed in a temporary _UT directory
	//
	struct CTempFilePairPool : public CTempFilePool, public CPathPairPool
	{
		CTempFilePairPool( const TCHAR* pSourceFilenames );
	};
}


#endif //USE_UT

#endif // TempFilePairPool_h
