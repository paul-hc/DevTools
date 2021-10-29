
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

	void CFileStateEnumerator::AddFoundFile( const TCHAR* pFilePath )
	{
		if ( m_pChainEnum != NULL )
			m_pChainEnum->AddFoundFile( pFilePath );
	}

	bool CFileStateEnumerator::AddFoundSubDir( const TCHAR* pSubDirPath )
	{
		if ( m_pChainEnum != NULL )
			m_pChainEnum->AddFoundSubDir( pSubDirPath );

		fs::CPath subDirPath( pSubDirPath );

		if ( !subDirPath.IsEmpty() )
			m_subDirPaths.push_back( subDirPath );

		return true;
	}

	bool CFileStateEnumerator::MustStop( void ) const
	{
		return m_fileStates.size() >= m_maxFiles;
	}

	size_t CFileStateEnumerator::UniquifyAll( void )
	{
		stdext::hash_set< fs::CPath > uniquePaths;

		return
			fs::UniquifyPaths( m_fileStates, uniquePaths ) +
			fs::UniquifyPaths( m_subDirPaths, uniquePaths );
	}
}
