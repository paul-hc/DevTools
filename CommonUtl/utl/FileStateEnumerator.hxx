#ifndef FileStateEnumerator_hxx
#define FileStateEnumerator_hxx

#include "ContainerUtilities.h"
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
	void CFileStateItemEnumerator<FileStateItemT, CreateFuncT>::OnAddFileInfo( const CFileFind& foundFile )
	{
		m_fileItems.push_back( m_createFunc( foundFile ) );

		__super::OnAddFileInfo( foundFile );		// take care of file progress chaining
	}

	template< typename FileStateItemT, typename CreateFuncT >
	inline void CFileStateItemEnumerator<FileStateItemT, CreateFuncT>::SortItems( void )
	{
		std::sort( m_fileItems.begin(), m_fileItems.end(), pred::TLess_PathItem() );		// sort by path key
	}

	template< typename FileStateItemT, typename CreateFuncT >
	size_t CFileStateItemEnumerator<FileStateItemT, CreateFuncT>::UniquifyAll( void )
	{
		size_t duplicateCount = __super::UniquifyAll();

		std::vector< FileStateItemT* > duplicates;
		duplicateCount += utl::Uniquify<pred::TLess_PathItem>( m_fileItems, &duplicates );
		utl::ClearOwningContainer( duplicates );

		return duplicateCount;
	}
}


#endif // FileStateEnumerator_hxx
