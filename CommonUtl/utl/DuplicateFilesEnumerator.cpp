
#include "stdafx.h"
#include "DuplicateFilesEnumerator.h"
#include "ContainerUtilities.h"
#include "FileEnumerator.h"
#include "Guards.h"
#include "IProgressService.h"
#include <hash_set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDuplicateFilesEnumerator implementation

CDuplicateFilesEnumerator::CDuplicateFilesEnumerator( void )
	: m_pProgressSvc( svc::CNoProgressService::Instance() )
	, m_pProgressEnum( NULL )
	, m_minFileSize( 0 )
{
	ASSERT_PTR( m_pProgressSvc );
}

CDuplicateFilesEnumerator::CDuplicateFilesEnumerator( utl::IProgressService* pProgressSvc, fs::IEnumerator* pProgressEnum )
	: m_pProgressSvc( pProgressSvc )
	, m_pProgressEnum( pProgressEnum )
	, m_minFileSize( 0 )
{
	ASSERT_PTR( m_pProgressSvc );
}

void CDuplicateFilesEnumerator::FindDuplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups,
											const std::vector< fs::CPath >& searchPaths,
											const std::vector< fs::CPath >& ignorePaths ) throws_( CUserAbortedException )
{
	std::vector< fs::CPath > foundPaths;

	SearchForFiles( foundPaths, searchPaths, ignorePaths );

	// optimize performance:
	//	step 1: compute file-size part of the content key, grouping duplicate candidates by file-size only
	//	step 2: compute CRC32: real duplicates are within size-based duplicate groups

	CDuplicateGroupStore groupsStore;

	  ProgSection_GroupByFileSize( foundPaths.size() );
	GroupByFileSize( &groupsStore, foundPaths );

	  ProgSection_GroupByCrc32( groupsStore.GetDuplicateItemCount() );			// count of duplicate candidates
	GroupByCrc32( rDuplicateGroups, &groupsStore );
}

void CDuplicateFilesEnumerator::SearchForFiles( std::vector< fs::CPath >& rFoundPaths,
											const std::vector< fs::CPath >& searchPaths,
											const std::vector< fs::CPath >& ignorePaths )
{
	utl::CSectionGuard section( _T("# SearchForFiles") );

	stdext::hash_set< fs::CPath > uniquePaths;

	for ( std::vector< fs::CPath >::const_iterator itSearchPath = searchPaths.begin(); itSearchPath != searchPaths.end(); ++itSearchPath )
	{
		const fs::CPath& searchPath = *itSearchPath;
		if ( fs::IsValidDirectory( searchPath.GetPtr() ) )
		{
			fs::CPathEnumerator found( fs::EF_Recurse, m_pProgressEnum );
			found.RefOptions().m_ignorePathMatches.Reset( ignorePaths );

			if ( m_pProgressEnum != NULL )
				m_pProgressEnum->AddFoundSubDir( searchPath.GetPtr() );		// progress only: advance stage to the root directory
			fs::EnumFiles( &found, searchPath, m_wildSpec.c_str() );

			fs::SortPaths( found.m_filePaths );

			rFoundPaths.reserve( rFoundPaths.size() + found.m_filePaths.size() );

			for ( std::vector< fs::CPath >::const_iterator itFilePath = found.m_filePaths.begin(); itFilePath != found.m_filePaths.end(); ++itFilePath )
				if ( uniquePaths.insert( *itFilePath ).second )		// path is unique?
					rFoundPaths.push_back( *itFilePath );

			m_outcome.m_searchedDirCount += 1 + found.m_subDirPaths.size();		// this directory + sub-directories
		}
		else if ( fs::IsValidFile( searchPath.GetPtr() ) )
			if ( uniquePaths.insert( searchPath ).second )				// path is unique?
				rFoundPaths.push_back( searchPath );
	}

	m_outcome.m_foundFileCount = rFoundPaths.size();
}

void CDuplicateFilesEnumerator::GroupByFileSize( CDuplicateGroupStore* pGroupsStore, const std::vector< fs::CPath >& foundPaths )
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

void CDuplicateFilesEnumerator::GroupByCrc32( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, CDuplicateGroupStore* pGroupsStore )
{
	utl::CSectionGuard section( _T("# ExtractDuplicateGroups (CRC32)") );

	utl::COwningContainer< std::vector< CDuplicateFilesGroup* > > newDuplicateGroups;
	pGroupsStore->ExtractDuplicateGroups( newDuplicateGroups, m_outcome.m_ignoredCount, m_pProgressSvc );

	rDuplicateGroups.swap( newDuplicateGroups );		// swap items and ownership
}

void CDuplicateFilesEnumerator::ProgSection_GroupByFileSize( size_t fileCount ) const
{
	if ( utl::IProgressHeader* pProgHeader = m_pProgressSvc->GetHeader() )
	{
		pProgHeader->SetOperationLabel( _T("Group Files by Size") );
		pProgHeader->ShowStage( false );
		pProgHeader->SetItemLabel( _T("Compute file size") );
	}

	m_pProgressSvc->SetBoundedProgressCount( fileCount );
}

void CDuplicateFilesEnumerator::ProgSection_GroupByCrc32( size_t itemCount ) const
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
