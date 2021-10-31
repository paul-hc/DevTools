
#include "stdafx.h"
#include "FileStateEnumerator.h"
#include <hash_set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	void CFileStateEnumerator::OnAddFileInfo( const CFileFind& foundFile )
	{
		__super::OnAddFileInfo( foundFile );

		m_fileStates.push_back( fs::CFileState( foundFile ) );
	}

	size_t CFileStateEnumerator::UniquifyAll( void )
	{
		stdext::hash_set< fs::CPath > uniquePaths;

		return
			path::UniquifyPaths( m_fileStates, uniquePaths ) +
			path::UniquifyPaths( m_subDirPaths, uniquePaths );
	}
}
