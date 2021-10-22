
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
											const std::vector< CPathItem* >& ignorePathItems,
											CWnd* pParent ) throws_( CUserAbortedException )
{
	CDuplicatesProgressService progress( pParent );
	ui::IProgressService* pProgress = progress.GetService();

	std::vector< fs::CPath > foundPaths;

	SearchForFiles( foundPaths, srcPathItems, ignorePathItems, progress.GetProgressEnumerator() );

	// optimize performance:
	//	step 1: compute file-size part of the content key, grouping duplicate candidates by file-size only
	//	step 2: compute CRC32: real duplicates are within size-based duplicate groups

	CDuplicateGroupStore groupsStore;

	progress.Section_GroupByFileSize( foundPaths.size() );
	GroupByFileSize( &groupsStore, foundPaths, pProgress );

	progress.Section_GroupByCrc32( groupsStore.GetDuplicateItemCount() );			// count of duplicate candidates
	GroupByCrc32( rDuplicateGroups, &groupsStore, pProgress );
}

void CDuplicateFilesFinder::SearchForFiles( std::vector< fs::CPath >& rFoundPaths,
											const std::vector< CPathItem* >& srcPathItems,
											const std::vector< CPathItem* >& ignorePathItems,
											fs::IEnumerator* pProgressEnum )
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
			fs::CEnumerator found( pProgressEnum );
			found.SetIgnorePathMatches( ignorePaths );

			pProgressEnum->AddFoundSubDir( srcPath.GetPtr() );		// progress only: advance stage to the root directory
			fs::EnumFiles( &found, srcPath, m_wildSpec.c_str(), fs::EF_Recurse );

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

void CDuplicateFilesFinder::GroupByFileSize( CDuplicateGroupStore* pGroupsStore, const std::vector< fs::CPath >& foundPaths, ui::IProgressService* pProgress )
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

		pProgress->AdvanceItem( filePath.Get() );
	}
}

void CDuplicateFilesFinder::GroupByCrc32( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, CDuplicateGroupStore* pGroupsStore, ui::IProgressService* pProgress )
{
	utl::CSectionGuard section( _T("# ExtractDuplicateGroups (CRC32)") );

	utl::COwningContainer< std::vector< CDuplicateFilesGroup* > > newDuplicateGroups;
	pGroupsStore->ExtractDuplicateGroups( newDuplicateGroups, m_outcome.m_ignoredCount, pProgress );

	rDuplicateGroups.swap( newDuplicateGroups );		// swap items and ownership
}


// CDuplicatesProgressService implementation

CDuplicatesProgressService::CDuplicatesProgressService( CWnd* pParent )
	: m_dlg( _T("Search for Duplicate Files"), CProgressDialog::StageLabelCount )
{
	if ( m_dlg.Create( _T("Duplicate Files Search"), pParent ) )
	{
		GetHeader()->SetStageLabel( _T("Search directory") );
		GetHeader()->SetItemLabel( _T("Found file") );
		GetService()->SetMarqueeProgress();
	}
}

CDuplicatesProgressService::~CDuplicatesProgressService()
{
	m_dlg.DestroyWindow();
}

void CDuplicatesProgressService::Section_GroupByFileSize( size_t fileCount )
{
	GetHeader()->SetOperationLabel( _T("Group Files by Size") );
	GetHeader()->ShowStage( false );
	GetHeader()->SetItemLabel( _T("Compute file size") );

	GetService()->SetBoundedProgressCount( fileCount );
}

void CDuplicatesProgressService::Section_GroupByCrc32( size_t itemCount )
{
	GetHeader()->SetOperationLabel( _T("Group Files by CRC32 Checksum") );
	GetHeader()->SetStageLabel( _T("Group of duplicates") );
	GetHeader()->SetItemLabel( _T("Compute file CRC32") );

	GetService()->SetBoundedProgressCount( itemCount );

	m_dlg.SetProgressStep( 1 );								// advance progress on each step since individual computations are slow
	GetService()->SetProgressState( PBST_PAUSED );			// yellow bar
}

void CDuplicatesProgressService::AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException )
{
	GetService()->AdvanceItem( pFilePath );
}

bool CDuplicatesProgressService::AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException )
{
	GetService()->AdvanceStage( pSubDirPath );
	return true;
}
