#ifndef FileStateEnumerator_hxx
#define FileStateEnumerator_hxx

#include "ContainerUtilities.h"


namespace fs
{
	// CFileStateItemEnumerator template code

	template< typename FileStateItemT >
	void CFileStateItemEnumerator<FileStateItemT>::Clear( void )
	{
		utl::ClearOwningContainer( m_fileItems );
		__super::ClearBitFlag();
	}

	template< typename FileStateItemT >
	void CFileStateItemEnumerator<FileStateItemT>::OnAddFileInfo( const CFileFind& foundFile )
	{
		if ( m_pChainEnum != NULL )
			m_pChainEnum->OnAddFileInfo( foundFile );

		m_fileItems.push_back( new FileStateItemT( foundFile ) );
	}
}


#endif // FileStateEnumerator_hxx
