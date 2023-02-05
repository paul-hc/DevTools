
#include "pch.h"
#include "FileStateEnumerator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	void CFileStateEnumerator::OnAddFileInfo( const fs::CFileState& fileState )
	{
		__super::OnAddFileInfo( fileState );

		m_fileStates.push_back( fileState );
	}
}
