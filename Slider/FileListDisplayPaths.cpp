
#include "stdafx.h"
#include "FileListDisplayPaths.h"
#include "AlbumModel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFileListDisplayPaths::CFileListDisplayPaths( const CAlbumModel& model, bool filesMustExist )
	: m_filesMustExist( filesMustExist )
{
	m_paths.resize( model.GetFileAttrCount() );

	for ( size_t dispPos = 0; dispPos != m_paths.size(); ++dispPos )
		m_paths[ dispPos ] = &model.GetFileAttr( dispPos )->GetPath();		// translate display pos
}

int CFileListDisplayPaths::GetPos( size_t pos ) const
{
	if ( pos < m_paths.size() )
		if ( !m_filesMustExist || m_paths[ pos ]->FileExist() )
			return static_cast< int >( pos );

	return -1;
}

void CFileListDisplayPaths::SetListState( CListViewState& rLvState, std::auto_ptr< CListViewState::CImpl< int > >& pIndexState )
{
	ASSERT( rLvState.IsConsistent() );
	rLvState.Clear();

	if ( rLvState.UseIndexes() )
		rLvState.m_pIndexImpl = pIndexState;
	else
	{
		std::vector< std::tstring > selStrings;
		selStrings.reserve( pIndexState->m_selItems.size() );
		for ( std::vector< int >::const_iterator itSelIndex = pIndexState->m_selItems.begin(); itSelIndex != pIndexState->m_selItems.end(); ++itSelIndex )
			if ( GetPos( *itSelIndex ) != -1 )
				selStrings.push_back( m_paths[ *itSelIndex ]->Get() );

		rLvState.m_pStringImpl->m_selItems.swap( selStrings );
	}
}

CListViewState::CImpl< int >* CFileListDisplayPaths::MakeIndexState( const CListViewState& lvState, const CAlbumModel& model )
{
	if ( lvState.UseIndexes() )
		return new CListViewState::CImpl< int >( *lvState.m_pIndexImpl );		// straight copy

	CListViewState::CImpl< int >* pIndexState = new CListViewState::CImpl< int >;
	pIndexState->m_caret = model.FindFileAttr( lvState.m_pStringImpl->m_caret );
	pIndexState->m_top = model.FindFileAttr( lvState.m_pStringImpl->m_top );

	std::vector< int > selIndexes;
	selIndexes.reserve( lvState.m_pStringImpl->m_selItems.size() );
	for ( std::vector< std::tstring >::const_iterator itSelItem = lvState.m_pStringImpl->m_selItems.begin(); itSelItem != lvState.m_pStringImpl->m_selItems.end(); ++itSelItem )
	{
		int selIndex = model.FindFileAttr( *itSelItem );
		if ( selIndex != -1 )
			selIndexes.push_back( selIndex );
	}
	pIndexState->m_selItems.swap( selIndexes );

	return pIndexState;
}
