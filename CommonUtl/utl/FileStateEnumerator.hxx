#ifndef FileStateEnumerator_hxx
#define FileStateEnumerator_hxx

#include "ContainerOwnership.h"
#include "PathItemBase.h"


namespace fs
{
	// CFileStateItemEnumerator template code

	template< typename FileStateItemT, typename CreateFuncT >
	void CFileStateItemEnumerator<FileStateItemT, CreateFuncT>::Clear( void )
	{
		utl::ClearOwningContainer( m_fileItems );
		__super::Clear();
	}

	template< typename FileStateItemT, typename CreateFuncT >
	void CFileStateItemEnumerator<FileStateItemT, CreateFuncT>::OnAddFileInfo( const fs::CFileState& fileState )
	{
		m_fileItems.push_back( m_createFunc( fileState ) );

		__super::OnAddFileInfo( fileState );		// take care of file progress chaining
	}

	template< typename FileStateItemT, typename CreateFuncT >
	inline void CFileStateItemEnumerator<FileStateItemT, CreateFuncT>::SortItems( void )
	{
		func::SortPathItems( m_fileItems );			// by path key
	}
}


#endif // FileStateEnumerator_hxx
