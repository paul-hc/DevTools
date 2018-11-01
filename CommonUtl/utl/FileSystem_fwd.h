#ifndef FileSystem_fwd_h
#define FileSystem_fwd_h
#pragma once

#include "Path.h"


namespace fs
{
	interface IEnumerator
	{
		virtual void AddFoundFile( const TCHAR* pFilePath ) = 0;
		virtual void AddFoundSubDir( const TCHAR* pSubDirPath ) { pSubDirPath; }

		// advanced, provides extra info
		virtual void AddFile( const CFileFind& foundFile ) { AddFoundFile( foundFile.GetFilePath() ); }

		// override to find first file, then abort searching
		virtual bool MustStop( void ) { return false; }
	};
}


#endif // FileSystem_fwd_h
