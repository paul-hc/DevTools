
#include "pch.h"
#include "DuplicateFilesEnumerator.h"
#include "ContainerOwnership.h"
#include "Guards.h"
#include "IProgressService.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDuplicateFilesEnumerator implementation

CDuplicateFilesEnumerator::CDuplicateFilesEnumerator( fs::TEnumFlags enumFlags, IEnumerator* pChainEnum /*= NULL*/,
													  utl::IProgressService* pProgressSvc /*= svc::CNoProgressService::Instance()*/ )
	: fs::CBaseEnumerator( enumFlags, pChainEnum )
	, m_pProgressSvc( pProgressSvc )
	, m_pGroupStore( NULL )
{
	ASSERT_PTR( m_pProgressSvc );
}

void CDuplicateFilesEnumerator::Clear( void )
{
	utl::ClearOwningContainer( m_dupGroupItems );
	m_outcome = CDupsOutcome();

	__super::Clear();
}

void CDuplicateFilesEnumerator::SearchDuplicates( const std::vector<fs::TPatternPath>& searchPaths )
{
	Clear();

	CDuplicateGroupStore m_groupStore;
	m_pGroupStore = &m_groupStore;

	{
		utl::CSectionGuard section( _T("# SearchDuplicates") );

		for ( std::vector<fs::TPatternPath>::const_iterator itSearchPath = searchPaths.begin(); itSearchPath != searchPaths.end(); ++itSearchPath )
			fs::SearchEnumFiles( this, *itSearchPath );
	}

	m_outcome.m_foundSubDirCount = m_subDirPaths.size();
	m_outcome.m_ignoredCount = GetIgnoredPaths().size();

	// lazy evaluate CRC32 checksums - real duplicates are within size-based duplicate groups
	GroupByCrc32();

	func::SortDuplicateGroupItems( m_dupGroupItems );		// sort groups by original item path
	m_pGroupStore = NULL;
}

void CDuplicateFilesEnumerator::OnAddFileInfo( const fs::CFileState& fileState )
{
	ASSERT_PTR( m_pGroupStore );

	++m_outcome.m_foundFileCount;

	// first stage: CRC32 is not yet computed, group found items by file size
	m_pGroupStore->RegisterItem( new CDuplicateFileItem( fileState ) );	// generated groups are stored in the groups store

	__super::OnAddFileInfo( fileState );
}

void CDuplicateFilesEnumerator::AddFoundFile( const fs::CPath& filePath )
{
	__super::AddFoundFile( filePath );		// base method is pure but implemented
}

void CDuplicateFilesEnumerator::GroupByCrc32( void )
{
	ProgSection_GroupByCrc32();

	utl::CSectionGuard section( _T("# ExtractDuplicateGroups (CRC32)") );

	utl::COwningContainer< std::vector<CDuplicateFilesGroup*> > newDuplicateGroups;
	m_pGroupStore->ExtractDuplicateGroups( newDuplicateGroups, m_outcome.m_ignoredCount, m_pProgressSvc );

	m_dupGroupItems.swap( newDuplicateGroups );		// swap items and ownership
}

void CDuplicateFilesEnumerator::ProgSection_GroupByCrc32( void ) const
{
	ASSERT_PTR( m_pGroupStore );

	if ( utl::IProgressHeader* pProgHeader = m_pProgressSvc->GetHeader() )
	{
		pProgHeader->SetOperationLabel( _T("Group Files by CRC32 Checksum") );
		pProgHeader->SetStageLabel( _T("Group of duplicates") );
		pProgHeader->SetItemLabel( _T("Compute file CRC32") );
	}

	m_pProgressSvc->SetBoundedProgressCount( m_pGroupStore->GetDuplicateItemCount() );	// count of duplicate candidates

	m_pProgressSvc->SetProgressStep( 1 );						// fine granularity: advance progress on each step since individual computations are slow
	m_pProgressSvc->SetProgressState( PBST_PAUSED );			// yellow bar
}
