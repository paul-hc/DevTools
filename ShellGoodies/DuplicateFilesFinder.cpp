
#include "stdafx.h"
#include "DuplicateFilesFinder.h"
#include "utl/ContainerUtilities.h"
#include "utl/FileEnumerator.h"
#include "utl/Guards.h"
#include <hash_set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDuplicateFilesFinder implementation

void CDuplicateFilesFinder::FindDuplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups,
											const std::vector< CPathItem* >& srcPathItems,
											const std::vector< CPathItem* >& ignorePathItems ) throws_( CUserAbortedException )
{
	std::vector< fs::CPath > foundPaths;

	SearchForFiles( foundPaths, srcPathItems, ignorePathItems );

	// optimize performance:
	//	step 1: compute file-size part of the content key, grouping duplicate candidates by file-size only
	//	step 2: compute CRC32: real duplicates are within size-based duplicate groups

	CDuplicateGroupStore groupsStore;

	  ProgSection_GroupByFileSize( foundPaths.size() );
	GroupByFileSize( &groupsStore, foundPaths );

	  ProgSection_GroupByCrc32( groupsStore.GetDuplicateItemCount() );			// count of duplicate candidates
	GroupByCrc32( rDuplicateGroups, &groupsStore );
}

void CDuplicateFilesFinder::SearchForFiles( std::vector< fs::CPath >& rFoundPaths,
											const std::vector< CPathItem* >& srcPathItems,
											const std::vector< CPathItem* >& ignorePathItems )
{
	utl::CSectionGuard section( _T("# SearchForFiles") );

	std::vector< fs::CPath > ignorePaths;
	func::QueryItemsPaths( ignorePaths, ignorePathItems );

	stdext::hash_set< fs::CPath > uniquePaths;

	for ( std::vector< CPathItem* >::const_iterator itSrcPathItem = srcPathItems.begin(); itSrcPathItem != srcPathItems.end(); ++itSrcPathItem )
	{
		const fs::CPath& srcPath = ( *itSrcPathItem )->GetFilePath();
		if ( fs::IsValidDirectory( srcPath.GetPtr() ) )
		{
			fs::CPathEnumerator found( fs::EF_Recurse, m_pProgressEnum );
			found.RefOptions().m_ignorePathMatches.Reset( ignorePaths );

			if ( m_pProgressEnum != NULL )
				m_pProgressEnum->AddFoundSubDir( srcPath.GetPtr() );		// progress only: advance stage to the root directory
			fs::EnumFiles( &found, srcPath, m_wildSpec.c_str() );

			fs::SortPaths( found.m_filePaths );

			rFoundPaths.reserve( rFoundPaths.size() + found.m_filePaths.size() );

			CWaitCursor wait;			// could take a long time for directories with many subdirectories and files
			for ( std::vector< fs::CPath >::const_iterator itFilePath = found.m_filePaths.begin(); itFilePath != found.m_filePaths.end(); ++itFilePath )
				if ( uniquePaths.insert( *itFilePath ).second )		// path is unique?
					rFoundPaths.push_back( *itFilePath );

			m_outcome.m_searchedDirCount += 1 + found.m_subDirPaths.size();		// this directory + sub-directories
		}
		else if ( fs::IsValidFile( srcPath.GetPtr() ) )
			if ( uniquePaths.insert( srcPath ).second )				// path is unique?
				rFoundPaths.push_back( srcPath );
	}

	m_outcome.m_foundFileCount = rFoundPaths.size();
}

void CDuplicateFilesFinder::GroupByFileSize( CDuplicateGroupStore* pGroupsStore, const std::vector< fs::CPath >& foundPaths )
{
	utl::CSectionGuard section( _T("# Grouping by duplicate file sizes") );

	for ( size_t i = 0; i != foundPaths.size(); ++i )
	{
		const fs::CPath& filePath = foundPaths[ i ];

		fs::CFileContentKey contentKey;
		bool registered = false;

		if ( contentKey.ComputeFileSize( filePath ) )
			if ( contentKey.m_fileSize >= m_minFileSize )				// has minimum size?
				registered = pGroupsStore->RegisterPath( filePath, contentKey );

		if ( !registered )
			++m_outcome.m_ignoredCount;

		m_pProgressSvc->AdvanceItem( filePath.Get() );
	}
}

void CDuplicateFilesFinder::GroupByCrc32( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, CDuplicateGroupStore* pGroupsStore )
{
	utl::CSectionGuard section( _T("# ExtractDuplicateGroups (CRC32)") );

	utl::COwningContainer< std::vector< CDuplicateFilesGroup* > > newDuplicateGroups;
	pGroupsStore->ExtractDuplicateGroups( newDuplicateGroups, m_outcome.m_ignoredCount, m_pProgressSvc );

	rDuplicateGroups.swap( newDuplicateGroups );		// swap items and ownership
}

void CDuplicateFilesFinder::ProgSection_GroupByFileSize( size_t fileCount ) const
{
	if ( utl::IProgressHeader* pProgHeader = m_pProgressSvc->GetHeader() )
	{
		pProgHeader->SetOperationLabel( _T("Group Files by Size") );
		pProgHeader->ShowStage( false );
		pProgHeader->SetItemLabel( _T("Compute file size") );
	}

	m_pProgressSvc->SetBoundedProgressCount( fileCount );
}

void CDuplicateFilesFinder::ProgSection_GroupByCrc32( size_t itemCount ) const
{
	if ( utl::IProgressHeader* pProgHeader = m_pProgressSvc->GetHeader() )
	{
		pProgHeader->SetOperationLabel( _T("Group Files by CRC32 Checksum") );
		pProgHeader->SetStageLabel( _T("Group of duplicates") );
		pProgHeader->SetItemLabel( _T("Compute file CRC32") );
	}

	m_pProgressSvc->SetBoundedProgressCount( itemCount );

	m_pProgressSvc->SetProgressStep( 1 );						// fine granularity: advance progress on each step since individual computations are slow
	m_pProgressSvc->SetProgressState( PBST_PAUSED );			// yellow bar
}
