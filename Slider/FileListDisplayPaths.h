#ifndef FileListDisplayPaths_h
#define FileListDisplayPaths_h
#pragma once

#include "utl/FlexPath.h"
#include "ListViewState.h"


class CFileList;
struct CListViewState;


class CFileListDisplayPaths
{
public:
	CFileListDisplayPaths( const CFileList& fileList, bool filesMustExist );

	int GetPos( size_t pos ) const;
	void SetListState( CListViewState& rLvState, std::auto_ptr< CListViewState::CImpl< int > >& pIndexState );

	static CListViewState::CImpl< int >* MakeIndexState( const CListViewState& lvState, const CFileList& fileList );
private:
	std::vector< const fs::CFlexPath* > m_paths;
	bool m_filesMustExist;
};


#endif // FileListDisplayPaths_h
